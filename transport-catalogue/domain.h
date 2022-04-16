#pragma once

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

} // namespace tc
