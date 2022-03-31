#pragma once

#include "transport_catalogue.h"

#include <cstddef>
#include <string>
#include <vector>

namespace tc::printer {

void ProcessQueries(const TransportCatalogue &, std::vector<std::string> &);

} // namespace tc::printer
