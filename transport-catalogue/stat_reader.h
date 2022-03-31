#pragma once

#include "transport_catalogue.h"

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace tc::printer {

void ProcessQueries(const TransportCatalogue &cat,
  std::vector<std::string> &data, std::ostream &out);

} // namespace tc::printer
