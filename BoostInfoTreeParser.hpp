#pragma once

#include "concepts.hpp"
#include <boost/property_tree/info_parser.hpp>
#include <sstream>
#include <string>
#include <string_view>

namespace moped {

template <IParserEventDispatchC ParseEventDispatchT> class BoostInfoTreeParser {
  ParseEventDispatchT _eventDispatch;

  // Helper to recursively walk the ptree and dispatch events
  Expected walk(const boost::property_tree::ptree &node,
                std::string_view key = {}) {
    using ptree = boost::property_tree::ptree;

    if (!key.empty())
      _eventDispatch.onMember(key);

    if (node.empty()) {
      // Leaf node: dispatch as string, bool, or numeric
      const std::string &val = node.data();
      if (val == "true" || val == "false") {
        _eventDispatch.onBooleanValue(val == "true");
      }
      else if (val == "null") {
        _eventDispatch.onNulValue();
      } else if (!val.empty() &&
                 std::all_of(val.begin(), val.end(), ::isdigit)) {
        _eventDispatch.onNumericValue(val);
      } else {
        _eventDispatch.onStringValue(val);
      }
      return;
    }

    // If node has children, determine if it's an array or object
    bool isArray = true;
    for (const auto &child : node) {
      if (!child.first.empty()) {
        isArray = false;
        break;
      }
    }

    if (isArray) {
      _eventDispatch.onArrayStart();
      for (const auto &child : node) {
        walk(child.second);
      }
      _eventDispatch.onArrayFinish();
    } else {
      auto result = _eventDispatch.onObjectStart();
      if (!result) {
        return std::unexpected(result.error());
      }
      for (const auto &child : node) {
        walk(child.second, child.first);
      }
      _eventDispatch.onObjectFinish();
    }
  }

public:
  using Expected = std::expected<void, ParseError>;

  Expected parseInfoTreeFromString(const std::string &infoTreeText) {
    std::stringstream ss(infoTreeText);
    boost::property_tree::ptree tree;
    try {
      boost::property_tree::read_info(ss, tree);
      walk(tree);
    } catch (const std::exception &ex) {
      return std::unexpected(std::string("INFO parse error: ") + ex.what());
    }
    return {};
  }

  Expected parseInfoTreeFromFile(const std::string &filePath) {
    std::stringstream ss(infoTreeText);
    boost::property_tree::ptree tree;
    try {
      boost::property_tree::read_info(ss, tree);
      walk(tree);
    } catch (const std::exception &ex) {
      return std::unexpected(std::string("INFO parse error: ") + ex.what());
    }
    return {};
  }

  auto &getDispatcher() { return _eventDispatch; }
};

} // namespace moped