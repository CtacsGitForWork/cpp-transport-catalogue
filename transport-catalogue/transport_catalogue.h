#pragma once

#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <vector>
#include "domain.h"
#include "geo.h"

namespace transport_catalogue {

    struct BusInfo {
        int stops_count;
        int unique_stops_count;
        int route_length;
        double curvature;
    };

    struct PairPointersHasher {
        size_t operator()(const std::pair<const domain::Stop*, const domain::Stop*>& pair) const {
            auto hash1 = std::hash<const void*>{}(pair.first);
            auto hash2 = std::hash<const void*>{}(pair.second);
            return hash1 + 37 * hash2;
        }
    };

    class TransportCatalogue {
    public:
        void AddStop(const domain::Stop& stop);
        void AddBus(const domain::Bus& bus);

        const domain::Bus* FindBus(std::string_view name) const;
        const domain::Stop* FindStop(std::string_view name) const;

        std::optional<BusInfo> GetBusInfo(std::string_view bus_name) const;
        const std::unordered_set<std::string_view>& GetBusesForStop(std::string_view stop_name) const;

        void SetDistance(const domain::Stop* from, const domain::Stop* to, int distance);
        int GetDistance(const domain::Stop* from, const domain::Stop* to) const;

        const std::deque<domain::Bus>& GetAllBuses() const;
        const std::deque<domain::Stop>& GetAllStops() const;

    private:
        std::deque<domain::Stop> stops_;
        std::deque<domain::Bus> buses_;
        std::unordered_map<std::string_view, const domain::Stop*> stop_name_to_stop_;
        std::unordered_map<std::string_view, const domain::Bus*> bus_name_to_bus_;
        std::unordered_map<std::string_view, std::unordered_set<std::string_view>> stop_to_buses_;
        std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, PairPointersHasher> distances_;
    };

} // namespace transport_catalogue

