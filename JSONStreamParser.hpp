#pragma once

#include "concepts.hpp"
#include "moped/ParserBase.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stack>
#include <string>

namespace moped {

using Expected = std::expected<void, std::string>;

template <IParserEventDispatchC ParseEventDispatchT>
class JSONStreamParser : public ParserBase {

  using ExpectedText = std::expected<std::string, std::string>;

  ExpectedText getMemberText(std::istream &is) {
    char c;
    is >> c;
    if (c != '"') {
      return std::unexpected(std::format(
          "Expected open quote for a member definition, '\"' but got {}", c));
    }

    _reusableBuffer.clear();
    while (is.operator bool() && is.peek() != '"') {
      _reusableBuffer += is.get();
    }
    if (!is) {
      return std::unexpected(
          std::format("Missing closing quote, '\"' for member start at {}",
                      _reusableBuffer));
    }

    is.get(); // flush end quote
    if (isIdentifier(_reusableBuffer)) {
      return _reusableBuffer;
    }

    return std::unexpected(
        std::format("Invalid member identifier {}", _reusableBuffer));
  }

  ExpectedText getStringValueText(std::istream &ss) {
    char c = ss.get();
    _reusableBuffer.clear();
    _reusableBuffer += c;
    while (ss) {
      if (ss.peek() == '\\') {
        // Check for escape characters
        _reusableBuffer += ss.get();
        _reusableBuffer += ss.get();
        continue;
      }
      c = ss.get();
      if (c != '"') {
        _reusableBuffer += c;
        continue;
      }
      return _reusableBuffer;
    }
    return std::unexpected(std::format(
        "String value='{}' missing closing quote,\"", _reusableBuffer));
  }

  ExpectedText getNumericValueText(std::istream &is) {
    _reusableBuffer.clear();
    while ((bool)is && isNumericChar(is.peek())) {
      _reusableBuffer += is.get();
    }
    if (isNumeric(_reusableBuffer)) {
      return _reusableBuffer;
    }
    return std::unexpected(
        std::format("Invalid numeric value {}", _reusableBuffer));
  }

  ParseEventDispatchT _eventDispatch;

public:
  Expected parseInputStream(std::istream &ss) {
    using ValidatorStack = std::stack<char>;
    ValidatorStack vs;

    char c;
    ss >> std::ws;
    ss >> c;

    if (c != '{') {
      return std::unexpected(
          std::format("Expected an object start, '{{' but got {}", c));
    }
    vs.push(c);
    _eventDispatch.onObjectStart();
    ParseState parseState = ParseState::MemberName;

    while (!vs.empty()) {
      char c;
      switch (parseState) {
      case Object:
        ss >> c;
        if (c != '{') {
          return std::unexpected(
              std::format("Expected an object start, '{{' but got {}", c));
        }
        _eventDispatch.onObjectStart();
        vs.push(c);
        parseState = ParseState::MemberName;
        break;
      case MemberName: {
        auto expectedText = getMemberText(ss);
        if (!expectedText) {
          return std::unexpected(std::format("{}", expectedText.error()));
        }
        ss >> std::ws;
        ss >> c;
        if (c != ':') {
          return std::unexpected(
              std::format("Expected ':' in member definition but got {}", c));
        }
        parseState = ParseState::OpenValue;
        _eventDispatch.onMember(expectedText.value());
      } break;
      case OpenValue: {
        ss >> std::ws;
        c = ss.peek();

        switch (c) {
        case '{':
          parseState = ParseState::Object;
          continue;
          ;
        case '[':
          parseState = ParseState::Array;
          _eventDispatch.onArrayStart();
          continue;
          ;
        case '"': {
          ss.get(); // consume the opening quote
          auto expectedText = getStringValueText(ss);
          if (!expectedText) {
            return std::unexpected(
                std::format("{} in jsonText", expectedText.error()));
          }
          _eventDispatch.onStringValue(expectedText.value());
        } break;
        case 't':
        case 'f': {
          // Boolean values
          std::string boolText;
          while (ss && isalpha(ss.peek())) {
            boolText += ss.get();
          }
          if (boolText == "true") {
            _eventDispatch.onBooleanValue(true);
          } else if (boolText == "false") {
            _eventDispatch.onBooleanValue(false);
          } else {
            return std::unexpected(
                std::format("Invalid boolean value '{}'", boolText));
          }
        } break;
        case ']':
        case '}':
          parseState = ParseState::CloseValue;
          continue;
          break;
        default:
          auto expectedText = getNumericValueText(ss);
          if (!expectedText) {
            return std::unexpected(std::format("{}", expectedText.error()));
          }
          _eventDispatch.onNumericValue(expectedText.value());
        };
        parseState = ParseState::CloseValue;
        // We've processed an atomic value
      } break;
      case Array:
        ss >> c;
        vs.push(c);
        parseState = ParseState::OpenValue;
        break;

      case CloseValue: {
        char c;
        ss >> c;

        switch (c) {
        case ',': {
          if (vs.top() == '{') {
            parseState = ParseState::MemberName;
            continue;
          }
          if (vs.top() == '[') {
            parseState = ParseState::OpenValue;
            continue;
          }
        } break;
        case ']': {
          if (vs.top() != '[') {
            return std::unexpected(
                std::format("Invalid json parse, unexpected character, {}", c));
          }
          _eventDispatch.onArrayFinish();
          vs.pop();
        } break;
        case '}': {
          if (vs.top() != '{') {
            return std::unexpected(
                std::format("Invalid json parse, unexpected character, {}", c));
          }
          _eventDispatch.onObjectFinish();
          vs.pop();
        } break;
        };
      }
      };
    }
    return Expected{};
  }

  Expected parseJsonText(const std::string &jsonText) {
    std::stringstream ss{jsonText};
    return parseInputStream(ss);
  }

  auto &getDispatcher() { return _eventDispatch; }
};

} // namespace moped