#include "json.h"
#include "json_reader.h"
#include "request_handler.h"
#include "serialization.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <iostream>
#include <string_view>

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
  stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    PrintUsage();
    return 1;
  }

  const std::string_view mode(argv[1]);
  tc::TransportCatalogue cat;
  auto base_req = "base_requests"s;
  auto stat_req = "stat_requests"s;
  auto render_set = "render_settings"s;
  auto routing_set = "routing_settings"s;
  auto serialization_settings = "serialization_settings"s;
  json::Array json;
  tc::renderer::RenderSettings render_settings;
  tc::router::RoutingSettings routing_settings;

  auto doc = json::Load(std::cin).GetRoot().AsMap();
  auto invalid_json_msg = "Invalid JSON. Exit."s;
  auto file = doc.at(serialization_settings).AsMap().at("file"s).AsString();

  if (mode == "make_base"sv) {
    if (doc.find(base_req) == doc.end()
      || doc.find(render_set) == doc.end()
      || doc.find(routing_set) == doc.end()) {
      std::cerr << invalid_json_msg << std::endl;
      return EXIT_FAILURE;
    }

    json = doc.at(base_req).AsArray();
    tc::filler::FillDB(cat, json);

    render_settings
      = tc::renderer::ReadRenderSettings(doc.at(render_set).AsMap());
    routing_settings =
      tc::router::ReadRoutingSettings(doc.at(routing_set).AsMap());
    tc::router::TransportRouter router{routing_settings, cat};

    // Сериализация базы данных
    tc::serial::SerializeDB(cat, render_settings, router, file);

  } else if (mode == "process_requests"sv) {
    tc::router::TransportRouter router;

    // Десериализация базы данных
    tc::serial::DeserializeDB(cat, render_settings, router, file);

    // Настройка рендеринга
    tc::renderer::MapRenderer renderer;
    renderer = tc::renderer::MapRenderer(std::move(render_settings));

    // Обработка запросов и печать результатов
    tc::RequestHandler handler(cat, renderer, router);
    if (doc.find(stat_req) != doc.end()) {
      json = doc.at(stat_req).AsArray();
      tc::printer::ProcessQueries(json, handler, std::cout);
    }
  } else {
    PrintUsage();
    return 1;
  }
}
