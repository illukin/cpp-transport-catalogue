#include "input_reader.h"
#include "transport_catalogue.h"

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace tc {

namespace reader {

std::string ReadLine(std::istream &in) {
  std::string s;
  std::getline(in, s);
  return s;
}

size_t ReadLineWithNumber(std::istream &in) {
  size_t result;
  in >> result;
  ReadLine(in);
  return result;
}

std::vector<std::string> ReadLines(size_t count, std::istream &in) {
  std::vector<std::string> result;
  result.reserve(count);

  for (size_t i = 0; i < count; ++i) {
    // Сохранение всех запросов в векторе для последующего разбора необходимо
    // для того, чтобы корректно обработать случай, когда в маршруте фигурирует
    // остановка, объявленная после объявления этого маршрута.
    result.push_back(ReadLine(in));
  }

  return result;
}

} // namespace reader

namespace filler {

struct Query {
  std::string type;
  std::string name;
  std::string data;
};

Query ParseData(const std::string &line) {
  Query query;
  auto colon_pos = line.find(':');
  auto space_pos = line.find(' ');
  auto name_begin = line.find_first_not_of(' ', space_pos);

  if (colon_pos == std::string::npos
    || space_pos == std::string::npos
    || name_begin >= colon_pos) {
    return {};
  }

  query.type = line.substr(0, space_pos);
  query.name = line.substr(name_begin, colon_pos - name_begin);
  query.data = line.substr(colon_pos + 1);

  return query;
}

std::pair<double, double> ParseCoordinates(const std::string &line) {
  auto lat_begin = line.find_first_not_of(' ');
  auto comma_pos = line.find(',');
  auto lng_begin = line.find_first_not_of(' ', comma_pos + 1);

  double lat = std::stod(line.substr(lat_begin, comma_pos - lat_begin));
  double lng = std::stod(line.substr(lng_begin));

  return {lat, lng};
}

std::vector<std::string> SplitLine(const std::string &line, char delim) {
  std::vector<std::string> result;
  auto name_begin = line.find_first_not_of(' ');

  while (name_begin < line.size()) {
    auto delim_pos = line.find(delim, name_begin);

    if (delim_pos == std::string::npos) {
      delim_pos = line.size();
    }

    auto name_end = line.find_last_not_of(' ', delim_pos - 1);
    result.push_back(line.substr(name_begin, name_end - name_begin + 1));

    if (delim_pos == line.size()) {
      name_begin = delim_pos;
    } else {
      name_begin = line.find_first_not_of(' ', delim_pos + 1);
    }
  }

  return result;
}

std::vector<std::pair<int, std::string>> ParseDistances(std::string &data) {
  using namespace std::string_view_literals;
  auto delim = "m to "sv;
  std::vector<std::pair<int, std::string>> distances;
  auto parts = SplitLine(data, ',');

  for (auto &part : parts) {
    auto delim_pos = part.find(delim);

    if (delim_pos == std::string::npos) {
      continue;
    }

    auto distance = std::stoi(std::string(part.substr(0, delim_pos)));
    auto dest = part.substr(delim_pos + delim.size());
    distances.emplace_back(distance, dest);
  }

  return distances;
}

void AddStop(TransportCatalogue &cat, Query &query) {
  auto [lat, lng] = ParseCoordinates(query.data);

  cat.AddStop({query.name, lat, lng});
}

void AddDistance(TransportCatalogue &cat, Query &query) {
  auto distances = ParseDistances(query.data);

  for (const auto &dist : distances) {
    auto a = cat.GetStop(query.name);
    auto b = cat.GetStop(dist.second);
    auto stops = std::make_pair(a, b);
    cat.SetDistance(stops, dist.first);
  }
}

std::vector<std::string> ParseRoute(const std::string &line) {
  std::vector<std::string> result;

  if (line.find('>') != std::string::npos) {
    result = SplitLine(line, '>');
  } else {
    result = SplitLine(line, '-');

    // Приведение маршрута вида "stop1 - stop2 - ... stopN" к виду
    // "stop1 > stop2 > ... > stopN-1 > stopN > stopN-1 > ... > stop2 > stop1"
    for (int i = static_cast<int>(result.size()) - 2; i >= 0; --i) {
      result.push_back(result[i]);
    }
  }

  return result;
}

void FillDB(TransportCatalogue &cat, std::vector<std::string> &data) {
  using namespace std::string_literals;
  std::vector<size_t> buses_indexes, stop_indexes;
  size_t current_index = 0;

  // Парсинг и добавление остановок в базу данных
  for (const auto &line : data) {
    auto query = ParseData(line);

    if (query.type == "Stop"s) {
      AddStop(cat, query);
      stop_indexes.push_back(current_index++);
      continue;
    }

    buses_indexes.push_back(current_index++);
  }

  // Парсинг и добавление расстояний между остановками в базу данных
  for (const auto i : stop_indexes) {
    auto query = ParseData(data[i]);

    AddDistance(cat, query);
  }

  // Парсинг и добавление маршрутов в базу данных
  for (const auto i : buses_indexes) {
    auto query = ParseData(data[i]);

    if (query.type == "Bus"s) {
      Route route;

      for (const auto &stop : ParseRoute(query.data)) {
        route.push_back(cat.GetStop(stop));
      }

      cat.AddRoute({query.name, route});
    }
  }
}

} // namespace filler

} // namespace tc
