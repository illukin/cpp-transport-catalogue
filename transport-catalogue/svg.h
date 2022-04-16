#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace svg {

struct Rgb {
  Rgb() = default;
  Rgb(uint8_t r, uint8_t g, uint8_t b);

  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;
};

struct Rgba {
  Rgba() = default;
  Rgba(uint8_t r, uint8_t g, uint8_t b, double o);

  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;
  double opacity = 1.0;
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
inline const Color NoneColor{};

enum class StrokeLineCap {
  BUTT,
  ROUND,
  SQUARE,
};

enum class StrokeLineJoin {
  ARCS,
  BEVEL,
  MITER,
  MITER_CLIP,
  ROUND,
};

struct Point {
  Point() = default;
  Point(double x, double y);
  double x = 0;
  double y = 0;
};

std::ostream &operator<<(std::ostream &out, StrokeLineCap line_cap);
std::ostream &operator<<(std::ostream &out, StrokeLineJoin line_join);
std::ostream &operator<<(std::ostream &out, const Color &color);
std::ostream &operator<<(std::ostream &out, std::monostate);
std::ostream &operator<<(std::ostream &out, Rgb &rgb);
std::ostream &operator<<(std::ostream &out, Rgba &rgba);

template <typename T>
void RenderAttr(std::ostream &out, std::string_view key, T val);

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
  RenderContext(std::ostream& out);
  RenderContext(std::ostream& out, int indent_step, int indent);

  [[nodiscard]] RenderContext Indented() const;
  void RenderIndent() const;

  std::ostream& out;
  int indent_step = 0;
  int indent = 0;
};

/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object {
public:
  void Render(const RenderContext& context) const;

  virtual ~Object() = default;

private:
  virtual void RenderObject(const RenderContext& context) const = 0;
};

class ObjectContainer {
public:
  virtual ~ObjectContainer() = default;

  template <typename Obj>
  void Add(Obj obj);

  virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
};

class Drawable {
public:
  virtual ~Drawable() = default;

  virtual void Draw(ObjectContainer &container) const = 0;
};

template <typename Owner>
class PathProps {
public:
  Owner &SetFillColor(Color color);
  Owner &SetStrokeColor(Color color);
  Owner &SetStrokeWidth(double width);
  Owner &SetStrokeLineCap(StrokeLineCap stroke_line_cap);
  Owner &SetStrokeLineJoin(StrokeLineJoin stroke_line_join);

protected:
  ~PathProps() = default;

  void RenderAttrs(std::ostream &out) const;

private:
  Owner &AsOwner() {
    return static_cast<Owner &>(*this);
  }

  std::optional<Color> fill_color_;
  std::optional<Color> stroke_color_;
  std::optional<double> stroke_width_;
  std::optional<StrokeLineCap> stroke_line_cap_;
  std::optional<StrokeLineJoin> stroke_line_join_;
};

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle> {
public:
  Circle& SetCenter(Point center);
  Circle& SetRadius(double radius);

private:
  void RenderObject(const RenderContext &context) const override;

  Point center_;
  double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline> {
public:
  // Добавляет очередную вершину к ломаной линии
  Polyline& AddPoint(Point point);

private:
  void RenderObject(const RenderContext &context) const override;

  std::vector<std::string> points_;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object, public PathProps<Text> {
public:
  // Задаёт координаты опорной точки (атрибуты x и y)
  Text& SetPosition(Point pos);

  // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
  Text& SetOffset(Point offset);

  // Задаёт размеры шрифта (атрибут font-size)
  Text& SetFontSize(uint32_t size);

  // Задаёт название шрифта (атрибут font-family)
  Text& SetFontFamily(std::string font_family);

  // Задаёт толщину шрифта (атрибут font-weight)
  Text& SetFontWeight(std::string font_weight);

  // Задаёт текстовое содержимое объекта (отображается внутри тега text)
  Text& SetData(std::string data);

private:
  void RenderObject(const RenderContext &context) const override;

  Point base_point_;
  Point offset_;
  uint32_t font_size_ = 1;
  std::optional<std::string> font_weight_;
  std::optional<std::string> font_family_;
  std::string data_;
};

class Document : public ObjectContainer {
public:
  // Добавляет в svg-документ объект-наследник svg::Object
  void AddPtr(std::unique_ptr<Object>&& obj) override;

  // Выводит в ostream svg-представление документа
  void Render(std::ostream& out) const;

private:
  std::vector<std::unique_ptr<Object>> objects_;
};

template <typename Obj>
void ObjectContainer::Add(Obj obj) {
  AddPtr(std::make_unique<Obj>(std::move(obj)));
}

template <typename Owner>
Owner &PathProps<Owner>::SetFillColor(Color color) {
  fill_color_ = std::move(color);
  return AsOwner();
}

template <typename Owner>
Owner &PathProps<Owner>::SetStrokeColor(Color color) {
  stroke_color_ = std::move(color);
  return AsOwner();
}

template <typename Owner>
Owner &PathProps<Owner>::SetStrokeWidth(double width) {
  stroke_width_ = width;
  return AsOwner();
}

template <typename Owner>
Owner &PathProps<Owner>::SetStrokeLineCap(StrokeLineCap stroke_line_cap) {
  stroke_line_cap_ = stroke_line_cap;
  return AsOwner();
}

template <typename Owner>
Owner &PathProps<Owner>::SetStrokeLineJoin(StrokeLineJoin stroke_line_join) {
  stroke_line_join_ = stroke_line_join;
  return AsOwner();
}

template <typename Owner>
void PathProps<Owner>::RenderAttrs(std::ostream &out) const {
  using namespace std::string_view_literals;

  if (fill_color_.has_value()) {
    RenderAttr(out, " fill"sv, fill_color_.value());
  }

  if (stroke_color_.has_value()) {
    RenderAttr(out, " stroke"sv, stroke_color_.value());
  }

  if (stroke_width_.has_value()) {
    RenderAttr(out, " stroke-width"sv, stroke_width_.value());
  }

  if (stroke_line_cap_.has_value()) {
    RenderAttr(out, " stroke-linecap"sv, stroke_line_cap_.value());
  }

  if (stroke_line_join_.has_value()) {
    RenderAttr(out, " stroke-linejoin"sv, stroke_line_join_.value());
  }
}

}  // namespace svg
