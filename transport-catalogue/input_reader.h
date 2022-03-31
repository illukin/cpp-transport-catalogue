#pragma once

#include "transport_catalogue.h"

#include <string>
#include <vector>

namespace tc {

namespace reader {

std::string ReadLine(std::istream &in);
size_t ReadLineWithNumber(std::istream &in);
std::vector<std::string> ReadLines(size_t, std::istream &in);

} // namespace reader

namespace filler {

void FillDB(TransportCatalogue &cat, std::vector<std::string> &data);

} // namespace filler

} // namespace tc
