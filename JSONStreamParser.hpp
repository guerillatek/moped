#pragma once

#include "moped/concepts.hpp"
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

  ParseEventDispatchT &_eventDispatch;

public:
  JSONStreamParser(ParseEventDispatchT &eventDispatch)
      : _eventDispatch{eventDispatch} {}

  Expected parse(std::istream &ss) {
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

    auto objectStartResult = _eventDispatch.onObjectStart();
    if (!objectStartResult) {
      return objectStartResult; // Both are Expected, return directly
    }

    ParseState parseState = ParseState::MemberName;

    while (!vs.empty()) {
      char c;
      switch (parseState) {
      case Object: {
        ss >> c;
        if (c != '{') {
          return std::unexpected(
              std::format("Expected an object start, '{{' but got {}", c));
        }

        auto objectStartResult = _eventDispatch.onObjectStart();
        if (!objectStartResult) {
          return objectStartResult; // Both are Expected, return directly
        }

        vs.push(c);
        parseState = ParseState::MemberName;
        break;
      }
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

        auto memberResult = _eventDispatch.onMember(expectedText.value());
        if (!memberResult) {
          return memberResult; // Both are Expected, return directly
        }
      } break;
      case OpenValue: {
        ss >> std::ws;
        c = ss.peek();

        switch (c) {
        case '{':
          parseState = ParseState::Object;
          continue;
        case '[':
          parseState = ParseState::Array;
          {
            auto arrayStartResult = _eventDispatch.onArrayStart();
            if (!arrayStartResult) {
              return arrayStartResult; // Both are Expected, return directly
            }
          }
          continue;
        case '"': {
          ss.get(); // consume the opening quote
          auto expectedText = getStringValueText(ss);
          if (!expectedText) {
            return std::unexpected(
                std::format("{} in jsonText", expectedText.error()));
          }

          auto stringValueResult =
              _eventDispatch.onStringValue(expectedText.value());
          if (!stringValueResult) {
            return stringValueResult; // Both are Expected, return directly
          }
        } break;
        case 't':
        case 'f': {
          // Boolean values
          std::string boolText;
          while (ss && isalpha(ss.peek())) {
            boolText += ss.get();
          }
          if (boolText == "true") {
            auto boolResult = _eventDispatch.onBooleanValue(true);
            if (!boolResult) {
              return boolResult; // Both are Expected, return directly
            }
          } else if (boolText == "false") {
            auto boolResult = _eventDispatch.onBooleanValue(false);
            if (!boolResult) {
              return boolResult; // Both are Expected, return directly
            }
          } else {
            return std::unexpected(
                std::format("Invalid boolean value '{}'", boolText));
          }
        } break;
        case ']':
        case '}':
          parseState = ParseState::CloseValue;
          continue;
        default: {
          auto expectedText = getNumericValueText(ss);
          if (!expectedText) {
            return std::unexpected(std::format("{}", expectedText.error()));
          }

          auto numericResult =
              _eventDispatch.onNumericValue(expectedText.value());
          if (!numericResult) {
            return numericResult; // Both are Expected, return directly
          }
        }
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

          auto arrayFinishResult = _eventDispatch.onArrayFinish();
          if (!arrayFinishResult) {
            return arrayFinishResult; // Both are Expected, return directly
          }

          vs.pop();
        } break;
        case '}': {
          if (vs.top() != '{') {
            return std::unexpected(
                std::format("Invalid json parse, unexpected character, {}", c));
          }

          auto objectFinishResult = _eventDispatch.onObjectFinish();
          if (!objectFinishResult) {
            return objectFinishResult; // Both are Expected, return directly
          }

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
    return parse(ss);
  }

  auto &getDispatcher() { return _eventDispatch; }
};

} // namespace moped