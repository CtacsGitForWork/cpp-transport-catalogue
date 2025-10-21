#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

#include <optional>
#include <unordered_map>
#include <string>
#include <string_view>
#include <memory>

class TransportRouter {
public:
    struct RoutingSettings {
        int bus_wait_time = 0;         // в минутах
        double bus_velocity = 0.0;     // в км/ч
    };

    struct RouteItem {
        std::string type;       // "Ожидание" или "Поездка"
        std::string name;       // имя маршрута
        double time;            // время поездки по маршруту
        int span_count = 0;     // кол-во перегонов
    };

    struct RouteResult {
        double total_time;
        std::vector<RouteItem> items;
    };

    explicit TransportRouter(const transport_catalogue::TransportCatalogue& db, RoutingSettings settings);

    std::optional<RouteResult> BuildRoute(std::string_view from, std::string_view to) const;

private:
    static constexpr double KMH_TO_M_PER_MIN = 1000.0 / 60.0;

    using Graph = graph::DirectedWeightedGraph<double>;
    using Router = graph::Router<double>;

    void BuildGraph();
    void InitVertices();
    void AddWaitEdges();
    void AddTripEdges();
    void AddTripEdgesForRange(const std::vector<const domain::Stop*>& stops, const std::string& bus_name);
    void AddEdge(graph::VertexId from, graph::VertexId to, double weight, std::string_view bus_name, int span_count, double real_time);

    const transport_catalogue::TransportCatalogue& db_;
    RoutingSettings settings_;
    Graph graph_;
    std::unique_ptr<Router> router_;

    std::unordered_map<const domain::Stop*, graph::VertexId> stop_to_vertex_;
    std::vector<const domain::Stop*> vertex_to_stop_;

    struct EdgeInfo {
        std::string bus_name;
        int span_count;
        double real_time; // Без учёта штрафа
    };
    std::unordered_map<graph::EdgeId, EdgeInfo> edge_info_;
};
