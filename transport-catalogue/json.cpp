#include "json.h"

namespace json {

namespace {

using namespace std::literals;

Node LoadNode(std::istream& input);
Node LoadString(std::istream& input);
void PrintNode(const Node& node, std::ostream &output);

Node LoadNumber(std::istream& input) {
  std::string parsed_num;

  // Считывает в parsed_num очередной символ из input
  auto read_char = [&parsed_num, &input] {
    parsed_num += static_cast<char>(input.get());
    if (!input) {
      throw ParsingError("Failed to read number from stream"s);
    }
  };

  // Считывает одну или более цифр в parsed_num из input
  auto read_digits = [&input, read_char] {
    if (!std::isdigit(input.peek())) {
      throw ParsingError("A digit is expected"s);
    }
    while (std::isdigit(input.peek())) {
      read_char();
    }
  };

  if (input.peek() == '-') {
    read_char();
  }
  // Парсим целую часть числа
  if (input.peek() == '0') {
    read_char();
    // После 0 в JSON не могут идти другие цифры
  } else {
    read_digits();
  }

  bool is_int = true;
  // Парсим дробную часть числа
  if (input.peek() == '.') {
    read_char();
    read_digits();
    is_int = false;
  }

  // Парсим экспоненциальную часть числа
  if (int ch = input.peek(); ch == 'e' || ch == 'E') {
    read_char();
    if (ch = input.peek(); ch == '+' || ch == '-') {
      read_char();
    }
    read_digits();
    is_int = false;
  }

  try {
    if (is_int) {
      // Сначала пробуем преобразовать строку в int
      try {
        return Node(std::stoi(parsed_num));
      } catch (...) {
        // В случае неудачи, например, при переполнении,
        // код ниже попробует преобразовать строку в double
      }
    }
    return Node(std::stod(parsed_num));
  } catch (...) {
    throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
  }
}

Node LoadArray(std::istream& input) {
  Array result;

  for (char c; input >> c && c != ']';) {
    if (c != ',') {
      input.putback(c);
    }
    result.push_back(LoadNode(input));
  }

  if (!input) {
    throw ParsingError("Load Array error");
  }

  return {std::move(result)};
}

Node LoadDict(std::istream& input) {
  Dict dict;
  auto dict_parse_error = "Dictionary parsing error"s;
  char c;

  while (input >> c && c != '}') {
    if (c == '"') {
      auto key = LoadString(input).AsString();

      if (input >> c && c == ':') {
        if (dict.find(key) != dict.end()) {
          throw ParsingError(dict_parse_error);
        }

        dict.emplace(std::move(key), LoadNode(input));
      } else {
        throw ParsingError(dict_parse_error);
      }
    } else if (c != ',') {
      throw ParsingError(dict_parse_error);
    }
  }

  if (!input) {
    throw ParsingError(dict_parse_error);
  }

  return {std::move(dict)};
}

Node LoadString(std::istream& input) {
  auto it = std::istreambuf_iterator<char>(input);
  auto end = std::istreambuf_iterator<char>();
  std::string result;
  auto str_parse_error = "String parsing error"s;
  char c;

  while(!input.eof()) {
    if (it == end) {
      throw ParsingError(str_parse_error);
    }

    c = *it;

    if (c == '"') {
      ++it;
      break;
    } else if (c == '\n' || c == '\r') {
      throw ParsingError(str_parse_error);
    } else if (c == '\\') {
      ++it;
      if (it == end) {
        throw ParsingError(str_parse_error);
      }

      c = *it;

      switch (c) {
        case 'n': result += '\n'; break;
        case 't': result += '\t'; break;
        case 'r': result += '\r'; break;
        case '"': result += '"'; break;
        case '\\': result += '\\'; break;
        default: throw ParsingError("Unsupported escape seq"s);
      }
    } else {
      result += c;
    }

    ++it;
  }

  return {std::move(result)};
}

void MakeStr(std::istream &input, std::string &result) {
  char c;

  while (std::isalpha(input.peek())) {
    input >> c;
    result += c;
  }
}

Node LoadBool(std::istream& input) {
  const auto t = "true"sv;
  const auto f = "false"sv;
  std::string result;

  MakeStr(input, result);

  if (result == t) {
    return {true};
  } else if (result == f) {
    return {false};
  }

  throw ParsingError("Bool parse error"s);
}

Node LoadNull(std::istream& input) {
  const auto comp = "null"sv;
  std::string result;

  MakeStr(input, result);

  if (result == comp) {
    return {};
  }

  throw ParsingError("Null parse error"s);
}

Node LoadNode(std::istream& input) {
  char c;

  if (!(input >> c)) {
    throw ParsingError("EOF"s);
  }

  switch (c) {
    case '[': return LoadArray(input);
    case '{': return LoadDict(input);
    case '"': return LoadString(input);
    case 't': [[fallthrough]]; // true и false обрабатываются одинаково
    case 'f': input.putback(c); return LoadBool(input);
    case 'n': input.putback(c); return LoadNull(input);
    default: input.putback(c); return LoadNumber(input);
  }
}

void PrintString(const std::string &str, std::ostream &out) {
  out << '"';
  for (const char c : str) {
    switch (c) {
      case '\r': out << R"(\r)"; break;
      case '\n': out << R"(\n)"; break;
      case '"': [[fallthrough]]; // (") и (\) обрабатываются одинаково
      case '\\': out << '\\'; [[fallthrough]];
      default: out << c; break;
    }
  }
  out << '"';
}

template <typename T>
void PrintValue(const T &value, std::ostream &out) {
  out << value;
}

void PrintValue(const std::string& value, std::ostream &out) {
  PrintString(value, out);
}

void PrintValue(const std::nullptr_t&, std::ostream &out) {
  out << "null"sv;
}

void PrintValue(const bool &value, std::ostream &out) {
  out << std::boolalpha << value;
}

void PrintValue(const Array& nodes, std::ostream &out) {
  out << "[\n"sv;
  bool first = true;

  for (const Node &node : nodes) {
    if (first) {
      first = false;
    } else {
      out << ",\n"sv;
    }

    PrintNode(node, out);
  }
  out << "\n]"sv;
}

void PrintValue(const Dict& nodes, std::ostream &out) {
  out << "{\n"sv;
  bool first = true;

  for (const auto& [key, node] : nodes) {
    if (first) {
      first = false;
    } else {
      out << ",\n"sv;
    }

    PrintString(key, out);
    out << ": "sv;
    PrintNode(node, out);
  }

  out << "\n}"sv;
}

void PrintNode(const Node& node, std::ostream &output) {
  std::visit([&output](const auto& value) {
    PrintValue(value, output);
  }, node.GetNodeType());
}

}  // namespace

Node::Node(variant value) : variant(std::move(value)) {}

bool Node::operator==(const Node &rhs) const {
  return GetNodeType() == rhs.GetNodeType();
}

bool Node::operator!=(const Node &rhs) const {
  return !(*this == rhs);
}

bool Node::IsNull() const {
  return std::holds_alternative<std::nullptr_t>(*this);
}

bool Node::IsArray() const {
  return std::holds_alternative<Array>(*this);
}

bool Node::IsMap() const {
  return std::holds_alternative<Dict>(*this);
}

bool Node::IsBool() const {
  return std::holds_alternative<bool>(*this);
}

bool Node::IsInt() const {
  return std::holds_alternative<int>(*this);
}

bool Node::IsDouble() const {
  return IsInt() || IsPureDouble();
}

bool Node::IsPureDouble() const {
  return std::holds_alternative<double>(*this);;
}

bool Node::IsString() const {
  return std::holds_alternative<std::string>(*this);
}

const Array &Node::AsArray() const {
  return IsArray() ? std::get<Array>(*this)
    : throw std::logic_error("Is not Array"s);
}

const Dict &Node::AsMap() const {
  return IsMap() ? std::get<Dict>(*this)
    : throw std::logic_error("Is not Dict"s);
}

bool Node::AsBool() const {
  return IsBool() ? std::get<bool>(*this)
    : throw std::logic_error("Is not bool"s);
}

int Node::AsInt() const {
  return IsInt() ? std::get<int>(*this)
    : throw std::logic_error("Is not int"s);
}

double Node::AsDouble() const {
  if (IsInt()) {
    return std::get<int>(*this);
  }

  if (IsPureDouble()) {
    return std::get<double>(*this);
  }

  throw std::logic_error("Is not double"s);
}

const std::string &Node::AsString() const {
  return IsString() ? std::get<std::string>(*this)
    : throw std::logic_error("Is not string"s);
}

const Node::variant &Node::GetNodeType() const {
  return *this;
}

Node::variant &Node::GetNodeType() {
  return *this;
}

Document::Document(Node root) : root_(std::move(root)) {}

bool Document::operator==(const Document &rhs) const {
  return this->GetRoot() == rhs.GetRoot();
}

bool Document::operator!=(const Document &rhs) const {
  return !(*this == rhs);
}

const Node& Document::GetRoot() const {
  return root_;
}

Document Load(std::istream& input) {
  return Document{LoadNode(input)};
}

void Print(const Document& doc, std::ostream& output) {
  PrintNode(doc.GetRoot(), output);
}

}  // namespace json
