#include "domain.h"

#include <utility>

namespace tc {

Stop::Stop(std::string stop_name, double latitude, double longitude)
  : name(std::move(stop_name)), lat(latitude), lng(longitude)
{}

Bus::Bus(std::string bus_name, std::vector<Stop *> bus_stops,
  std::vector<Stop *> final_stops) : name(std::move(bus_name)),
  stops(std::move(bus_stops)), final_stops(std::move(final_stops)) {}

bool Bus::operator<(Bus &other)
{
  return std::lexicographical_compare(name.begin(), name.end(),
    other.name.begin(), other.name.end());
}

} // namespace tc
