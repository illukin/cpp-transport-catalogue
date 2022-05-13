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

bool BusPtrComparator::operator()(Bus *lhs, Bus *rhs) const {
  return *lhs < *rhs;
}

size_t Hasher::CountStopHash(const Stop *stop) {
  return std::hash<std::string>{}(stop->name)
    + std::hash<double>{}(stop->lat) * salt
    + std::hash<double>{}(stop->lng) * salt * salt;
}

size_t Hasher::CountRouteHash(const Route *route) {
  size_t hash = 0;

  for (const auto r : *route) {
    hash += CountStopHash(r);
  }

  return hash;
}

size_t Hasher::operator()(const std::pair<Stop *, Stop *> &stops) const {
  return CountStopHash(stops.first) * salt
    + CountStopHash(stops.second);
}

size_t Hasher::operator()(const Bus *bus) const {
  return std::hash<std::string>{}(bus->name)
    + CountRouteHash(&bus->stops) * salt;
}

size_t Hasher::operator()(const Stop *stop) const {
  return CountStopHash(stop);
}

} // namespace tc
