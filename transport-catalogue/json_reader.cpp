#include "json_reader.h"

#include "json_builder.h"
#include "request_handler.h"
#include "transport_catalogue.h"

#include <map>
#include <sstream>

namespace tc {

namespace filler {

void AddStopsToDB(TransportCatalogue &cat, json::Array &data,
  std::map<std::string, json::Dict> &distances) {
  using namespace std::string_literals;

  for (const auto &req : data) {
    auto map_req = req.AsMap();
    auto type_stop = map_req.at("type"s);

    if (type_stop == "Stop"s) {
      auto name = map_req.at("name"s).AsString();
      auto lat = map_req.at("latitude"s).AsDouble();
      auto lng = map_req.at("longitude"s).AsDouble();
      distances[name] = map_req.at("road_distances"s).AsMap();

      cat.AddStop({name, lat, lng});
    }
  }
}

void AddDistancesToDB(TransportCatalogue &cat,
  std::map<std::string, json::Dict> &distances) {
  for (const auto &[stop_a_name, map_distances] : distances) {
    for (const auto &[stop_b_name, dist] : map_distances) {
      auto a = cat.GetStop(stop_a_name);
      auto b = cat.GetStop(stop_b_name);
      auto stops = std::make_pair(a, b);

      cat.SetDistance(stops, dist.AsInt());
    }
  }
}

void AddRoutesToDB(TransportCatalogue &cat, json::Array &data) {
  using namespace std::string_literals;

  for (const auto &req: data) {
    auto map_req = req.AsMap();
    auto type_stop = map_req.at("type"s);

    if (type_stop == "Bus"s) {
      auto name = map_req.at("name"s).AsString();
      auto is_roundtrip = map_req.at("is_roundtrip"s).AsBool();
      auto stops = map_req.at("stops"s).AsArray();
      Route route;
      Route final_stops;

      for (const auto &stop: stops) {
        route.push_back(cat.GetStop(stop.AsString()));
      }

      // Определение конечных остановок
      final_stops.push_back(route.front());
      if (route.front() != route.back()) {
        final_stops.push_back(route.back());
      }

      // Приведение маршрута вида "stop1 - stop2 - ... stopN" к виду
      // "stop1 > stop2 > ... > stopN-1 > stopN > stopN-1 > ... > stop2 > stop1"
      if (!is_roundtrip) {
        route.insert(route.end(), route.rbegin() + 1, route.rend());
      }

      cat.AddRoute({name, route, final_stops});
    }
  }
}

void FillDB(TransportCatalogue &cat, json::Array &data)
{
  std::map<std::string, json::Dict> distances;

  // Добавление остановок в базу данных
  AddStopsToDB(cat, data, distances);

  // Добавление расстояний между остановками в базу данных
  AddDistancesToDB(cat, distances);

  // Добавление маршрутов в базу данных
  AddRoutesToDB(cat, data);
}

} // namespace filler

namespace printer {

void PrintStops(const RequestHandler &handler, std::string &name,
  json::Builder &builder) {
  using namespace std::string_literals;

  const auto buses = handler.GetBusesByStop(name);
  if (buses == nullptr) {
    builder.Key("error_message"s).Value("not found"s);
    return;
  }

  builder.Key("buses").StartArray();
  for (const auto &bus : *buses) {
    builder.Value(bus->name);
  }
  builder.EndArray();
}

void PrintBuses(const RequestHandler &handler, std::string &name,
  json::Builder &builder) {
  using namespace std::string_literals;

  const auto info = handler.GetBusInfo(name);

  if (info == std::nullopt) {
    builder.Key("error_message"s).Value("not found"s);
    return;
  }

  builder
    .Key("curvature"s)
    .Value(info->fact_route_length / info->line_route_length)
    .Key("route_length"s).Value(info->fact_route_length)
    .Key("stop_count"s).Value(static_cast<int>(info->total_stops))
    .Key("unique_stop_count"s).Value(static_cast<int>(info->unique_stops));
}

void PrintMap(const RequestHandler &handler, json::Builder &builder) {
  using namespace std::string_literals;

  std::stringstream render;
  handler.RenderMap().Render(render);

  builder.Key("map"s).Value(render.str());
}

void ProcessQueries(json::Array &data, RequestHandler &handler,
  std::ostream &out) {
  using namespace std::string_literals;
  using namespace std::string_view_literals;

  json::Builder json_builder;

  json_builder.StartArray();
  for (const auto &req : data) {
    auto map_req = req.AsMap();
    auto type_stop = map_req.at("type"s);
    std::string name;

    json_builder.StartDict()
      .Key("request_id").Value(map_req.at("id"s).AsInt());

    if (type_stop == "Stop"s) {
      name = map_req.at("name"s).AsString();
      PrintStops(handler, name, json_builder);
    } else if (type_stop == "Bus"s) {
      name = map_req.at("name"s).AsString();
      PrintBuses(handler, name, json_builder);
    } else if (type_stop == "Map"s) {
      PrintMap(handler, json_builder);
    }

    json_builder.EndDict();
  }
  json_builder.EndArray();

  json::Print(json::Document(json_builder.Build()), out);
}

} // namespace printer

namespace renderer {

namespace {
svg::Color ReadColor(const json::Node &json) {
  if (json.IsArray()) {
    const auto &arr = json.AsArray();

    if (arr.size() == 3) {  // Rgb
      return svg::Rgb(arr[0].AsInt(), arr[1].AsInt(), arr[2].AsInt());
    } else if (arr.size() == 4) {  // Rgba
      return svg::Rgba(arr[0].AsInt(), arr[1].AsInt(), arr[2].AsInt(),
        arr[3].AsDouble());
    }
  } else if (json.IsString()) {
    return json.AsString();
  }

  return svg::NoneColor;
}

svg::Point ReadPoint(const json::Array& json) {
  return {json[0].AsDouble(), json[1].AsDouble()};
}

std::vector<svg::Color> ReadColors(const json::Array &json) {
  std::vector<svg::Color> colors;
  colors.reserve(json.size());

  for (const auto &item : json) {
    colors.emplace_back(ReadColor(item));
  }

  return colors;
}

} // namespace

RenderSettings ReadRenderSettings(const json::Dict &settings) {
  using namespace std::string_literals;

  RenderSettings rs;
  rs.width = settings.at("width"s).AsDouble();
  rs.height = settings.at("height"s).AsDouble();
  rs.padding = settings.at("padding"s).AsDouble();
  rs.line_width = settings.at("line_width"s).AsDouble();
  rs.stop_radius = settings.at("stop_radius"s).AsDouble();
  rs.bus_label_font_size = settings.at("bus_label_font_size").AsInt();
  rs.bus_label_offset = ReadPoint(settings.at("bus_label_offset").AsArray());
  rs.stop_label_font_size = settings.at("stop_label_font_size"s).AsInt();
  rs.stop_label_offset = ReadPoint(settings.at("stop_label_offset"s).AsArray());
  rs.underlayer_color = ReadColor(settings.at("underlayer_color"s));
  rs.underlayer_width = settings.at("underlayer_width"s).AsDouble();
  rs.color_palette = ReadColors(settings.at("color_palette"s).AsArray());

  return rs;
}

} // namespace renderer

} // namespace tc
