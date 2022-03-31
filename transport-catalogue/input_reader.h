#pragma once

#include "transport_catalogue.h"

#include <string>
#include <vector>

namespace tc::filler {

std::string ReadLine();
size_t ReadLineWithNumber();
std::vector<std::string> ReadLines(size_t);

void FillDB(TransportCatalogue &, std::vector<std::string> &);

} // namespace tc::filler
