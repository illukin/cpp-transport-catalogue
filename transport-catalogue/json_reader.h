#pragma once

#include "json.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

#include <string>
#include <vector>

namespace tc {

namespace filler {

void FillDB(TransportCatalogue &cat, json::Array &data);

} // namespace filler

namespace printer {

void ProcessQueries(const TransportCatalogue &cat, json::Array &data,
  renderer::MapRenderer &renderer, std::ostream &out);

} // namespace printer

namespace renderer {

RenderSettings ReadRenderSettings(const json::Dict &render_settings);

} // namespace renderer

} // namespace tc