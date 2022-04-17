#include "request_handler.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

namespace tc {

RequestHandler::RequestHandler(const TransportCatalogue& db,
  const renderer::MapRenderer& renderer) : db_(db), renderer_(renderer) {}

std::optional<BusInfo>
RequestHandler::GetBusInfo(const std::string_view &bus_name) const {
  auto bus = db_.GetBus(bus_name);

  return bus ? std::make_optional(db_.GetBusInfo(bus)) : std::nullopt;
}

const Buses *
RequestHandler::GetBusesByStop(const std::string_view &stop_name) const {
  auto stop = db_.GetStop(stop_name);

  return stop ? &(db_.GetBusesByStop(stop->name)) : nullptr;
}

svg::Document RequestHandler::RenderMap() const {
  svg::Document doc;
  auto buses = db_.GetBuses();

  renderer_.RenderMap(buses.begin(), buses.end()).Draw(doc);

  return doc;
}

} // namespace tc
