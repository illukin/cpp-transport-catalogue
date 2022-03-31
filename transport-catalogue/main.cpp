#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"

template <typename Func>
void CinProccess(tc::TransportCatalogue &cat, Func &func) {
  const size_t count = tc::filler::ReadLineWithNumber();
  std::vector<std::string> data = tc::filler::ReadLines(count);

  func(cat, data);
}

int main () {
  tc::TransportCatalogue cat;

  CinProccess(cat, tc::filler::FillDB);
  CinProccess(cat, tc::printer::ProcessQueries);

  return 0;
}
