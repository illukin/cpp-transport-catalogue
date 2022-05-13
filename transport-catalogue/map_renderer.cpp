#include "geo.h"
#include "map_renderer.h"

#include <algorithm>
#include <unordered_set>

namespace tc::renderer {

namespace {

inline const double EPSILON = 1e-6;
bool IsZero(double value) {
  return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
  template<typename PointInputIt>
  SphereProjector(PointInputIt points_begin, PointInputIt points_end,
      double max_width, double max_height, double padding) : padding_(padding) {
    if (points_begin == points_end) {
      return;
    }

    const auto[left_it, right_it]
    = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
      return lhs.lng < rhs.lng;
    });
    min_lon_ = left_it->lng;
    const double max_lon = right_it->lng;

    const auto[bottom_it, top_it]
    = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
      return lhs.lat < rhs.lat;
    });
    const double min_lat = bottom_it->lat;
    max_lat_ = top_it->lat;

    std::optional<double> width_zoom;
    if (!IsZero(max_lon - min_lon_)) {
      width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
    }

    std::optional<double> height_zoom;
    if (!IsZero(max_lat_ - min_lat)) {
      height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
    }

    if (width_zoom && height_zoom) {
      zoom_coeff_ = std::min(*width_zoom, *height_zoom);
    } else if (width_zoom) {
      zoom_coeff_ = *width_zoom;
    } else if (height_zoom) {
      zoom_coeff_ = *height_zoom;
    }
  }

  svg::Point operator()(geo::Coordinates coords) const {
    return {(coords.lng - min_lon_) * zoom_coeff_ + padding_,
      (max_lat_ - coords.lat) * zoom_coeff_ + padding_};
  }

private:
  double padding_;
  double min_lon_ = 0;
  double max_lat_ = 0;
  double zoom_coeff_ = 0;
};

} // namespace

Map::Map(RenderSettings settings, std::vector<Bus *> buses)
    : settings_(std::move(settings)) , buses_(std::move(buses)) {
  std::sort(buses_.begin(), buses_.end(), [](Bus *lhs, Bus *rhs) {
    return lhs->name < rhs->name;
  });

  std::unordered_set<Stop *> stops;
  for (const auto bus : buses_) {
    stops.insert(bus->stops.begin(), bus->stops.end());
  }

  std::vector<geo::Coordinates> positions;
  positions.reserve(stops.size());
  for (const auto stop : stops) {
    positions.push_back({stop->lat, stop->lng});
  }

  SphereProjector projector{positions.begin(), positions.end(), settings_.width,
    settings_.height, settings_.padding};
  for (const auto stop : stops) {
    stops_positions_[stop] = projector({stop->lat, stop->lng});
  }
}

void Map::Draw(svg::ObjectContainer &container) const {
  RenderBusLines(container);
  RenderBusLabels(container);
  RenderStops(container);
  RenderStopLabels(container);
}

void Map::RenderBusLines(svg::ObjectContainer &container) const {
  size_t bus_index = 0;

  for (const auto bus : buses_) {
    auto &stops = bus->stops;

    if (!stops.empty()) {
      auto line = svg::Polyline()
        .SetStrokeColor(GetBusLineColor(bus_index++))
        .SetStrokeWidth(settings_.line_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetFillColor(svg::NoneColor)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

      for (const auto stop : stops) {
        line.AddPoint(stops_positions_.at(stop));
      }
      container.Add(std::move(line));
    }
  }
}

void Map::RenderBusLabels(svg::ObjectContainer &container) const {
  using namespace std::string_literals;

  size_t bus_index = 0;

  for (const auto bus : buses_) {
    if (bus->stops.empty()) {
      continue;
    }

    auto &bus_color = GetBusLineColor(bus_index++);

    for (const auto stop : bus->final_stops) {
      auto &stop_pos = stops_positions_.at(stop);
      auto base = svg::Text()
        .SetPosition(stop_pos)
        .SetOffset(settings_.bus_label_offset)
        .SetFontSize(settings_.bus_label_font_size)
        .SetFontFamily(settings_.bus_label_font_family)
        .SetFontWeight("bold"s)
        .SetData(bus->name);

      container.Add(svg::Text{base}
        .SetFillColor(settings_.underlayer_color)
        .SetStrokeColor(settings_.underlayer_color)
        .SetStrokeWidth(settings_.underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND));

      container.Add(svg::Text{base}.SetFillColor(bus_color));
    }
  }
}

void Map::RenderStops(svg::ObjectContainer &container) const {
  using namespace std::string_literals;

  for (const auto &[key, position] : stops_positions_) {
    container.Add(svg::Circle()
      .SetRadius(settings_.stop_radius)
      .SetCenter(position)
      .SetFillColor("white"s));
  }
}

void Map::RenderStopLabels(svg::ObjectContainer &container) const {
  for (const auto &[stop, stop_coord] : stops_positions_) {
    auto base = svg::Text()
      .SetPosition(stop_coord)
      .SetOffset(settings_.stop_label_offset)
      .SetFontSize(settings_.stop_label_font_size)
      .SetFontFamily(settings_.stop_label_font_family)
      .SetData(stop->name);
    container.Add(svg::Text{base}
      .SetFillColor(settings_.underlayer_color)
      .SetStrokeColor(settings_.underlayer_color)
      .SetStrokeWidth(settings_.underlayer_width)
      .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
      .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND));
    container.Add(svg::Text{base}.SetFillColor(settings_.stop_label_color));
  }
}

const svg::Color &Map::GetBusLineColor(size_t index) const {
  using namespace std::string_literals;

  static const svg::Color default_color = svg::Color{"black"s};
  const auto &palette = settings_.color_palette;

  return !palette.empty() ? palette[index % palette.size()] : default_color;
}

bool Map::LexicSorterByName::operator()(Stop *lhs, Stop *rhs) const {
  return lhs->name < rhs->name;
}

MapRenderer::MapRenderer(RenderSettings settings)
  : settings_(std::move(settings)) {
}

} // namespace tc::renderer
