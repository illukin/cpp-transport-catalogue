#pragma once

#include "json.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

#include <string>
#include <vector>

namespace tc {

class RequestHandler;

namespace filler {

void FillDB(TransportCatalogue &cat, json::Array &data);

} // namespace filler

namespace printer {

void ProcessQueries(json::Array &data, RequestHandler &handler,
  std::ostream &out);

} // namespace printer

namespace renderer {

RenderSettings ReadRenderSettings(const json::Dict &render_settings);

} // namespace renderer

} // namespace tc