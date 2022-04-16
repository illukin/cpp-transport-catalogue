#pragma once

#include "domain.h"
#include "svg.h"

#include <cassert>
#include <map>

namespace tc::renderer {

struct RenderSettings {
  double width = 0;
  double height = 0;
  double padding = 0;
  double line_width = 0;
  double stop_radius = 0;
  int bus_label_font_size = 0;
  svg::Point bus_label_offset{};
  int stop_label_font_size = 0;
  svg::Point stop_label_offset{};
  svg::Color underlayer_color{std::string("white")};
  double underlayer_width = 0;
  std::vector<svg::Color> color_palette;
  std::string stop_label_font_family{std::string("Verdana")};
  std::string bus_label_font_family{std::string("Verdana")};
  svg::Color stop_label_color{std::string("black")};
};

class Map : public svg::Drawable {
public:
  Map(RenderSettings settings, std::vector<Bus *> buses);

  void Draw(svg::ObjectContainer &container) const override;

private:
  void RenderBusLines(svg::ObjectContainer &) const;
  void RenderBusLabels(svg::ObjectContainer &) const;
  void RenderStops(svg::ObjectContainer &) const;
  void RenderStopLabels(svg::ObjectContainer &) const;

  const svg::Color &GetBusLineColor(size_t index) const;

  struct LexicSorterByName {
    bool operator()(Stop *lhs, Stop *rhs) const;
  };

  RenderSettings settings_;
  std::vector<Bus *> buses_;
  std::map<Stop *, svg::Point, LexicSorterByName> stops_positions_;
};


class MapRenderer {
public:
  MapRenderer() = default;
  MapRenderer(RenderSettings settings);

  template <typename Iterator>
  Map RenderMap(Iterator begin, Iterator end) const;

private:
  RenderSettings settings_;
};

template <typename Iterator>
Map MapRenderer::RenderMap(Iterator begin, Iterator end) const {
  std::vector<Bus *> buses;

  while (begin != end) {
    buses.emplace_back(&(*begin));
    ++begin;
  }

  return {settings_, std::move(buses)};
}

}  // namespace tc::renderer
