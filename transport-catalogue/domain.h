#pragma once

#include <set>
#include <string>
#include <vector>

namespace tc {

struct Stop {
  Stop() = default;

  Stop(std::string stop_name, double latitude, double longitude);

  std::string name;
  double lat{};
  double lng{};
};

using Route = std::vector<Stop *>;

struct Bus {
  Bus() = default;

  Bus(std::string bus_name, std::vector<Stop *> bus_stops,
    std::vector<Stop *> final_stops);

  bool operator<(Bus &other);

  std::string name;
  Route stops;
  Route final_stops;
};

struct BusInfo {
  size_t total_stops{};
  size_t unique_stops{};
  double fact_route_length{};
  double line_route_length{};
};

struct BusPtrComparator {
  bool operator()(Bus *lhs, Bus *rhs) const;
};

using Buses = std::set<Bus *, BusPtrComparator>;

struct Hasher {
  static const size_t salt = 77;
  static size_t CountStopHash(const Stop *stop);
  static size_t CountRouteHash(const Route *route);
  size_t operator()(const std::pair<Stop *, Stop *> &stops) const;
  size_t operator()(const Bus *bus) const;
  size_t operator()(const Stop *stop) const;
};

} // namespace tc
