#include "json_builder.h"

namespace json {

Builder::Builder() : root_{}, nodes_stack_{&root_} {}

Node Builder::Build() {
  using namespace std::string_literals;

  if (!nodes_stack_.empty()) {
    throw std::logic_error("Invalid JSON. Building error."s);
  }

  return std::move(root_);
}

Builder::DictValueContext Builder::Key(const std::string &key) {
  using namespace std::string_literals;

  auto &parent = GetCurrentValue();

  if (!std::holds_alternative<Dict>(parent)) {
    throw std::logic_error("Error. Key is not in a dict."s);
  }

  nodes_stack_.emplace_back(&(std::get<Dict>(parent)[key]));

  return BaseContext(*this);
}

Builder::BaseContext Builder::Value(const NodeType &value) {
  AddNextElement(value, true);

  return *this;
}

Builder::ArrayItemContext Builder::StartArray() {
  AddNextElement(Array{});

  return BaseContext(*this);
}

Builder::BaseContext Builder::EndArray() {
  using namespace std::string_literals;

  if (!std::holds_alternative<Array>(GetCurrentValue())) {
    throw std::logic_error("Error while ending an array."s);
  }

  nodes_stack_.pop_back();

  return *this;
}

Builder::DictItemContext Builder::StartDict() {
  AddNextElement(Dict{});

  return BaseContext(*this);
}

Builder::BaseContext Builder::EndDict() {
  using namespace std::string_literals;

  if (!std::holds_alternative<Dict>(GetCurrentValue())) {
    throw std::logic_error("Error while ending a dict."s);
  }

  nodes_stack_.pop_back();

  return *this;
}

NodeType &Builder::GetCurrentValue() {
  using namespace std::string_literals;

  if (nodes_stack_.empty()) {
    throw std::logic_error("Error while trying to change complete JSON."s);
  }

  return nodes_stack_.back()->GetNodeType();
}

void Builder::AddNextElement(const NodeType &value, bool is_just_value) {
  using namespace std::string_literals;

  auto &parent = GetCurrentValue();

  if (std::holds_alternative<Array>(parent)) {
    auto &node = std::get<Array>(parent).emplace_back(value);

    if (std::holds_alternative<Array>(value)
        || std::holds_alternative<Dict>(value)) {
      nodes_stack_.emplace_back(&node);
    }
  } else {
    parent = value;
    if (is_just_value) {
      nodes_stack_.pop_back();
    }
  }
}

Builder::BaseContext::BaseContext(Builder &builder) : builder_(builder) {}

Node Builder::BaseContext::Build() {
  return builder_.Build();
}

Builder::DictValueContext Builder::BaseContext::Key(const std::string &key) {
  return builder_.Key(key);
}

Builder::BaseContext Builder::BaseContext::Value(const NodeType &value) {
  return builder_.Value(value);
}

Builder::DictItemContext Builder::BaseContext::StartDict() {
  return builder_.StartDict();
}

Builder::BaseContext Builder::BaseContext::EndDict() {
  return builder_.EndDict();
}

Builder::ArrayItemContext Builder::BaseContext::StartArray() {
  return builder_.StartArray();
}

Builder::BaseContext Builder::BaseContext::EndArray() {
  return builder_.EndArray();
}

Builder::DictValueContext::DictValueContext(BaseContext base)
  : BaseContext(base) {}

Builder::DictItemContext
Builder::DictValueContext::Value(const NodeType &value) {
  return BaseContext::Value(value);
}

Builder::DictItemContext::DictItemContext(BaseContext base)
  : BaseContext(base) {}

Builder::ArrayItemContext::ArrayItemContext(BaseContext base)
  : BaseContext(base) {}

Builder::ArrayItemContext
Builder::ArrayItemContext::Value(const NodeType &value) {
  return BaseContext::Value(value);
}

} // namespace json
