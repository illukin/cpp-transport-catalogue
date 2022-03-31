#pragma once

#include <list>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <utility>

namespace tc {

struct Stop {
  Stop() = default;
  Stop(std::string stop_name, double latitude, double longitude)
    : name(std::move(stop_name)), lat(latitude), lng(longitude) {};

  std::string name;
  double lat{};
  double lng{};
};

using Route = std::vector<Stop *>;

struct Bus {
  Bus() = default;
  Bus(std::string bus_name, std::vector<Stop *> bus_stops)
    : name(std::move(bus_name)), stops(std::move(bus_stops)) {};

  bool operator<(Bus &other) {
    return std::lexicographical_compare(name.begin(), name.end(),
      other.name.begin(), other.name.end());
  }

  std::string name;
  Route stops;
};

struct BusPtrComparator {
  bool operator() (Bus *lhs, Bus *rhs) const {
    return *lhs < *rhs;
  }
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

  static auto CountStopHash(const Stop *stop) {
    return std::hash<std::string>{}(stop->name)
      + std::hash<double>{}(stop->lat) * salt
      + std::hash<double>{}(stop->lng) * salt * salt;
  }

  size_t operator()(const std::pair<Stop *, Stop *> &stops) const {
    return CountStopHash(stops.first) * salt
      + CountStopHash(stops.second);
  }
};

class TransportCatalogue {
public:
  void AddStop(Stop);
  void AddRoute(Bus);
  void SetDistance(std::pair<Stop *, Stop *> &, int);

  Stop *GetStop(std::string_view) const;
  Bus *GetBus(std::string_view) const;
  BusInfo GetBusInfo(Bus *) const;
  const Buses &GetBusesByStop(std::string_view) const;
  int GetDistance(Stop *, Stop *) const;

private:
  std::list<Stop> stops_;
  std::list<Bus> buses_;
  std::unordered_map<std::string_view, Stop *> name_to_stop_;
  std::unordered_map<std::string_view, Bus *> name_to_bus_;
  std::unordered_map<std::string_view, Buses> stop_to_buses_;
  std::unordered_map<std::pair<Stop *, Stop *>, int, Hasher> distances_;
};

} // namespace tc
