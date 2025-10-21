#include "json_reader.h"

#include <algorithm>
#include <sstream>

using namespace std;

JsonReader::JsonReader(transport_catalogue::TransportCatalogue& db) : db_(db) {}

void JsonReader::ParseBaseRequests(const json::Node& root) {
    const auto& base_requests = root.AsDict().at("base_requests").AsArray();

    for (const auto& req : base_requests) {      // Сначала добавляем все остановки
        const auto& map = req.AsDict();
        if (map.at("type").AsString() == "Stop") {
            ParseStop(map);
        }
    }

    for (const auto& req : base_requests) {      // Затем добавляем расстояния между остановками
        const auto& map = req.AsDict();
        if (map.at("type").AsString() == "Stop" && map.count("road_distances")) {
            ParseDistances(map);
        }
    }

    for (const auto& req : base_requests) {      // После этого обрабатываем маршруты
        const auto& map = req.AsDict();
        if (map.at("type").AsString() == "Bus") {
            ParseBus(map);
        }
    }
}

void JsonReader::ParseStatRequests(const json::Node& root) {
    stat_requests_.clear();
    
    if (auto it = root.AsDict().find("stat_requests"); it != root.AsDict().end()) {
        stat_requests_ = it->second.AsArray();
    }
}

json::Node JsonReader::ProcessStatRequests(const RequestHandler& handler) const {
    json::Array results;

    for (const auto& req : stat_requests_) {
        if (!req.IsDict()) continue;

        const auto& map = req.AsDict();
        int id = map.at("id").AsInt();
        const std::string& type = map.at("type").AsString();

        json::Builder builder;
        builder.StartDict().Key("request_id").Value(id);

        if (type == "Bus") {
            ProcessBusRequest(map, handler, builder);
        }
        else if (type == "Stop") {
            ProcessStopRequest(map, handler, builder);
        }
        else if (type == "Map") {
            ProcessMapRequest(map, handler, builder);
        }

        else if (type == "Route") {
            ProcessRouteRequest(map, handler, builder);
        }
        
        builder.EndDict();
        results.push_back(builder.Build());
    }

    return json::Node(std::move(results));
}

map_renderer::RenderSettings JsonReader::ParseRenderSettings(const json::Node& root) const {
    // Парсинг настроек визуализации
    map_renderer::RenderSettings render_settings;

    if (auto it = root.AsDict().find("render_settings"); it != root.AsDict().end()) {
        const auto& settings = it->second.AsDict();

        render_settings.width = settings.at("width").AsDouble();
        render_settings.height = settings.at("height").AsDouble();
        render_settings.padding = settings.at("padding").AsDouble();
        render_settings.line_width = settings.at("line_width").AsDouble();
        render_settings.stop_radius = settings.at("stop_radius").AsDouble();
        render_settings.bus_label_font_size = settings.at("bus_label_font_size").AsInt();
        render_settings.bus_label_offset = {
            settings.at("bus_label_offset").AsArray()[0].AsDouble(),
            settings.at("bus_label_offset").AsArray()[1].AsDouble()
        };
        render_settings.stop_label_font_size = settings.at("stop_label_font_size").AsInt();
        render_settings.stop_label_offset = {
            settings.at("stop_label_offset").AsArray()[0].AsDouble(),
            settings.at("stop_label_offset").AsArray()[1].AsDouble()
        };
        render_settings.underlayer_color = ParseColor(settings.at("underlayer_color"));
        render_settings.underlayer_width = settings.at("underlayer_width").AsDouble();

        for (const auto& color_node : settings.at("color_palette").AsArray()) {
            render_settings.color_palette.push_back(ParseColor(color_node));
        }
    }

    return render_settings;
}

TransportRouter::RoutingSettings JsonReader::ParseRoutingSettings(const json::Node& root) const {
    TransportRouter::RoutingSettings settings;

    if (auto it = root.AsDict().find("routing_settings"); it != root.AsDict().end()) {
        const auto& dict = it->second.AsDict();
        settings.bus_wait_time = dict.at("bus_wait_time").AsInt();
        settings.bus_velocity = dict.at("bus_velocity").AsDouble();
    }

    return settings;
}

svg::Color JsonReader::ParseColor(const json::Node& node) const {
    if (node.IsString()) {
        return node.AsString();
    }
    else if (node.IsArray()) {
        const auto& arr = node.AsArray();
        if (arr.size() == 3) {
            return svg::Rgb{
                static_cast<uint8_t>(arr[0].AsInt()),
                static_cast<uint8_t>(arr[1].AsInt()),
                static_cast<uint8_t>(arr[2].AsInt())
            };
        }
        else if (arr.size() == 4) {
            return svg::Rgba{
                static_cast<uint8_t>(arr[0].AsInt()),
                static_cast<uint8_t>(arr[1].AsInt()),
                static_cast<uint8_t>(arr[2].AsInt()),
                arr[3].AsDouble()
            };
        }
    }

    return svg::NoneColor;
}

void JsonReader::ParseStop(const json::Dict& map) {
    domain::Stop stop;
    stop.name = map.at("name").AsString();
    stop.coordinates.lat = map.at("latitude").AsDouble();
    stop.coordinates.lng = map.at("longitude").AsDouble();
    db_.AddStop(stop);
}

void JsonReader::ParseDistances(const json::Dict& map) {
    const auto& distances = map.at("road_distances").AsDict();
    const domain::Stop* from = db_.FindStop(map.at("name").AsString());
    for (const auto& [to_name, dist_node] : distances) {
        const domain::Stop* to = db_.FindStop(to_name);
        if (to) {
            db_.SetDistance(from, to, dist_node.AsInt());
        }
    }
}

void JsonReader::ParseBus(const json::Dict& map) {
    domain::Bus bus;
    bus.name = map.at("name").AsString();
    bus.is_roundtrip = map.at("is_roundtrip").AsBool();

    for (const auto& stop_node : map.at("stops").AsArray()) {
        if (const domain::Stop* stop_ptr = db_.FindStop(stop_node.AsString())) {
            bus.stops.push_back(stop_ptr);
        }
    }

    db_.AddBus(bus);
}

void JsonReader::ProcessBusRequest(const json::Dict& map,
    const RequestHandler& handler,
    json::Builder& builder) const {
    std::string bus_name = map.at("name").AsString();
    auto info_opt = handler.GetBusStat(bus_name);

    if (!info_opt) {
        builder.Key("error_message").Value("not found");
    }
    else {
        const auto& info = *info_opt;
        builder.Key("stop_count").Value(info.stops_count)
            .Key("unique_stop_count").Value(info.unique_stops_count)
            .Key("route_length").Value(info.route_length)
            .Key("curvature").Value(info.curvature);
    }
}

void JsonReader::ProcessStopRequest(const json::Dict& map,
    const RequestHandler& handler,
    json::Builder& builder) const {
    std::string stop_name = map.at("name").AsString();
    const domain::Stop* stop = handler.FindStop(stop_name);

    if (!stop) {
        builder.Key("error_message").Value("not found");
    }
    else {
        const auto* buses = handler.GetBusesByStop(stop_name);
        builder.Key("buses").StartArray();

        if (buses) {
            std::vector<std::string> sorted_buses(buses->begin(), buses->end());
            std::sort(sorted_buses.begin(), sorted_buses.end());
            for (const auto& bus_name : sorted_buses) {
                builder.Value(bus_name);
            }
        }

        builder.EndArray();
    }
}

void JsonReader::ProcessMapRequest(const json::Dict& /*map*/,
    const RequestHandler& handler,
    json::Builder& builder) const {
    std::ostringstream svg_stream;
    handler.RenderMap().Render(svg_stream);
    builder.Key("map").Value(svg_stream.str());
}

void JsonReader::ProcessRouteRequest(const json::Dict& map,
                                     const RequestHandler& handler,
                                     json::Builder& builder) const {
    const std::string from = map.at("from").AsString();
    const std::string to = map.at("to").AsString();

    auto route = handler.BuildRoute(from, to);
    if (!route) {
        builder.Key("error_message").Value("not found");
        return;
    }

    builder.Key("total_time").Value(route->total_time)
           .Key("items").StartArray();

    for (const auto& item : route->items) {
        builder.StartDict().Key("type").Value(item.type);
        if (item.type == "Wait") {
            builder.Key("stop_name").Value(item.name)
                   .Key("time").Value(item.time);
        } else if (item.type == "Bus") {
            builder.Key("bus").Value(item.name)
                   .Key("span_count").Value(item.span_count)
                   .Key("time").Value(item.time);
        }
        builder.EndDict();
    }

    builder.EndArray();
}
