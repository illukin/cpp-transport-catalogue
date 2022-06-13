#include "serialization.h"

#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <fstream>
#include <variant>

#include <transport_catalogue.pb.h>

namespace tc::serial {

template <typename M>
static void SerializeColor(const std::monostate &color, M *mut_field_rs ) {}

template <typename M>
static void SerializeColor(const std::string &color, M *mut_field_rs ) {
  mut_field_rs->mutable_str()->set_value(color);
}

template <typename M>
static void SerializeColor(const svg::Rgb &color, M *mut_field_rs) {
  auto mutable_rgb = mut_field_rs->mutable_rgb();

  mutable_rgb->set_red(color.red);
  mutable_rgb->set_green(color.green);
  mutable_rgb->set_blue(color.blue);
}

static void DeserializeColor(svg::Color &color,
    const transport_catalogue::Color &s_field_rs) {
  using std::string_literals::operator""s;

  if (s_field_rs.has_str()) {
    color = s_field_rs.str().value();
  } else if (s_field_rs.has_rgb()) {
    svg::Rgb rgb = {
      static_cast<uint8_t>(s_field_rs.rgb().red()),
      static_cast<uint8_t>(s_field_rs.rgb().green()),
      static_cast<uint8_t>(s_field_rs.rgb().blue()),
    };
    color = rgb;
  } else if (s_field_rs.has_rgba()) {
    svg::Rgba rgba = {
      static_cast<uint8_t>(s_field_rs.rgba().rgb().red()),
      static_cast<uint8_t>(s_field_rs.rgba().rgb().green()),
      static_cast<uint8_t>(s_field_rs.rgba().rgb().blue()),
      s_field_rs.rgba().opacity()
    };
    color = rgba;
  } else {
    color = ""s; // имитация пустого значения std::monostate
  }
}

template <typename M>
static void SerializeColor(const svg::Rgba &color, M *mut_field_rs) {
  auto mutable_rgba = mut_field_rs->mutable_rgba();

  mutable_rgba->mutable_rgb()->set_red(color.red);
  mutable_rgba->mutable_rgb()->set_green(color.green);
  mutable_rgba->mutable_rgb()->set_blue(color.blue);
  mutable_rgba->set_opacity(color.opacity);
}

static void SerializeStops(const TransportCatalogue &cat,
    transport_catalogue::TransportCatalogue &serial) {
  for (const auto &stop : cat.GetStops()) {
    auto s_stop = serial.add_stop();

    s_stop->set_name(stop.name);
    s_stop->set_latitude(stop.lat);
    s_stop->set_longitude(stop.lng);

    for (const auto &[stops, d] : cat.GetDistances()) {
      if (stop.name != stops.first->name) {
        continue;
      }

      auto s_dist = s_stop->add_road_distances();
      s_dist->set_name(stops.second->name);
      s_dist->set_distance(d);
    }
  }
}

static void SerializeBuses(const TransportCatalogue &cat,
    transport_catalogue::TransportCatalogue &serial) {
  for (const auto &bus : cat.GetBuses()) {
    auto s_bus = serial.add_bus();
    auto stops_unique_count = bus.stops.size();
    auto final_stops_count = bus.final_stops.size();

    s_bus->set_name(bus.name);

    // Определение кольцевого маршрута
    if (final_stops_count == 1) {
      s_bus->set_is_roundtrip(true);
    } else {
      s_bus->set_is_roundtrip(false);

      // В случае, когда маршрут НЕ кольцевой, обратный путь не учитывается
      stops_unique_count = (stops_unique_count / final_stops_count)
        + (stops_unique_count % final_stops_count);
    }

    for (int i = 0; i < stops_unique_count; ++i) {
      s_bus->add_stop(bus.stops[i]->name);
    }
  }
}
static void SerializeRenderSettings(const renderer::RenderSettings &rs,
    transport_catalogue::TransportCatalogue &serial) {
  auto mutable_rs = serial.mutable_render_settings();

  mutable_rs->set_width(rs.width);
  mutable_rs->set_height(rs.height);
  mutable_rs->set_padding(rs.padding);
  mutable_rs->set_line_width(rs.line_width);
  mutable_rs->set_stop_radius(rs.stop_radius);
  mutable_rs->set_bus_label_font_size(rs.bus_label_font_size);
  mutable_rs->mutable_bus_label_offset()->set_x(rs.bus_label_offset.x);
  mutable_rs->mutable_bus_label_offset()->set_y(rs.bus_label_offset.y);
  mutable_rs->set_stop_label_font_size(rs.stop_label_font_size);
  mutable_rs->mutable_stop_label_offset()->set_x(rs.stop_label_offset.x);
  mutable_rs->mutable_stop_label_offset()->set_y(rs.stop_label_offset.y);
  mutable_rs->set_underlayer_width(rs.underlayer_width);
  mutable_rs->set_stop_label_font_family(rs.stop_label_font_family);
  mutable_rs->set_bus_label_font_family(rs.bus_label_font_family);

  std::visit(
    [&mutable_rs](const auto &color) {
      SerializeColor(color, mutable_rs->mutable_underlayer_color());
    },
    rs.underlayer_color);

  std::visit(
    [&mutable_rs](const auto &color) {
      SerializeColor(color, mutable_rs->mutable_stop_label_color());
    },
    rs.stop_label_color);

  for (const auto &color : rs.color_palette) {
    auto m_cp = mutable_rs->add_color_palette();

    std::visit(
      [&m_cp](const auto &color) {
        SerializeColor(color, m_cp);
      },
      color);
  }
}

static void SerializeRoutingSettings(const router::RoutingSettings &ros,
    transport_catalogue::TransportCatalogue &serial) {
  auto mutable_ros = serial.mutable_router()->mutable_routing_settings();

  mutable_ros->set_bus_wait_time(ros.bus_wait_time.count());
  mutable_ros->set_bus_velocity(ros.bus_velocity);
}

static void
SerializeGraph(const graph::DirectedWeightedGraph<router::Minutes> &graph,
    transport_catalogue::TransportCatalogue &serial) {
  auto mutable_graph = serial.mutable_router()->mutable_graph();

  for (size_t i = 0; i < graph.GetEdgeCount(); ++i) {
    auto s_edge = mutable_graph->add_edge();
    auto cur_edge = graph.GetEdge(i);

    s_edge->set_from(cur_edge.from);
    s_edge->set_to(cur_edge.to);
    s_edge->set_weight(cur_edge.weight.count());
  }

  for (size_t i = 0; i < graph.GetVertexCount(); ++i) {
    auto s_ilist = mutable_graph->add_incidence_list();
    auto edges_range = graph.GetIncidentEdges(i);

    for (const auto &edge_id : edges_range) {
      s_ilist->add_edge_id(edge_id);
    }
  }
}

static void
SerializeStopVertexIds(const std::unordered_map<const Stop *,
  router::TransportRouter::StopVertexIds, Hasher> &stops_vertex_ids,
    transport_catalogue::TransportCatalogue &serial) {
  for (const auto &[key, val] : stops_vertex_ids) {
    auto s_svid = serial.mutable_router()->add_stops_vertex_id();

    s_svid->set_stop_name(key->name);
    s_svid->mutable_id()->set_in(val.in);
    s_svid->mutable_id()->set_out(val.out);
  }
}

static void
SerializeVertexes(const std::vector<const Stop *> &vertexes,
    transport_catalogue::TransportCatalogue &serial) {
  for (const auto &v : vertexes) {
    serial.mutable_router()->add_vertex()->set_stop_name(v->name);
  }
}

static void
SerializeEdges(const std::vector<router::TransportRouter::EdgeInfo> &edges,
    transport_catalogue::TransportCatalogue &serial) {
  for (const auto &edge : edges) {
    auto s_edge = serial.mutable_router()->add_edge();

    if (edge.has_value()) {
      s_edge->mutable_bus_edge()->set_bus_name(edge->bus->name);
      s_edge->mutable_bus_edge()->set_span_count(edge->span_count);
    } else {
      s_edge->set_nullopt(true);
    }
  }
}


static void DeserializeStops(TransportCatalogue &cat,
    const transport_catalogue::TransportCatalogue &serial) {
  std::map<std::string, std::map<std::string, int>> distances;

  for (int i = 0; i < serial.stop_size(); ++i) {
    Stop stop;

    stop.name = serial.stop(i).name();
    stop.lat = serial.stop(i).latitude();
    stop.lng = serial.stop(i).longitude();
    cat.AddStop(stop);

    for (int j = 0; j < serial.stop(i).road_distances_size(); ++j) {
      distances[stop.name][serial.stop(i).road_distances(j).name()]
        = static_cast<int>(serial.stop(i).road_distances(j).distance());
    }
  }

  for (const auto &[stop_a_name, map_distances] : distances) {
    for (const auto &[stop_b_name, dist] : map_distances) {
      auto a = cat.GetStop(stop_a_name);
      auto b = cat.GetStop(stop_b_name);
      auto stops = std::make_pair(a, b);

      cat.SetDistance(stops, dist);
    }
  }
}

static void DeserializeBuses(TransportCatalogue &cat,
    const transport_catalogue::TransportCatalogue &serial) {
  for (int i = 0; i < serial.bus_size(); ++i) {
    Route route;
    Route final_stops;

    for (int j = 0; j < serial.bus(i).stop_size(); ++j) {
      route.push_back(cat.GetStop(serial.bus(i).stop(j)));
    }

    // Определение конечных остановок
    final_stops.push_back(route.front());
    if (route.front() != route.back()) {
      final_stops.push_back(route.back());
    }

    // Приведение маршрута вида "stop1 - stop2 - ... stopN" к виду
    // "stop1 > stop2 > ... > stopN-1 > stopN > stopN-1 > ... > stop2 > stop1"
    if (!serial.bus(i).is_roundtrip()) {
      route.insert(route.end(), route.rbegin() + 1, route.rend());
    }

    cat.AddRoute({serial.bus(i).name(), route, final_stops});
  }
}

static void DeserializeRenderSettings(renderer::RenderSettings &rs,
    const transport_catalogue::TransportCatalogue &serial) {
  using std::string_literals::operator""s;
  const auto &s_rs = serial.render_settings();

  rs.width = s_rs.width();
  rs.height = s_rs.height();
  rs.padding = s_rs.padding();
  rs.line_width = s_rs.line_width();
  rs.stop_radius = s_rs.stop_radius();
  rs.bus_label_font_size = s_rs.bus_label_font_size();
  rs.bus_label_offset = {s_rs.bus_label_offset().x(),
    s_rs.bus_label_offset().y()};
  rs.stop_label_font_size = s_rs.stop_label_font_size();
  rs.stop_label_offset = {s_rs.stop_label_offset().x(),
    s_rs.stop_label_offset().y()};
  rs.underlayer_width = s_rs.underlayer_width();
  rs.stop_label_font_family = s_rs.stop_label_font_family();
  rs.bus_label_font_family = s_rs.bus_label_font_family();

  DeserializeColor(rs.underlayer_color,
    serial.render_settings().underlayer_color());
  DeserializeColor(rs.stop_label_color,
    serial.render_settings().stop_label_color());

  for (int i = 0; i < s_rs.color_palette_size(); ++i) {
    svg::Color color;

    DeserializeColor(color, serial.render_settings().color_palette(i));
    rs.color_palette.push_back(color);
  }
}

static void DeserializeRoutingSettings(router::RoutingSettings &ros,
    const transport_catalogue::TransportCatalogue &serial) {
  std::chrono::minutes m{serial.router().routing_settings().bus_wait_time()};
  ros.bus_wait_time = m;
  ros.bus_velocity = serial.router().routing_settings().bus_velocity();
}

static void
DeserializeGraph(graph::DirectedWeightedGraph<router::Minutes> &graph,
    const transport_catalogue::TransportCatalogue &serial) {
  auto s_graph = serial.router().graph();
  auto &edges = graph.GetEdges();
  auto &incidence_lists = graph.GetIncidenceList();

  edges.clear();
  for (int i = 0; i < s_graph.edge_size(); ++i) {
    graph::Edge<router::Minutes> edge = {
      s_graph.edge(i).from(),
      s_graph.edge(i).to(),
      static_cast<router::Minutes>(s_graph.edge(i).weight())};
    edges.push_back(edge);
  }

  incidence_lists.resize(s_graph.incidence_list_size());
  for (int i = 0; i < s_graph.incidence_list_size(); ++i) {
    for (int j = 0; j < s_graph.incidence_list(i).edge_id_size(); ++j) {
      incidence_lists[i].push_back(s_graph.incidence_list(i).edge_id(j));
    }
  }
}

static void
DeserializeStopVertexIds(const TransportCatalogue &cat,
  std::unordered_map<const Stop *, router::TransportRouter::StopVertexIds,
    Hasher> &stops_vertex_ids,
    const transport_catalogue::TransportCatalogue &serial) {
  for (int i = 0; i < serial.router().stops_vertex_id_size(); ++i) {
    auto s_svid = serial.router().stops_vertex_id(i);

    router::TransportRouter::StopVertexIds svid = {
      s_svid.id().in(),
      s_svid.id().out()
    };
    stops_vertex_ids[cat.GetStop(s_svid.stop_name())] = svid;
  }
}

static void
DeserializeVertexes(const TransportCatalogue &cat,
    std::vector<const Stop *> &vertexes,
    const transport_catalogue::TransportCatalogue &serial) {
  for (int i = 0; i < serial.router().vertex_size(); ++i) {
    vertexes.push_back(cat.GetStop(serial.router().vertex(i).stop_name()));
  }
}

static void
DeserializeEdges(const TransportCatalogue &cat,
    std::vector<router::TransportRouter::EdgeInfo> &edges,
    const transport_catalogue::TransportCatalogue &serial) {
  for (int i = 0; i < serial.router().edge_size(); ++i) {
    auto s_edge = serial.router().edge(i);

    if (s_edge.has_bus_edge()) {
      router::TransportRouter::BusEdge bus_edge = {
        cat.GetBus(s_edge.bus_edge().bus_name()),
        s_edge.bus_edge().span_count()
      };

      edges.emplace_back(bus_edge);
    } else {
      edges.emplace_back(std::nullopt);
    }
  }
}

static void SerializeRoute(const router::TransportRouter &router,
    transport_catalogue::TransportCatalogue &serial) {
  SerializeRoutingSettings(router.GetRoutingSettings(), serial);
  SerializeGraph(router.GetGraph(), serial);
  SerializeStopVertexIds(router.GetStopsVertexIds(), serial);
  SerializeVertexes(router.GetVertexes(), serial);
  SerializeEdges(router.GetEdges(), serial);
}

static void DeserializeRoute(const TransportCatalogue &cat,
    router::TransportRouter &router,
    const transport_catalogue::TransportCatalogue &serial) {
  DeserializeRoutingSettings(router.GetRoutingSettings(), serial);
  DeserializeGraph(router.GetGraph(), serial);
  DeserializeStopVertexIds(cat, router.GetStopsVertexIds(), serial);
  DeserializeVertexes(cat, router.GetVertexes(), serial);
  DeserializeEdges(cat, router.GetEdges(), serial);

  // При создании пустого объекта TransportRouter для последующей десереализации
  // его полей, указатель на Router, по умолчанию, равен nullptr. Необходимо
  // обновить указатель на объект класса Router, конструктор которого принимает
  // граф. Граф десереализуется после того, как пустой объект TransportRouter
  // сконструирован, поэтому и необходимо обновить указатель.
  router.UpdateRouterPtr();
}

void SerializeDB(const TransportCatalogue &cat,
    const renderer::RenderSettings &rs,
    const router::TransportRouter &router,
    const std::filesystem::path &path) {
  std::ofstream ofile(path, std::ios::binary);
  transport_catalogue::TransportCatalogue s_tc;

  SerializeStops(cat, s_tc);
  SerializeBuses(cat, s_tc);
  SerializeRenderSettings(rs, s_tc);
  SerializeRoute(router, s_tc);

  s_tc.SerializeToOstream(&ofile);
}

void DeserializeDB(TransportCatalogue &cat, renderer::RenderSettings &rs,
    router::TransportRouter &router, const std::filesystem::path &path) {
  std::ifstream ifile(path, std::ios::binary);
  transport_catalogue::TransportCatalogue s_tc;
  s_tc.ParseFromIstream(&ifile);

  DeserializeStops(cat, s_tc);
  DeserializeBuses(cat, s_tc);
  DeserializeRenderSettings(rs, s_tc);
  DeserializeRoute(cat, router, s_tc);
}

} // namespace tc::serial
