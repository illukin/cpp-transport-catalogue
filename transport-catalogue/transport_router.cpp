#include "transport_catalogue.h"
#include "transport_router.h"

namespace tc::router {

Router::Router(RoutingSettings settings, const TransportCatalogue &cat)
  : settings_(settings) {
  const auto &stops = cat.GetStops();
  const size_t vertex_count = stops.size() * 2;  // По две вершины на остановку
  graph_ = graph::DirectedWeightedGraph<Minutes>(vertex_count);
  vertexes_.reserve(vertex_count);

  AddStopsToGraph(cat);
  AddBusesToGraph(cat);

  router_ = std::make_unique<graph::Router<Minutes>>(graph_);
}

std::optional<RouteInfo>
Router::FindRoute(const Stop *from, const Stop *to) const {
  const graph::VertexId vertex_from = stops_vertex_ids_.at(from).out;
  const graph::VertexId vertex_to = stops_vertex_ids_.at(to).out;
  const auto route = router_->BuildRoute(vertex_from, vertex_to);

  if (!route) {
    return std::nullopt;
  }

  RouteInfo route_info;
  route_info.total_time = route->weight;
  route_info.items.reserve(route->edges.size());

  for (const auto edge_id : route->edges) {
    const auto &edge = graph_.GetEdge(edge_id);
    const auto &bus_edge_info = edges_[edge_id];

    if (bus_edge_info.has_value()) {
      route_info.items.emplace_back(RouteInfo::BusItem{
        bus_edge_info->bus,
        edge.weight,
        bus_edge_info->span_count,
      });
    } else {
      const graph::VertexId vertex_id = edge.from;
      route_info.items.emplace_back(RouteInfo::WaitItem{
        vertexes_[vertex_id],
        edge.weight,
      });
    }
  }
  return route_info;
}

void Router::AddStopsToGraph(const TransportCatalogue &cat) {
  graph::VertexId vertex_id = 0;
  const auto &stops = cat.GetStops();

  for (const auto &stop : stops) {
    auto &vertex_ids = stops_vertex_ids_[&stop];

    vertex_ids.in = vertex_id++;
    vertex_ids.out = vertex_id++;
    vertexes_[vertex_ids.in] = &stop;
    vertexes_[vertex_ids.out] = &stop;

    edges_.emplace_back(std::nullopt);
    graph_.AddEdge({
      vertex_ids.out,
      vertex_ids.in,
      Minutes(settings_.bus_wait_time.count())
    });
  }
}

void Router::AddBusesToGraph(const TransportCatalogue &cat) {
  const auto &buses = cat.GetBuses();

  for (const auto &bus : buses) {
    const auto &bus_stops = bus.stops;
    const size_t stop_count = bus_stops.size();

    if (stop_count <= 1) {
      continue;
    }

    auto compute_distance_from = [&cat, &bus_stops](size_t stop_idx) {
      return cat.GetDistance(bus_stops[stop_idx], bus_stops[stop_idx + 1]);
    };

    for (size_t begin_i = 0; begin_i + 1 < stop_count; ++begin_i) {
      const graph::VertexId start = stops_vertex_ids_.at(bus_stops[begin_i]).in;
      size_t total_distance = 0;

      for (size_t end_i = begin_i + 1; end_i < stop_count; ++end_i) {
        total_distance += compute_distance_from(end_i - 1);
        edges_.emplace_back(BusEdge{
          &bus,
          end_i - begin_i,
        });

        graph_.AddEdge({
          start,
          stops_vertex_ids_.at(bus_stops[end_i]).out,
          Minutes(
            static_cast<double>(total_distance)
              / (settings_.bus_velocity * 1000.0 / 60)
          )
        });
      }
    }
  }
}

} // namespace tc::router
