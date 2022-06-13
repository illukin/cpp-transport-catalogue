#include "transport_catalogue.h"
#include "transport_router.h"

namespace tc::router {

TransportRouter::TransportRouter(RoutingSettings settings, const TransportCatalogue &cat)
  : settings_(settings) {
  const auto &stops = cat.GetStops();
  const size_t vertex_count = stops.size() * 2;  // По две вершины на остановку
  graph_ = graph::DirectedWeightedGraph<Minutes>(vertex_count);
  vertexes_.resize(vertex_count);

  AddStopsToGraph(cat);
  AddBusesToGraph(cat);

  router_ = std::make_unique<graph::Router<Minutes>>(graph_);
}

void TransportRouter::UpdateRouterPtr() {
  router_ = std::make_unique<graph::Router<Minutes>>(graph_);
}

const graph::DirectedWeightedGraph<Minutes> &TransportRouter::GetGraph() const {
  return graph_;
}

graph::DirectedWeightedGraph<Minutes> &TransportRouter::GetGraph() {
  return graph_;
}

const RoutingSettings &TransportRouter::GetRoutingSettings() const {
  return settings_;
}

RoutingSettings &TransportRouter::GetRoutingSettings() {
  return settings_;
}

const std::unordered_map<const Stop *, TransportRouter::StopVertexIds, Hasher> &
TransportRouter::GetStopsVertexIds() const {
  return stops_vertex_ids_;
}

std::unordered_map<const Stop *, TransportRouter::StopVertexIds, Hasher> &
TransportRouter::GetStopsVertexIds() {
  return stops_vertex_ids_;
}

const std::vector<const Stop *> &TransportRouter::GetVertexes() const {
  return vertexes_;
}

std::vector<const Stop *> &TransportRouter::GetVertexes() {
  return vertexes_;
}

const std::vector<TransportRouter::EdgeInfo> &TransportRouter::GetEdges() const {
  return edges_;
}

std::vector<TransportRouter::EdgeInfo> &TransportRouter::GetEdges() {
  return edges_;
}

std::optional<RouteInfo>
TransportRouter::FindRoute(const Stop *from, const Stop *to) const {
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

void TransportRouter::AddStopsToGraph(const TransportCatalogue &cat) {
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

void TransportRouter::AddBusesToGraph(const TransportCatalogue &cat) {
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
