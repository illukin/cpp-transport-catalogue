#pragma once

#include "json.h"

#include <string>
#include <variant>
#include <vector>

namespace json {

class Builder {
private:
  class BaseContext;
  class DictValueContext;
  class DictItemContext;
  class ArrayItemContext;

public:
  Builder();

  Node Build();

  DictValueContext Key(const std::string &key);
  BaseContext Value(const NodeType &value);

  DictItemContext StartDict();
  BaseContext EndDict();

  ArrayItemContext StartArray();
  BaseContext EndArray();

private:
  Node root_;
  std::vector<Node *> nodes_stack_;

  NodeType &GetCurrentValue();
  void AddNextElement(const NodeType &value, bool is_just_value = false);
};

class Builder::BaseContext {
public:
  BaseContext(Builder &builder);

  Node Build();

  DictValueContext Key(const std::string &key);
  BaseContext Value(const NodeType &value);

  DictItemContext StartDict();
  BaseContext EndDict();

  ArrayItemContext StartArray();
  BaseContext EndArray();

private:
  Builder &builder_;
};

class Builder::DictValueContext : public BaseContext {
public:
  DictValueContext(BaseContext base);

  DictItemContext Value(const NodeType &value);

  Node Build() = delete;
  DictValueContext Key(const std::string &key) = delete;
  BaseContext EndDict() = delete;
  BaseContext EndArray() = delete;
};

class Builder::DictItemContext : public BaseContext {
public:
  DictItemContext(BaseContext base);

  Node Build() = delete;
  BaseContext Value(const NodeType &value) = delete;
  DictItemContext StartDict() = delete;
  ArrayItemContext StartArray() = delete;
  BaseContext EndArray() = delete;
};

class Builder::ArrayItemContext : public BaseContext {
public:
  ArrayItemContext(BaseContext base);

  ArrayItemContext Value(const NodeType &value);

  Node Build() = delete;
  DictValueContext Key(const std::string &key) = delete;
  BaseContext EndDict() = delete;
};

} // namespace json
