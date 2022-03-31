#include "geo.h"
#include "transport_catalogue.h"

#include <unordered_set>

namespace tc {

void TransportCatalogue::AddStop(Stop stop) {
  auto &ref = stops_.emplace_back(std::move(stop.name),
    stop.lat, stop.lng);

  name_to_stop_[ref.name] = &ref;
}

void TransportCatalogue::AddRoute(Bus bus) {
  auto &ref = buses_.emplace_back(std::move(bus.name), bus.stops);

  for (const auto stop : bus.stops) {
    stop_to_buses_[stop->name].insert(&ref);
  }

  name_to_bus_[ref.name] = &ref;
}

void TransportCatalogue::SetDistance(std::pair<Stop *, Stop *> &stops,
  int distance) {
  distances_[stops] = distance;
}

int TransportCatalogue::GetDistance(Stop *a, Stop *b) const {
  // Расстояние от А до Б
  auto it = distances_.find({a, b});

  if (it == distances_.end()) {
    // Расстояние от Б до А
    it = distances_.find({b, a});
    if (it == distances_.end()) {
      return -1;
    }
  }

  return it->second;
}

Stop *TransportCatalogue::GetStop(const std::string_view name) const {
  auto it = name_to_stop_.find(name);

  if (it != name_to_stop_.end()) {
    return it->second;
  }

  return nullptr;
}

Bus *TransportCatalogue::GetBus(const std::string_view name) const {
  auto it = name_to_bus_.find(name);

  if (it != name_to_bus_.end()) {
    return it->second;
  }

  return nullptr;
}

BusInfo TransportCatalogue::GetBusInfo(Bus *bus) const {
  BusInfo info;
  double fact_route_length = 0, line_route_length = 0;
  Route &route = bus->stops;

  // Уникальные остановки
  std::unordered_set<Stop *> unique_stops(route.begin(), route.end());

  // Расчёт длины маршрута по координатам
  for (auto i = 0; i < route.size() - 1; ++i) {
    geo::Coordinates first = {(*route[i]).lat, (*route[i]).lng};
    geo::Coordinates second = {(*route[i + 1]).lat, (*route[i + 1]).lng};
    line_route_length += ComputeDistance(first, second);
  }

  // Рассчёт длины маршрута по заданным пользователем значениям
  for (auto i = 0; i < route.size() - 1; ++i) {
    fact_route_length += GetDistance(route[i], route[i + 1]);
  }

  info.total_stops = route.size();
  info.unique_stops = unique_stops.size();
  info.fact_route_length = fact_route_length;
  info.line_route_length = line_route_length;

  return info;
}

const Buses &TransportCatalogue::GetBusesByStop(std::string_view name) const {
  static const std::set<Bus *, BusPtrComparator> empty;
  auto it = stop_to_buses_.find(name);

  if (it != stop_to_buses_.end()) {
    return it->second;
  }

  return empty;
}

} // namespace tc
