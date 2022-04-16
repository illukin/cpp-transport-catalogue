#include "svg.h"

#include <sstream>
#include <utility>

namespace svg {

using namespace std::literals;

void NormalizeStr(std::ostream &out, const std::string_view line) {
  for (const auto &c : line) {
    switch(c) {
      case '&': out << "&amp;"sv; break;
      case '<': out << "&lt;"sv; break;
      case '>': out << "&gt;"sv; break;
      case '\'':  // Для одинарной кавычки используется случай апострофа
      case '`': out << "&apos;"sv; break;
      case '"': out << "&quot;"sv; break;
      default: out << c; break;
    }
  }
}

template <typename T>
void RenderAttr(std::ostream &out, std::string_view key, T val) {
  out << key << R"(=")" << val << R"(")";
}

std::ostream &operator<<(std::ostream &out, const std::monostate) {
  out << "none"sv;

  return out;
}

std::ostream &operator<<(std::ostream &out, const Rgb &rgb) {
  out << "rgb("sv << static_cast<int>(rgb.red) << ','
    << static_cast<int>(rgb.green) << ','
    << static_cast<int>(rgb.blue) << ')';

  return out;
}

std::ostream &operator<<(std::ostream &out, const Rgba rgba) {
  out << "rgba("sv << static_cast<int>(rgba.red) << ','
    << static_cast<int>(rgba.green) << ','
    << static_cast<int>(rgba.blue) << ',' << rgba.opacity << ')';

  return out;
}

std::ostream &operator<<(std::ostream &out, StrokeLineCap line_cap) {
  using namespace std::string_view_literals;

  switch(line_cap) {
    case StrokeLineCap::BUTT: out << "butt"sv; break;
    case StrokeLineCap::ROUND: out << "round"sv; break;
    case StrokeLineCap::SQUARE: out << "square"sv; break;
    default: break; // Недостижимая ветка
  }

  return out;
}

std::ostream &operator<<(std::ostream &out, StrokeLineJoin line_join) {
  using namespace std::string_view_literals;

  switch(line_join) {
    case StrokeLineJoin::ARCS: out << "arcs"sv; break;
    case StrokeLineJoin::BEVEL: out << "bevel"sv; break;
    case StrokeLineJoin::MITER: out << "miter"sv; break;
    case StrokeLineJoin::MITER_CLIP: out << "miter-clip"sv; break;
    case StrokeLineJoin::ROUND: out << "round"sv; break;
    default: break; // Недостижимая ветка
  }

  return out;
}

std::ostream &operator<<(std::ostream &out, const Color &color) {
  std::visit([&out](const auto color) {
    out << color;
  }, color);

  return out;
}

Rgb::Rgb(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {}

Rgba::Rgba(uint8_t r, uint8_t g, uint8_t b, double o)
  : red(r), green(g), blue(b), opacity(o) {}

Point::Point(double x, double y) : x(x) , y(y) {}

RenderContext::RenderContext(std::ostream& out) : out(out) {}

RenderContext::RenderContext(std::ostream& out, int indent_step, int indent = 0)
  : out(out), indent_step(indent_step), indent(indent) {}

RenderContext RenderContext::Indented() const {
  return {out, indent_step, indent + indent_step};
}

void RenderContext::RenderIndent() const {
  for (int i = 0; i < indent; ++i) {
    out.put(' ');
  }
}

void Object::Render(const RenderContext &context) const {
  context.RenderIndent();
  RenderObject(context);
  context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
  center_ = center;
  return *this;
}

Circle& Circle::SetRadius(double radius)  {
  radius_ = radius;
  return *this;
}

void Circle::RenderObject(const RenderContext &context) const {
  auto &out = context.out;

  out << "<circle"sv;
  RenderAttr(out, " cx"sv, center_.x);
  RenderAttr(out, " cy"sv, center_.y);
  RenderAttr(out, " r"sv, radius_);
  RenderAttrs(out);
  out << "/>"sv;
}

// ---------- Polyline ------------------

Polyline &Polyline::AddPoint(Point point) {
  std::ostringstream out;

  out << point.x << "," << point.y;
  points_.emplace_back(out.str());

  return *this;
}

void Polyline::RenderObject(const RenderContext &context) const {
  auto &out = context.out;

  out << R"(<polyline points=")";
  auto is_first = true;
  for (const auto &point : points_) {
    if (is_first) {
      out << point;
      is_first = false;
    } else {
      out << ' ' << point;
    }
  }

  out << R"(" )";
  RenderAttrs(out);
  out << "/>"sv;
}

// ---------- Text ------------------

Text &Text::SetPosition(Point pos) {
  base_point_ = pos;
  return *this;
}

// Задаёт смещение относительно опорной точки (атрибуты dx, dy)
Text &Text::SetOffset(Point offset) {
  offset_ = offset;
  return *this;
}

// Задаёт размеры шрифта (атрибут font-size)
Text &Text::SetFontSize(uint32_t size) {
  font_size_ = size;
  return *this;
}

// Задаёт название шрифта (атрибут font-family)
Text &Text::SetFontFamily(std::string font_family) {
  font_family_ = std::move(font_family);
  return *this;
}

// Задаёт толщину шрифта (атрибут font-weight)
Text &Text::SetFontWeight(std::string font_weight) {
  font_weight_ = std::move(font_weight);
  return *this;
}

// Задаёт текстовое содержимое объекта (отображается внутри тега text)
Text &Text::SetData(std::string data) {
  data_ = std::move(data);
  return *this;
}

void Text::RenderObject(const RenderContext &context) const {
  auto &out = context.out;

  out << "<text"sv;
  RenderAttr(out, " x"sv, base_point_.x);
  RenderAttr(out, " y"sv, base_point_.y);
  RenderAttr(out, " dx"sv, offset_.x);
  RenderAttr(out, " dy"sv, offset_.y);
  RenderAttr(out, " font-size"sv, font_size_);

  if (font_family_.has_value()) {
    RenderAttr(out, " font-family"sv, font_family_.value());
  }

  if (font_weight_.has_value()) {
    RenderAttr(out, " font-weight"sv, font_weight_.value());
  }

  RenderAttrs(out);

  out << ">"sv;
  NormalizeStr(out, data_);
  out << "</text>"sv;
}

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
  objects_.emplace_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
  out << R"(<?xml version="1.0" encoding="UTF-8" ?>)" << std::endl
    << R"(<svg xmlns="http://www.w3.org/2000/svg" version="1.1">)" << std::endl;

  for(const auto &obj : objects_) {
    obj->Render(out);
  }

  out << "</svg>"sv << std::endl;
}

}  // namespace svg
