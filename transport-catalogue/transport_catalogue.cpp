#include "transport_catalogue.h"

#include <algorithm>

namespace transport_catalogue {

    void TransportCatalogue::AddStop(const domain::Stop& stop) {
        stops_.push_back(stop);
        const auto& stop_ref = stops_.back();
        stop_name_to_stop_[stop_ref.name] = &stop_ref;
        stop_to_buses_[stop_ref.name];
    }

    void TransportCatalogue::AddBus(const domain::Bus& bus) {
        buses_.push_back(bus);
        auto& bus_ref = buses_.back();
        bus_name_to_bus_[bus_ref.name] = &bus_ref;

        for (const auto stop : bus_ref.stops) {
            stop_to_buses_[stop->name].insert(bus_ref.name);
        }
    }

    const domain::Bus* TransportCatalogue::FindBus(std::string_view name) const {
        if (auto it = bus_name_to_bus_.find(name); it != bus_name_to_bus_.end()) {
            return it->second;
        }
        return nullptr;
    }

    const domain::Stop* TransportCatalogue::FindStop(std::string_view name) const {
        if (auto it = stop_name_to_stop_.find(name); it != stop_name_to_stop_.end()) {
            return it->second;
        }
        return nullptr;
    }

    void TransportCatalogue::SetDistance(const domain::Stop* from, const domain::Stop* to, int distance) {
        distances_[{from, to}] = distance;
    }

    int TransportCatalogue::GetDistance(const domain::Stop* from, const domain::Stop* to) const {
        if (auto it = distances_.find({ from, to }); it != distances_.end()) {
            return it->second;
        }
        if (auto it = distances_.find({ to, from }); it != distances_.end()) {
            return it->second;
        }
        return static_cast<int>(geo::ComputeDistance(from->coordinates, to->coordinates));
    }

    const std::deque<domain::Bus>& TransportCatalogue::GetAllBuses() const {
        return buses_;
    }

    const std::deque<domain::Stop>& TransportCatalogue::GetAllStops() const {
        return stops_;
    }

    std::optional<BusInfo> TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
        const domain::Bus* bus = FindBus(bus_name);
        if (!bus || bus->stops.empty()) {
            return std::nullopt;
        }

        BusInfo info;

        // Учёт типа маршрута (кольцевой/линейный)
        info.stops_count = bus->is_roundtrip ? bus->stops.size() : bus->stops.size() * 2 - 1;

        // Уникальные остановки
        std::unordered_set<std::string_view> unique_stops;
        for (const domain::Stop* stop : bus->stops) {
            unique_stops.insert(stop->name);
        }
        info.unique_stops_count = unique_stops.size();

        // Расчёт расстояний
        double straight_distance = 0.0;
        int road_distance = 0;

        // Прямой путь
        for (size_t i = 1; i < bus->stops.size(); ++i) {
            road_distance += GetDistance(bus->stops[i - 1], bus->stops[i]);
            straight_distance += geo::ComputeDistance(
                bus->stops[i - 1]->coordinates,
                bus->stops[i]->coordinates
            );
        }

        // Обратный путь (для линейных маршрутов)
        if (!bus->is_roundtrip) {
            // Добавляем дорожное расстояние обратного пути
            for (size_t i = bus->stops.size() - 1; i > 0; --i) {
                road_distance += GetDistance(bus->stops[i], bus->stops[i - 1]);
            }

            // Добавляем географическое расстояние обратного пути
            // (оно такое же как и прямое, так как координаты те же)
            straight_distance *= 2.0;
        }

        info.route_length = road_distance;
        info.curvature = straight_distance > 0 ? road_distance / straight_distance : 0.0;

        return info;
    }

    const std::unordered_set<std::string_view>& TransportCatalogue::GetBusesForStop(std::string_view stop_name) const {
        static const std::unordered_set<std::string_view> empty_result;
        if (auto it = stop_to_buses_.find(stop_name); it != stop_to_buses_.end()) {
            return it->second;
        }
        return empty_result;
    }

} // namespace transport_catalogue

