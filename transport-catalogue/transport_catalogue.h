#pragma once

#include "domain.h"

#include <list>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <utility>

namespace tc {

struct BusPtrComparator {
  bool operator()(Bus *lhs, Bus *rhs) const;
};

struct BusInfo {
  size_t total_stops{};
  size_t unique_stops{};
  double fact_route_length{};
  double line_route_length{};
};

using Buses = std::set<Bus *, BusPtrComparator>;

struct Hasher {
  static const size_t salt = 77;
  static size_t CountStopHash(const Stop *stop);
  static size_t CountRouteHash(const Route *route);
  size_t operator()(const std::pair<Stop *, Stop *> &stops) const;
  size_t operator()(const Bus *bus) const;
};

class TransportCatalogue {
public:
  void AddStop(Stop stop);
  void AddRoute(Bus bus);
  void SetDistance(std::pair<Stop *, Stop *> &stops, int distance);

  Stop *GetStop(std::string_view name) const;
  Bus *GetBus(std::string_view name) const;
  BusInfo GetBusInfo(Bus *bus) const;
  const Buses &GetBusesByStop(std::string_view name) const;
  int GetDistance(Stop *a, Stop *b) const;
  const std::list<Bus> &GetBuses() const;

private:
  std::list<Stop> stops_;
  std::list<Bus> buses_;
  std::unordered_map<std::string_view, Stop *> name_to_stop_;
  std::unordered_map<std::string_view, Bus *> name_to_bus_;
  std::unordered_map<std::string_view, Buses> stop_to_buses_;
  std::unordered_map<Bus *, BusInfo, Hasher> bus_to_info_;
  std::unordered_map<std::pair<Stop *, Stop *>, int, Hasher> distances_;
};

} // namespace tc
