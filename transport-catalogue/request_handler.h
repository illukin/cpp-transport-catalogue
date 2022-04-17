#include "transport_catalogue.h"

#include <optional>

namespace svg {
class Document;
} // namespace svg

namespace tc {

namespace renderer {
class MapRenderer;
} // namespace renderer

class RequestHandler {
public:
  RequestHandler(const TransportCatalogue& db,
    const renderer::MapRenderer &renderer);

  // Возвращает информацию о маршруте (запрос Bus)
  [[nodiscard]] std::optional<BusInfo>
  GetBusInfo(const std::string_view& bus_name) const;

  // Возвращает маршруты, проходящие через остановку (запрос Stop)
  [[nodiscard]] const Buses *
  GetBusesByStop(const std::string_view& stop_name) const;

  // Рендерит карту маршрутов
  [[nodiscard]] svg::Document RenderMap() const;

private:
  const TransportCatalogue& db_;
  const renderer::MapRenderer& renderer_;
};

}  // namespace tc
