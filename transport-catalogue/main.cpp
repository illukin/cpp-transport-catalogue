#include "json.h"
#include "json_reader.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

#include <iostream>

using namespace std::string_literals;

int main () {
  tc::TransportCatalogue cat;
  auto base_req = "base_requests"s;
  auto stat_req = "stat_requests"s;
  auto render_set = "render_settings"s;
  json::Array json;

  auto doc = json::Load(std::cin).GetRoot().AsMap();

  // Заполнение базы данных
  if (doc.find(base_req) != doc.end()) {
    json = doc.at(base_req).AsArray();
    tc::filler::FillDB(cat, json);
  }

  // Настройка рендеринга
  tc::renderer::MapRenderer renderer;
  if (doc.find(render_set) != doc.end()) {
    auto settings = tc::renderer::ReadRenderSettings(doc.at(render_set).AsMap());
    renderer = tc::renderer::MapRenderer(std::move(settings));
  }

  // Обработка запросов и печать результатов
  if (doc.find(stat_req) != doc.end()) {
    json = doc.at(stat_req).AsArray();
    tc::printer::ProcessQueries(cat, json, renderer, std::cout);
  }

  return 0;
}
