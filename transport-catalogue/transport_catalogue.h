#pragma once

#include "domain.h"

#include <list>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <utility>

namespace tc {

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
  std::unordered_map<std::pair<Stop *, Stop *>, int, Hasher>
  GetDistances() const;
  const std::list<Bus> &GetBuses() const;
  const std::list<Stop> &GetStops() const;

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
