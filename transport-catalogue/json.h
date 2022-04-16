#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json {

class Node;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;
using NodeType = std::variant<std::nullptr_t, Array, Dict, bool, int, double,
  std::string>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
  using runtime_error::runtime_error;
};

class Node {
public:
  Node() = default;
  Node(std::nullptr_t);
  Node(Array array);
  Node(Dict map);
  Node(bool value);
  Node(int value);
  Node(double value);
  Node(std::string value);

  [[nodiscard]] bool IsNull() const;
  [[nodiscard]] bool IsArray() const;
  [[nodiscard]] bool IsMap() const;
  [[nodiscard]] bool IsBool() const;
  [[nodiscard]] bool IsInt() const;
  [[nodiscard]] bool IsDouble() const;
  [[nodiscard]] bool IsPureDouble() const;
  [[nodiscard]] bool IsString() const;

  const Array &AsArray() const;
  const Dict &AsMap() const;
  bool AsBool() const;
  int AsInt() const;
  double AsDouble() const;
  const std::string &AsString() const;

  const NodeType &GetNodeType() const;

  bool operator==(const Node &rhs) const;
  bool operator!=(const Node &rhs) const;

private:
  NodeType node_type_;
};

class Document {
public:
  explicit Document(Node root);

  [[nodiscard]] const Node& GetRoot() const;

  bool operator==(const Document &rhs) const;
  bool operator!=(const Document &rhs) const;

private:
  Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}  // namespace json
