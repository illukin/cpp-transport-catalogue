#pragma once

#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <filesystem>

namespace tc::serial {

void SerializeDB(const TransportCatalogue &cat,
  const renderer::RenderSettings &rs,
  const router::TransportRouter &router,
  const std::filesystem::path &path);

void DeserializeDB(TransportCatalogue &cat, renderer::RenderSettings &rs,
  router::TransportRouter &router, const std::filesystem::path &path);

} // namespace tc::serial
