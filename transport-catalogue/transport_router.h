#pragma once

#include "domain.h"
#include "graph.h"
#include "router.h"

#include <chrono>
#include <memory>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

class TransportCatalogue;

namespace tc::router {

struct RoutingSettings {
  std::chrono::minutes bus_wait_time{};
  double bus_velocity = 0;
};

using Minutes = std::chrono::duration<double, std::chrono::minutes::period>;

struct RouteInfo {
  Minutes total_time;

  struct WaitItem {
    const Stop *stop;
    Minutes time{};
  };

  struct BusItem {
    const Bus *bus;
    Minutes time{};
    size_t span_count = 0;
  };

  using Item = std::variant<BusItem, WaitItem>;
  std::vector<Item> items;
};

class TransportRouter {
public:
  struct StopVertexIds {
    graph::VertexId in;
    graph::VertexId out;
  };

  struct BusEdge {
    const Bus *bus;
    size_t span_count;
  };

  using EdgeInfo = std::optional<BusEdge>;

  TransportRouter() = default;
  TransportRouter(RoutingSettings settings, const TransportCatalogue &cat);

  std::optional<RouteInfo> FindRoute(const Stop *from, const Stop *to) const;

  void UpdateRouterPtr();

  const graph::DirectedWeightedGraph<Minutes> &GetGraph() const;
  graph::DirectedWeightedGraph<Minutes> &GetGraph();

  const RoutingSettings &GetRoutingSettings() const;
  RoutingSettings &GetRoutingSettings();

  const std::unordered_map<const Stop *, StopVertexIds, Hasher> &
  GetStopsVertexIds() const;
  std::unordered_map<const Stop *, StopVertexIds, Hasher> &
  GetStopsVertexIds();

  const std::vector<const Stop *> &GetVertexes() const;
  std::vector<const Stop *> &GetVertexes();

  const std::vector<EdgeInfo> &GetEdges() const;
  std::vector<EdgeInfo> &GetEdges();

private:
  void AddStopsToGraph(const TransportCatalogue &cat);
  void AddBusesToGraph(const TransportCatalogue &cat);

  RoutingSettings settings_;
  graph::DirectedWeightedGraph<Minutes> graph_;
  std::unique_ptr<graph::Router<Minutes>> router_;
  std::unordered_map<const Stop *, StopVertexIds, Hasher> stops_vertex_ids_;
  std::vector<const Stop *> vertexes_;
  std::vector<EdgeInfo> edges_;
};

}  // namespace tc::router
