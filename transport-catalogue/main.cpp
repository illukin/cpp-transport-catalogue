#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"

#include <iostream>

int main () {
  tc::TransportCatalogue cat;

  // Заполнение базы данных
  size_t count = tc::reader::ReadLineWithNumber(std::cin);
  std::vector<std::string> data = tc::reader::ReadLines(count, std::cin);
  tc::filler::FillDB(cat, data);

  // Обработка запросов и печать результатов
  count = tc::reader::ReadLineWithNumber(std::cin);
  data = tc::reader::ReadLines(count, std::cin);
  tc::printer::ProcessQueries(cat, data, std::cout);

  return 0;
}
