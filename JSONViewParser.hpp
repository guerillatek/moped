#pragma once

#include "concepts.hpp"
#include "moped/ParserBase.hpp"
#include <expected>
#include <regex>
#include <stack>
#include <string>
#include <string_view>

namespace moped {

template <IParserEventDispatchC ParseEventDispatchT>
class JSONViewParser : public ParserBase {
  using Iterator = std::string_view::const_iterator;
  using ExpectedText = std::expected<std::string_view, ParseError>;

  ParseEventDispatchT &_eventDispatch;

  // Utility: skip whitespace
  static void skipWhitespace(Iterator &it, Iterator end) {
    while (it != end && std::isspace(static_cast<unsigned char>(*it)))
      ++it;
  }

  // Parse a quoted member name, return the view and advance iterator
  ExpectedText getMemberText(Iterator &it, Iterator end) {
    skipWhitespace(it, end);
    if (it == end || *it != '"')
      return std::unexpected("Expected open quote for member name");
    ++it; // skip opening quote
    Iterator start = it;
    while (it != end && *it != '"')
      ++it;
    if (it == end)
      return std::unexpected("Missing closing quote for member name");
    std::string_view name{start, static_cast<size_t>(it - start)};
    ++it; // skip closing quote
    // Optionally, validate name with regex here if needed
    return name;
  }

  // Parse a quoted string value, return the view and advance iterator
  ExpectedText getStringValueText(Iterator &it, Iterator end) {
    Iterator start = it;
    while (it != end) {
      if (*it == '\\') {
        it += 2; // skip escaped char
        continue;
      }
      if (*it == '"') {
        std::string_view val{start, static_cast<size_t>(it - start)};
        ++it; // skip closing quote
        return val;
      }
      ++it;
    }
    return std::unexpected("String value missing closing quote");
  }

  // Parse a numeric value, return the view and advance iterator
  ExpectedText getNumericValueText(Iterator &it, Iterator end) {
    Iterator start = it;
    while (it != end && (isNumericChar(*it))) {
      ++it;
    }
    if (start == it)
      return std::unexpected("Expected numeric value");
    std::string_view val{start, static_cast<size_t>(it - start)};
    // Optionally, validate with regex here
    return val;
  }

public:
  JSONViewParser(ParseEventDispatchT &eventDispatch)
      : _eventDispatch(eventDispatch) {}

  Expected parse(std::string_view json) {

    enum class ParseState { MemberName, OpenValue, Object, Array, CloseValue };
    ParseState parseState = ParseState::MemberName;

    using ValidatorStack = std::stack<char>;
    ValidatorStack vs;
    Iterator it = json.begin(), end = json.end();

    skipWhitespace(it, end);

    bool usingRootArray = false;
    if (*it == '[') {
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
      if (it == end || *it != '{') {
        return std::unexpected("Expected object start '{'");
      }
      vs.push(*it++);
      auto objectStartResult = _eventDispatch.onObjectStart();
      if (!objectStartResult) {
        return objectStartResult; // Both are Expected, return directly
      }
    }

    while (!vs.empty()) {
      skipWhitespace(it, end);
      switch (parseState) {
      case ParseState::Object: {
        if (it == end || *it != '{')
          return std::unexpected("Expected object start '{'");

        auto objectStartResult = _eventDispatch.onObjectStart();
        if (!objectStartResult) {
          return objectStartResult; // Both are Expected, return directly
        }

        vs.push(*it++);
        parseState = ParseState::MemberName;
        break;
      }
      case ParseState::MemberName: {
        auto expectedText = getMemberText(it, end);
        if (!expectedText)
          return std::unexpected(expectedText.error());
        skipWhitespace(it, end);
        if (it == end || *it != ':')
          return std::unexpected("Expected ':' after member name");
        ++it; // skip ':'
        parseState = ParseState::OpenValue;
        auto memberResult = _eventDispatch.onMember(expectedText.value());
        if (!memberResult) {
          return memberResult; // Both are Expected, return directly
        }
      } break;
      case ParseState::OpenValue: {
        skipWhitespace(it, end);
        if (it == end)
          return std::unexpected("Unexpected end of input in value");
        switch (*it) {
        case '{':
          parseState = ParseState::Object;
          continue;
        case '[': {
          vs.push(*it++);
          auto arrayStartResult = _eventDispatch.onArrayStart();
          if (!arrayStartResult) {
            return arrayStartResult; // Both are Expected, return directly
          }
          parseState = ParseState::OpenValue;
          continue;
        }
        case '"': {
          ++it; // skip opening quote
          auto expectedText = getStringValueText(it, end);
          if (!expectedText)
            return std::unexpected(expectedText.error());

          auto stringValueResult =
              _eventDispatch.onStringValue(expectedText.value());
          if (!stringValueResult) {
            return stringValueResult; // Both are Expected, return directly
          }
        } break;
        case 'n':
        case 't':
        case 'f': {
          // Boolean
          Iterator boolStart = it;
          while (it != end && std::isalpha(*it))
            ++it;
          std::string_view nullBoolText{boolStart,
                                        static_cast<size_t>(it - boolStart)};
          if (nullBoolText == "true") {
            auto boolResult = _eventDispatch.onBooleanValue(true);
            if (!boolResult) {
              return boolResult; // Both are Expected, return directly
            }
          } else if (nullBoolText == "false") {
            auto boolResult = _eventDispatch.onBooleanValue(false);
            if (!boolResult) {
              return boolResult; // Both are Expected, return directly
            }
          } else if (nullBoolText == "null") {
            auto nullResult = _eventDispatch.onNullValue();
            if (!nullResult) {
              return nullResult; // Both are Expected, return directly
            }
          } else {
            return std::unexpected(
                ParseError("Invalid unquoted non numeric string, only "
                           "'true', 'false', and "
                           "'null' are valid RFC 7159 values",
                           nullBoolText));
          }
        } break;
        case ']':
        case '}':
          parseState = ParseState::CloseValue;
          continue;
        default: {
          auto expectedText = getNumericValueText(it, end);
          if (!expectedText)
            return std::unexpected(expectedText.error());

          auto numericResult =
              _eventDispatch.onNumericValue(expectedText.value());
          if (!numericResult) {
            return numericResult; // Both are Expected, return directly
          }
        }
        }
        parseState = ParseState::CloseValue;
      } break;
      case ParseState::Array:
        if (it == end || *it != '[')
          return std::unexpected("Expected array start '['");
        vs.push(*it++);
        parseState = ParseState::OpenValue;
        break;
      case ParseState::CloseValue: {
        skipWhitespace(it, end);
        if (it == end)
          return std::unexpected("Unexpected end of input after value");
        char c = *it++;
        switch (c) {
        case ',':
          if (vs.top() == '{')
            parseState = ParseState::MemberName;
          else if (vs.top() == '[')
            parseState = ParseState::OpenValue;
          else
            return std::unexpected("Unexpected ','");
          break;
        case ']': {
          if (vs.top() != '[')
            return std::unexpected("Unexpected ']'");

          auto arrayFinishResult = _eventDispatch.onArrayFinish();
          if (!arrayFinishResult) {
            return arrayFinishResult; // Both are Expected, return directly
          }

          vs.pop();
          if (usingRootArray && vs.size() == 1 && vs.top() == '{') {
            // Finish the root object
            auto objectFinishResult = _eventDispatch.onObjectFinish();
            if (!objectFinishResult) {
              return objectFinishResult; // Both are Expected, return directly
            }
            vs.pop();
          }
          break;
        }
        case '}': {
          if (vs.top() != '{')
            return std::unexpected("Unexpected '}'");

          auto objectFinishResult = _eventDispatch.onObjectFinish();
          if (!objectFinishResult) {
            return objectFinishResult; // Both are Expected, return directly
          }

          vs.pop();
          break;
        }
        default:
          return std::unexpected("Unexpected character after value");
        }
      } break;
      }
    }
    return {};
  }

  auto &getDispatcher() { return _eventDispatch; }
};

} // namespace moped