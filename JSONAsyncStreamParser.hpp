#pragma once

#include "CoParseResult.hpp"
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

using ExpectedText = std::expected<std::string, ParseError>;
using ParseResult = CoParseResult<Expected>;

template <IParserEventDispatchC ParseEventDispatchT>
class JSONSAsyncStreamParser : public ParserBase {

  template <typename Actions...>
  inline std::awaitable<Expected>
  coWaitForValidStream(auto &&getEndofStreamError, auto &&...readAction,
                       Actions &&...continueReadActions) {
    if (_activeStream.eof()) {
      if (_finalFrame) {
        co_return std::unexpected(getEndofStreamError());
      }
      co_await std::suspend_always{};
      if (_finalFrame && _activeStream.eof()) {
        co_return std::unexpected(getEndofStreamError());
      }
    }
    readAction();
    if constexpr (sizeof...(continueReadActions) == 0) {
      co_return Expected{};
    } else {
      co_return coWaitForValidStream(
          getEndofStreamError, std::forward<Actions>(continueReadActions)...);
    }
  }

  std::awaitable<ExpectedText> getMemberText() {
    char c;
    if (auto result = co_await coWaitForValidStream(
            [&, this]() {
              co_return std::string(
                  "Unexpected end of frame while expecting text'\"' "
                  "for member definition");
            },
            [&, this]() { _activeStream >> c; });
        !result) {
      co_return std::unexpected(result.error());
    }

    if (c != '"') {
      co_return std::unexpected(std::format(
          "Expected open quote for a member definition, '\"' but got {}", c));
    }

    bool isEndQuote = false;
    if (auto result = co_await coWaitForValidStream(
            [&, this]() {
              co_return std::string(
                  "Unexpected end of frame while expecting text'\"' "
                  "for member definition");
            },
            [&, this]() {
              _reusableBuffer.clear();
              isEndQuote = ((c = _activeStream.get()) == '"');
            });
        !result) {
      co_return std::unexpected(result.error());
    }

    while (!isEndQuote) {
      if (auto result = co_await coWaitForValidStream(
              [&, this]() {
                co_return std::format(
                    "Unexpected end of frame while reading member text=",
                    _reusableBuffer);
              },
              [&, this]() {
                _reusableBuffer += c;
                isEndQuote = ((c = _activeStream.get()) == '"');
              });
          !result) {
        co_return std::unexpected(result.error());
      }
    }

    if (isIdentifier(_reusableBuffer)) {
      co_return _reusableBuffer;
    }

    co_return std::unexpected(
        std::format("Invalid member identifier {}", _reusableBuffer));
  }

  std::awaitable<ExpectedText> getStringValueText() {
    _reusableBuffer.clear();

    char c;
    if (auto result = co_await coWaitForValidStream(
            [&, this]() {
              return std::format(
                  "Unexpected end of frame while reading content for "
                  "string {}",
                  _reusableBuffer);
            },
            [&, this]() { c = _activeStream.peek(); });
        !result) {
      co_return std::unexpected(result.error());
    }

    while (c != '"') {

      if (c == '\\') {
        // Check for escape characters
        _reusableBuffer += _activeStream.get();
        if (auto result = co_await coWaitForValidStream(
                [&, this]() {
                  return std::format("Unexpected end of frame while reading "
                                     "escaped content in value"
                                     "for string {}",
                                     _reusableBuffer);
                },
                [&, this]() { _reusableBuffer += _activeStream.get(); },
                [&, this]() { c = _activeStream.peek(); });
            !result) {
          co_return std::unexpected(result.error());
        }
        continue;
      }
      _reusableBuffer += _activeStream.get();
      if (auto result = co_await coWaitForValidStream(
              [&, this]() {
                return std::format(
                    "Unexpected end of frame while reading value "
                    "for string {}",
                    _reusableBuffer);
              },
              [&, this]() { c = _activeStream.peek(); });
          !result) {
        co_return std::unexpected(result.error());
      }
    }
    co_return _reusableBuffer;
  }

  std::awaitable<ExpectedText> getNumericValueText() {
    _reusableBuffer.clear();
    char numChar = _activeStream.peek();

    while (isNumericChar(numChar)) {
      _reusableBuffer += _activeStream.get();
      if (auto result = co_await coWaitForValidStream(
              [&, this]() {
                return std::format(
                    "Unexpected end of frame while parsing numeric value {}",
                    _reusableBuffer);
              },
              [&, this]() { numChar = _activeStream.peek(); });
          !result) {
        co_return std::unexpected(result.error());
      }
    }

    if (isNumeric(_reusableBuffer)) {
      co_return _reusableBuffer;
    }
    co_return std::unexpected(
        std::format("Invalid numeric value {}", _reusableBuffer));
  }

  ParseEventDispatchT _eventDispatch;
  std::ispanstream _activeStream;
  bool _finalFrame { false }

public:
  ParseResult parseInputStream() {
    using ValidatorStack = std::stack<char>;
    ParseState parseState = ParseState::MemberName;
    bool usingRootArray = false;
    ValidatorStack vs;
    char c;
    if (auto result = co_await coWaitForValidStream(
            [&, this]() {
              return std::string("Unexpected end of frame starting json parse");
            },
            [&, this]() { _activeStream >> std::ws; },
            [&, this]() { _activeStream >> c; });
        !result) {
      co_return std::unexpected(result.error());
    }

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
    }

    if (c != '{') {
      co_return std::unexpected(
          std::format("Expected an object start, '{{' but got {}", c));
    }
    vs.push(c);
    auto result = _eventDispatch.onObjectStart();
    if (!result) {
      co_return std::unexpected(result.error());
    }

    while (!vs.empty()) {
      char c;
      switch (parseState) {
      case Object: {
        _activeStream >> c;
        if (c != '{') {
          co_return std::unexpected(
              std::format("Expected an object start, '{{' but got {}", c));
        }
        auto result = _eventDispatch.onObjectStart();
        if (!result) {
          co_return std::unexpected(result.error());
        }
        vs.push(c);
        parseState = ParseState::MemberName;
      }; break;
      case MemberName: {
        auto expectedText = getMemberText();
        if (!expectedText) {
          co_return std::unexpected(std::format("{}", expectedText.error()));
        }
        if (auto result = co_await coWaitForValidStream(
                [&, this]() {
                  return std::string(
                      "Unexpected end of  expected ':' in member definition");
                },
                [&, this]() { _activeStream >> std::ws; },
                [&, this]() { _activeStream >> c; });
            !result) {
          co_return std::unexpected(result.error());
        }
        if (c != ':') {
          co_return std::unexpected(
              std::format("Expected ':' in member definition but got {}", c));
        }
        parseState = ParseState::OpenValue;
        auto result = _eventDispatch.onMember(expectedText.value());
        if (!result) {
          co_return std::unexpected(result.error());
        }
      } break;
      case OpenValue: {
        if (auto result = co_await coWaitForValidStream(
                [&, this]() {
                  return std::string(
                      "Unexpected end of  expected ':' in member definition");
                },
                [&, this]() { _activeStream >> std::ws; },
                [&, this]() { c = _activeStream.peek(); });
            !result) {
          co_return std::unexpected(result.error());
        }

        switch (c) {
        case '{':
          parseState = ParseState::Object;
          continue;
          ;
        case '[': {
          parseState = ParseState::Array;
          auto result = _eventDispatch.onArrayStart();
          if (!result) {
            co_return std::unexpected(result.error());
          }
          continue;
        }
        case '"': {
          _activeStream.get(); // consume the opening quote
          auto expectedText = getStringValueText();
          if (!expectedText) {
            co_return std::unexpected(
                std::format("{} in jsonText", expectedText.error()));
          }
          auto result = _eventDispatch.onStringValue(expectedText.value());
          if (!result) {
            co_return std::unexpected(result.error());
          }
        } break;
        case 'n':
        case 't':
        case 'f': {
          // Boolean values
          std::string nullBoolText;
          while (isalpha(c)) {
            nullBoolText += _activeStream.get();
            if (auto result = co_await coWaitForValidStream(
                    [&, this]() {
                      return std::format("Unexpected end of frame while "
                                         "reading boolean content {}",
                                         nullBoolText);
                    },
                    [&, this]() { c = _activeStream.peek(); });
                !result) {
              co_return std::unexpected(result.error());
            }
          }
          if (nullBoolText == "true") {
            auto result = _eventDispatch.onBooleanValue(true);
            if (!result) {
              co_return std::unexpected(result.error());
            }
          } else if (nullBoolText == "false") {
            auto result = _eventDispatch.onBooleanValue(false);
            if (!result) {
              co_return std::unexpected(result.error());
            }
          } else if (nullBoolText == "null") {
            auto result = _eventDispatch.onNullValue();
            if (!result) {
              co_return std::unexpected(result.error());
            }
          } else {
            co_return std::unexpected(
                std::format("Invalid unquoted non numeric string, only 'true', "
                            "'false', and "
                            "'null' are valid RFC 7159 values'{}'",
                            nullBoolText));
          }
        } break;
        case ']':
        case '}':
          parseState = ParseState::CloseValue;
          continue;
          break;
        default:
          auto expectedText = co_await getNumericValueText();
          if (!expectedText) {
            co_return std::unexpected(std::format("{}", expectedText.error()));
          }
          auto result = _eventDispatch.onNumericValue(expectedText.value());
          if (!result) {
            co_return std::unexpected(result.error());
          }
        };
        parseState = ParseState::CloseValue;
        // We've processed an atomic value
      } break;
      case Array:
        _activeStream >> c;
        vs.push(c);
        parseState = ParseState::OpenValue;
        break;

      case CloseValue: {
        char c;
        _activeStream >> c;

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
            co_return std::unexpected(
                std::format("Invalid json parse, unexpected character, {}", c));
          }
          auto result = _eventDispatch.onArrayFinish();
          if (!result) {
            co_return std::unexpected(result.error());
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
        } break;
        case '}': {
          if (vs.top() != '{') {
            co_return std::unexpected(
                std::format("Invalid json parse, unexpected character, {}", c));
          }
          _eventDispatch.onObjectFinish();
          vs.pop();
        } break;
        };
      }
      };
    }
    co_return Expected{};
  }

  auto &getDispatcher() { return _eventDispatch; }
};

} // namespace moped