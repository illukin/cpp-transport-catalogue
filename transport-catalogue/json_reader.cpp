#include "json_reader.h"

#include "json_builder.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <chrono>
#include <map>
#include <sstream>
#include <variant>

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

void BuildRouteItem(json::Builder &builder,
  const router::RouteInfo::BusItem &item) {
  using namespace std::string_literals;

  builder.StartDict()
    .Key("type"s).Value("Bus"s)
    .Key("bus"s).Value(item.bus->name)
    .Key("time"s).Value(item.time.count())
    .Key("span_count"s).Value(static_cast<int>(item.span_count))
    .EndDict();
}
void BuildRouteItem(json::Builder &builder,
  const router::RouteInfo::WaitItem &item) {
  using namespace std::string_literals;

  builder.StartDict()
    .Key("type"s).Value("Wait"s)
    .Key("stop_name"s).Value(item.stop->name)
    .Key("time"s).Value(item.time.count())
    .EndDict();
}

void PrintRoute(const RequestHandler &handler, std::string &from,
  std::string &to, json::Builder &builder) {
  using namespace std::string_literals;

  const auto route = handler.FindRoute(from, to);
  if (!route.has_value()) {
    builder.Key("error_message"s).Value("not found"s);
    return;
  }

  builder
    .Key("total_time"s).Value(route->total_time.count())
    .Key("items"s).StartArray();

  // Конструирования массива элементов, каждый из которых описывает непрерывную
  // активность пассажира, требующую временных затрат
  for (const auto &item : route->items) {
    std::visit(
      [&builder](const auto &item) {
        BuildRouteItem(builder, item);
      },
      item);
  }

  builder.EndArray();
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
    auto req_type = map_req.at("type"s);
    std::string name, from, to;

    json_builder.StartDict()
      .Key("request_id").Value(map_req.at("id"s).AsInt());

    if (req_type == "Stop"s) {
      name = map_req.at("name"s).AsString();
      PrintStops(handler, name, json_builder);
    } else if (req_type == "Bus"s) {
      name = map_req.at("name"s).AsString();
      PrintBuses(handler, name, json_builder);
    }else if (req_type == "Route"s) {
      from = map_req.at("from"s).AsString();
      to = map_req.at("to"s).AsString();
      PrintRoute(handler, from, to, json_builder);
    } else if (req_type == "Map"s) {
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

namespace router {

RoutingSettings ReadRoutingSettings(const json::Dict &settings) {
  using namespace std::string_literals;

  RoutingSettings rs;
  auto bus_wait_time = settings.at("bus_wait_time"s).AsInt();
  rs.bus_wait_time = std::chrono::minutes(bus_wait_time);
  rs.bus_velocity = settings.at("bus_velocity"s).AsDouble();

  return rs;
}

} // namespace router

} // namespace tc
