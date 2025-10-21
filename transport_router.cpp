#include "transport_router.h"

using namespace std;

TransportRouter::TransportRouter(const transport_catalogue::TransportCatalogue& db, RoutingSettings settings)
    : db_(db), settings_(settings) {
    BuildGraph();
}

void TransportRouter::BuildGraph() {
    InitVertices();
    graph_ = Graph(vertex_to_stop_.size());
    AddWaitEdges();
    AddTripEdges();
    router_ = std::make_unique<Router>(graph_);
}

void TransportRouter::InitVertices() {
    const auto& stops = db_.GetAllStops();
    vertex_to_stop_.clear();
    stop_to_vertex_.clear();

    // Каждой остановке сопоставляем две вершины: ожидание и поездка
    for (const auto& stop : stops) {
        graph::VertexId wait_vertex = vertex_to_stop_.size();
        stop_to_vertex_[&stop] = wait_vertex;
        vertex_to_stop_.push_back(&stop); // Ожидание
        vertex_to_stop_.push_back(&stop); // Поездка
    }
}

void TransportRouter::AddEdge(graph::VertexId from, graph::VertexId to, double weight, std::string_view bus_name, int span_count, double real_time) {
    graph::EdgeId id = graph_.AddEdge({from, to, weight});
    edge_info_[id] = {std::string(bus_name), span_count, real_time};
}

void TransportRouter::AddWaitEdges() {
    for (const auto& [stop, wait_vertex] : stop_to_vertex_) {
        graph::VertexId bus_vertex = wait_vertex + 1;
        AddEdge(wait_vertex, bus_vertex, static_cast<double>(settings_.bus_wait_time), "", 0, 0.0);
    }
}

void TransportRouter::AddTripEdges() {
    for (const auto& bus : db_.GetAllBuses()) {
        const auto& stops = bus.stops;
        if (stops.size() < 2) continue;

        AddTripEdgesForRange(stops, bus.name);

        // Обратное направление — только для НЕ-кольцевых маршрутов
        if (!bus.is_roundtrip) {
            std::vector<const domain::Stop*> reversed(stops.rbegin(), stops.rend());
            AddTripEdgesForRange(reversed, bus.name);
        }
    }
}

void TransportRouter::AddTripEdgesForRange(const std::vector<const domain::Stop*>& stops, const std::string& bus_name) {
    constexpr double PENALTY_PER_STOP = 1e-3;    // мягкий штраф за раннюю пересадку, чтобы при прочих равных ехать на одном маршруте до упора

    for (size_t i = 0; i + 1 < stops.size(); ++i) {
        graph::VertexId from_bus = stop_to_vertex_.at(stops[i]) + 1;
        int distance = 0;

        for (size_t j = i + 1; j < stops.size(); ++j) {
            if (stops[i] == stops[j]) continue;  // если остановка одинаковая, пропускаем ребро  

            distance += db_.GetDistance(stops[j - 1], stops[j]);
            double base_time = distance / (settings_.bus_velocity * KMH_TO_M_PER_MIN);
            double penalty = (j - i < stops.size() - i - 1) ? PENALTY_PER_STOP * (stops.size() - j) : 0.0;
            double weight = base_time + penalty;

            graph::VertexId to_wait = stop_to_vertex_.at(stops[j]);
            AddEdge(from_bus, to_wait, weight, bus_name, static_cast<int>(j - i), base_time);
        }
    }
}

std::optional<TransportRouter::RouteResult> TransportRouter::BuildRoute(std::string_view from, std::string_view to) const {
    const domain::Stop* from_stop = db_.FindStop(from);
    const domain::Stop* to_stop = db_.FindStop(to);

    // Проверяем, что остановки существуют
    if (!from_stop || !to_stop) {
        return std::nullopt;
    }

    // Проверяем, не совпадают ли остановки
    if (from_stop == to_stop) {
        return RouteResult{0.0, {}};
    }

    graph::VertexId from_vertex = stop_to_vertex_.at(from_stop);
    graph::VertexId to_vertex = stop_to_vertex_.at(to_stop);

    auto route = router_->BuildRoute(from_vertex, to_vertex);
    if (!route) return std::nullopt;

    RouteResult result;
    result.total_time = 0.0;

    for (graph::EdgeId edge_id : route->edges) {
        const auto& edge = graph_.GetEdge(edge_id);
        const auto& info = edge_info_.at(edge_id);

        if (info.bus_name.empty()) {
            result.items.push_back({                // Ожидание
                "Wait",
                vertex_to_stop_.at(edge.from)->name,
                edge.weight,
                0
            });
            result.total_time += edge.weight;
        } else {
            result.items.push_back({                // Поездка
                "Bus",
                info.bus_name,
                info.real_time,
                info.span_count
            });
            result.total_time += info.real_time;
        }
    }

    return result;
}
