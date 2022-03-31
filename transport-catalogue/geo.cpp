#include "geo.h"

namespace tc::geo {

bool Coordinates::operator==(const Coordinates& other) const {
  return lat == other.lat && lng == other.lng;
}

bool Coordinates::operator!=(const Coordinates& other) const {
  return !(*this == other);
}

} // namespace tc::geo
