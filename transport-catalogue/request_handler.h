#include "transport_catalogue.h"

#include <optional>
#include <string_view>
#include <unordered_set>

namespace svg {
class Document;
} // namespace svg

namespace tc {

namespace renderer {
class MapRenderer;
} // namespace renderer

namespace router {
struct RouteInfo;
class TransportRouter;
} // namespace router

class RequestHandler {
public:
  RequestHandler(const TransportCatalogue& db,
    const renderer::MapRenderer &renderer,
    const router::TransportRouter &router);

  // Возвращает информацию о маршруте (запрос Bus)
  [[nodiscard]] std::optional<BusInfo>
  GetBusInfo(const std::string_view &bus_name) const;

  // Возвращает маршруты, проходящие через остановку (запрос Stop)
  [[nodiscard]] const Buses *
  GetBusesByStop(const std::string_view &stop_name) const;

  // Рендерит карту маршрутов
  [[nodiscard]] svg::Document RenderMap() const;

  // Возвращает описание маршрута
  [[nodiscard]] std::optional<router::RouteInfo>
  FindRoute(std::string_view stop_from, std::string_view stop_to) const;


private:
  const TransportCatalogue &db_;
  const renderer::MapRenderer &renderer_;
  const router::TransportRouter &router_;
};

}  // namespace tc
