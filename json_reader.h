#pragma once

#include "transport_catalogue.h"
#include "json.h"
#include "svg.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "json_builder.h"

#include <vector>

class JsonReader {
public:
    explicit JsonReader(transport_catalogue::TransportCatalogue& db);

    void ParseBaseRequests(const json::Node& root);
    void ParseStatRequests(const json::Node& root);
    json::Node ProcessStatRequests(const RequestHandler& handler) const;

    map_renderer::RenderSettings ParseRenderSettings(const json::Node& root) const;
    TransportRouter::RoutingSettings ParseRoutingSettings(const json::Node& root) const;

private:
    svg::Color ParseColor(const json::Node& node) const;

    void ParseStop(const json::Dict& map);
    void ParseDistances(const json::Dict& map);
    void ParseBus(const json::Dict& map);

    void ProcessBusRequest(const json::Dict& map, const RequestHandler& handler, json::Builder& builder) const;
    void ProcessStopRequest(const json::Dict& map, const RequestHandler& handler, json::Builder& builder) const;
    void ProcessMapRequest(const json::Dict& /*map*/, const RequestHandler& handler, json::Builder& builder) const;
    void ProcessRouteRequest(const json::Dict& map, const RequestHandler& handler, json::Builder& builder) const;

    transport_catalogue::TransportCatalogue& db_;
    std::vector<json::Node> stat_requests_;
};
