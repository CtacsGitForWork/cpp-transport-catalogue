#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include <optional>
#include <string>
#include <unordered_set>

class RequestHandler {
public:
    RequestHandler(const transport_catalogue::TransportCatalogue& db,
                   const map_renderer::MapRenderer& renderer,
                   const TransportRouter* router = nullptr);

    // Основные методы API
    std::optional<transport_catalogue::BusInfo> GetBusStat(const std::string& bus_name) const;
    const std::unordered_set<std::string_view>* GetBusesByStop(const std::string& stop_name) const;
    const domain::Stop* FindStop(const std::string& stop_name) const;

    // Метод для рендеринга карты
    svg::Document RenderMap() const;

    // Метод для построения маршрута
    std::optional<TransportRouter::RouteResult> BuildRoute(std::string_view from, std::string_view to) const;

private:
    const transport_catalogue::TransportCatalogue& db_;
    const map_renderer::MapRenderer& renderer_;
    const TransportRouter* router_;
};
