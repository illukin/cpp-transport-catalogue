#include "json.h"
#include "json_reader.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <iostream>

using namespace std::string_literals;

int main () {
  tc::TransportCatalogue cat;
  auto base_req = "base_requests"s;
  auto stat_req = "stat_requests"s;
  auto render_set = "render_settings"s;
  auto routing_set = "routing_settings"s;
  json::Array json;

  auto doc = json::Load(std::cin).GetRoot().AsMap();
  auto invalid_json_msg = "Invalid JSON. Exit."s;

  // Заполнение базы данных
  if (doc.find(base_req) != doc.end()) {
    json = doc.at(base_req).AsArray();
    tc::filler::FillDB(cat, json);
  } else {
    std::cerr << invalid_json_msg << std::endl;
    return EXIT_FAILURE;
  }

  // Настройка маршрутов
  tc::router::RoutingSettings routing_settings;
  if (doc.find(routing_set) != doc.end()) {
    routing_settings =
      tc::router::ReadRoutingSettings(doc.at(routing_set).AsMap());
  } else {
    std::cerr << invalid_json_msg << std::endl;
    return EXIT_FAILURE;
  }
  tc::router::Router router{routing_settings, cat};

  // Настройка рендеринга
  tc::renderer::MapRenderer renderer;
  if (doc.find(render_set) != doc.end()) {
    auto render_settings =
      tc::renderer::ReadRenderSettings(doc.at(render_set).AsMap());
    renderer = tc::renderer::MapRenderer(std::move(render_settings));
  } else {
    std::cerr << invalid_json_msg << std::endl;
    return EXIT_FAILURE;
  }

  // Обработка запросов и печать результатов
  tc::RequestHandler handler(cat, renderer, router);
  if (doc.find(stat_req) != doc.end()) {
    json = doc.at(stat_req).AsArray();
    tc::printer::ProcessQueries(json, handler, std::cout);
  }

  return 0;
}
