#include "request_handler.h"

#include <algorithm>

RequestHandler::RequestHandler(const transport_catalogue::TransportCatalogue& db,
                               const map_renderer::MapRenderer& renderer,
                               const TransportRouter* router)
    : db_(db), renderer_(renderer), router_(router) {}

std::optional<transport_catalogue::BusInfo> RequestHandler::GetBusStat(const std::string& bus_name) const {
    auto info = db_.GetBusInfo(bus_name);
    if (!info.has_value() || info->stops_count == 0) {
        return std::nullopt;
    }
    return info;
}

const domain::Stop* RequestHandler::FindStop(const std::string& stop_name) const {
    return db_.FindStop(stop_name);
}


const std::unordered_set<std::string_view>* RequestHandler::GetBusesByStop(const std::string& stop_name) const {
    const domain::Stop* stop = db_.FindStop(stop_name);
    if (!stop) {
        return nullptr; // возвращаем nullptr только если остановки не существует*/
    }
    // Всегда возвращаем указатель на множество, даже если оно пустое
    // (это означает, что остановка существует, но через нее не проходят автобусы
    return &db_.GetBusesForStop(stop_name);
}

svg::Document RequestHandler::RenderMap() const {
    // 1. Все остановки, через которые проходят автобусы
    std::vector<const domain::Stop*> used_stops;
    for (const domain::Stop& stop : db_.GetAllStops()) {
        const auto* buses = GetBusesByStop(stop.name); // Используем метод GetBusesByStop RequestHandler'a

        if (buses && !buses->empty()) { // Проверяем и указатель, и пустоту{
            used_stops.push_back(&stop);
        }
    }
       
    // Сортируем остановки по имени
    std::sort(used_stops.begin(), used_stops.end(),
        [](const domain::Stop* lhs, const domain::Stop* rhs) {
            return lhs->name < rhs->name;
        });

    // 2. Все автобусы, у которых есть хотя бы одна остановка
    std::vector<const domain::Bus*> buses;
    for (const domain::Bus& bus : db_.GetAllBuses()) {
        if (!bus.stops.empty()) {
            buses.push_back(&bus);
        }
    }

    // Сортируем автобусы по имени
    std::sort(buses.begin(), buses.end(),
        [](const domain::Bus* lhs, const domain::Bus* rhs) {
            return lhs->name < rhs->name;
        });

    // 3. Географические координаты только нужных остановок
    std::vector<geo::Coordinates> geo_coords;
    geo_coords.reserve(used_stops.size());
    for (const domain::Stop* stop : used_stops) {
        geo_coords.push_back(stop->coordinates);
    }

    // Проектор (только если есть координаты)
    if (geo_coords.empty()) {
        return svg::Document{}; // Возвращаем пустой документ, если нет остановок
    }

    // 4. Проектор
    const map_renderer::RenderSettings& settings = renderer_.GetSettings();
    map_renderer::SphereProjector projector{
        geo_coords.begin(), geo_coords.end(),
        settings.width,
        settings.height,
        settings.padding
    };

    // 5. Рендер
    return renderer_.Render(buses, used_stops, projector);
}

std::optional<TransportRouter::RouteResult> RequestHandler::BuildRoute(std::string_view from, std::string_view to) const {
    if (!router_) return std::nullopt;
    return router_->BuildRoute(from, to);
}
