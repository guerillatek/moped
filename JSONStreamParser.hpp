#pragma once

#include "moped/ParserBase.hpp"
#include "moped/concepts.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stack>
#include <string>

namespace moped {
template <IParserEventDispatchC ParseEventDispatchT>
class JSONStreamParser : public ParserBase {

  using ExpectedText = std::expected<std::string, ParseError>;

  ExpectedText getMemberText(std::istream &is) {
    char c;
    is >> c;
    if (c != '"') {
      return std::unexpected{ParseError{
          "Expected open quote for a member definition, '\"' but got", c}};
    }

    _reusableBuffer.clear();
    while (is.operator bool() && is.peek() != '"') {
      _reusableBuffer += is.get();
    }
    if (!is) {
      return std::unexpected{
          ParseError{"Missing closing quote, '\"' for member start at {}",
                     _reusableBuffer}};
    }

    is.get(); // flush end quote
    if (isIdentifier(_reusableBuffer)) {
      return _reusableBuffer;
    }

    return std::unexpected{
        ParseError{"Invalid member identifier", _reusableBuffer.data()}};
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
    return std::unexpected{ParseError{
        "String value='{}' missing closing quote,\"", _reusableBuffer.data()}};
  }

  ExpectedText getNumericValueText(std::istream &is) {
    _reusableBuffer.clear();
    while ((bool)is && isNumericChar(is.peek())) {
      _reusableBuffer += is.get();
    }
    if (isNumeric(_reusableBuffer)) {
      return _reusableBuffer;
    }
    return std::unexpected{
        ParseError{"Invalid numeric value", _reusableBuffer.data()}};
  }

  ParseEventDispatchT &_eventDispatch;

public:
  JSONStreamParser(ParseEventDispatchT &eventDispatch)
      : _eventDispatch{eventDispatch} {}

  Expected parse(std::istream &ss) {
    using ValidatorStack = std::stack<char>;
    ValidatorStack vs;
    ParseState parseState = ParseState::MemberName;

    char c;
    ss >> std::ws;
    ss >> c;

    bool usingRootArray = false;
    if (c == '[') {
      usingRootArray = true;
      // Root array documents require specialize moped mappings
      // where a class object must contain a single member mapping
      // with and empty string collection type member.
      vs.push('{');
      auto result = _eventDispatch.onObjectStart();
      if (!result) {
        return result; // Both are Expected, return directly
      }
      result = _eventDispatch.onMember("");
      if (!result) {
        return std::unexpected{
            "JSON documents with root arrays require specialize moped mappings "
            "whereby the top level class object must contain a single member "
            "mapping with "
            "associating and and empty string (\"\") to a class member that "
            "maps to a moped collection"};
      }
      parseState = ParseState::OpenValue;
    } else {
      if (c != '{') {
        return std::unexpected(
            ParseError{"Expected an object start, '{' but got ", c});
      }
      vs.push(c);
      auto objectStartResult = _eventDispatch.onObjectStart();
      if (!objectStartResult) {
        return objectStartResult; // Both are Expected, return directly
      }
    }

    while (!vs.empty()) {
      char c;
      switch (parseState) {
      case Object: {
        ss >> c;
        if (c != '{') {
          return std::unexpected{
              ParseError{"Expected an object start, '{' but got ", c}};
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
          return std::unexpected(expectedText.error());
        }
        ss >> std::ws;
        ss >> c;
        if (c != ':') {
          return std::unexpected{
              ParseError{"Expected ':' in member definition but got", c}};
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
            return std::unexpected{expectedText.error()};
          }

          auto stringValueResult =
              _eventDispatch.onStringValue(expectedText.value());
          if (!stringValueResult) {
            return stringValueResult; // Both are Expected, return directly
          }
        } break;
        case 'n':
        case 't':
        case 'f': {
          // Boolean values
          char nullBoolText[64];
          size_t index = 0;
          while (ss && isalpha(ss.peek())) {
            nullBoolText[index++] = ss.get();
            if (index == (sizeof(nullBoolText) - 1)) {
              nullBoolText[index] = '\0';
              std::unexpected{ParseError{
                  "Invalid unquoted non numeric string, only "
                  "'true', 'false', and "
                  "'null' are valid RFC 7159 values ... parser found",
                  std::string_view{nullBoolText}}};
            }
          }
          nullBoolText[index] = '\0';
          std::string_view capturedText{nullBoolText};
          if (capturedText == "true") {
            auto boolResult = _eventDispatch.onBooleanValue(true);
            if (!boolResult) {
              return boolResult; // Both are Expected, return directly
            }
          } else if (capturedText == "false") {
            auto boolResult = _eventDispatch.onBooleanValue(false);
            if (!boolResult) {
              return boolResult; // Both are Expected, return directly
            }
          } else if (capturedText == "null") {
            auto nullResult = _eventDispatch.onNullValue();
            if (!nullResult) {
              return nullResult; // Both are Expected, return directly
            } else {
              return std::unexpected{ParseError{
                  "Invalid unquoted non numeric string, only "
                  "'true', 'false', and "
                  "'null' are valid RFC 7159 values ... parser found",
                  capturedText}};
            }
          }
        } break;
        case ']':
        case '}':
          parseState = ParseState::CloseValue;
          continue;
        default: {
          auto expectedText = getNumericValueText(ss);
          if (!expectedText) {
            return std::unexpected(expectedText.error());
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
            return std::unexpected{
                ParseError{"Invalid json parse, unexpected character", c}};
          }

          auto arrayFinishResult = _eventDispatch.onArrayFinish();
          if (!arrayFinishResult) {
            return arrayFinishResult; // Both are Expected, return directly
          }

          vs.pop();
          // If this was a root array, we need to close the root object
          if (usingRootArray && vs.size() == 1 && vs.top() == '{') {
            // Finish the root object
            auto objectFinishResult = _eventDispatch.onObjectFinish();
            if (!objectFinishResult) {
              return objectFinishResult; // Both are Expected, return directly
            }
            vs.pop();
          }
        } break;
        case '}': {
          if (vs.top() != '{') {
            return std::unexpected{
                ParseError{"Invalid json parse, unexpected character", c}};
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