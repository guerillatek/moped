#pragma once

#include <yaml.h>

#include "ParserBase.hpp"
#include "concepts.hpp"
#include <format>

namespace moped {

template <IParserEventDispatchC ParseEventDispatchT>
class YAMLParser : public ParserBase {
public:
  using ExpectedText = std::expected<std::string_view, std::string>;

  YAMLParser() {
    yaml_parser_initialize(&_parser);
    _parser.read_handler = nullptr;
    _parser.read_handler_data = nullptr;
  }

  ~YAMLParser() { yaml_parser_delete(&_parser); }

  auto &getDispatcher() { return _eventDispatch; }

  Expected parseInputView(std::string_view input) {
    enum class Context { MappingKey, MappingValue, Sequence, None };
    std::stack<Context> contextStack;
    std::optional<std::string_view> lastKey;

    auto parseYAMLEvent = [this]() {
      return yaml_parser_parse(&_parser, &_event) &&
             _parser.error == YAML_NO_ERROR &&
             _parser.state != YAML_PARSE_END_STATE;
    };

    yaml_parser_set_input_string(
        &_parser, reinterpret_cast<const unsigned char *>(input.data()),
        input.size());
    while (true) {
      if (!parseYAMLEvent()) {
        return std::unexpected(
            std::format("YAML parse error: {}", getErrorText()));
      }
      switch (_event.type) {
      case YAML_STREAM_START_EVENT:
      case YAML_STREAM_END_EVENT:
      case YAML_NO_EVENT:
      case YAML_ALIAS_EVENT:
      case YAML_DOCUMENT_START_EVENT:
      case YAML_DOCUMENT_END_EVENT: {
        // No-op for these events
        break;
      }
      case YAML_MAPPING_START_EVENT: {
        auto objectStartResult = _eventDispatch.onObjectStart();
        if (!objectStartResult) {
          return objectStartResult; // Both are Expected, return directly
        }
        contextStack.push(Context::MappingKey);
        break;
      }
      case YAML_MAPPING_END_EVENT: {
        auto objectFinishResult = _eventDispatch.onObjectFinish();
        if (!objectFinishResult) {
          return objectFinishResult; // Both are Expected, return directly
        }
        if (!contextStack.empty())
          contextStack.pop();
        if ((!contextStack.empty()) &&
            (contextStack.top() == Context::MappingValue)) {
          contextStack.pop();
        }
        break;
      }

      case YAML_SEQUENCE_START_EVENT: {
        auto arrayStartResult = _eventDispatch.onArrayStart();
        if (!arrayStartResult) {
          return arrayStartResult; // Both are Expected, return directly
        }
        contextStack.push(Context::Sequence);
        break;
      }
      case YAML_SEQUENCE_END_EVENT: {
        auto arrayFinishResult = _eventDispatch.onArrayFinish();
        if (!arrayFinishResult) {
          return arrayFinishResult; // Both are Expected, return directly
        }
        if (!contextStack.empty())
          contextStack.pop();
        if ((!contextStack.empty()) &&
            (contextStack.top() == Context::MappingValue)) {
          contextStack.pop();
        }
        break;
      }
      case YAML_SCALAR_EVENT: {
        std::string_view value(
            reinterpret_cast<const char *>(_event.data.scalar.value),
            _event.data.scalar.length);

        // Are we in a mapping and expecting a key?
        if (!contextStack.empty() &&
            contextStack.top() == Context::MappingKey) {
          lastKey = value;
          auto memberResult = _eventDispatch.onMember(value);
          if (!memberResult) {
            return memberResult; // Both are Expected, return directly
          }
          contextStack.push(Context::MappingValue);
        } else {
          // Value position (either mapping value or sequence element)
          std::string value_str(value);
          if (value == "true" || value == "false") {
            auto boolResult = _eventDispatch.onBooleanValue(value == "true");
            if (!boolResult) {
              return boolResult; // Both are Expected, return directly
            }
          } else if (ParserBase::isNumeric(value_str)) {
            auto numericResult = _eventDispatch.onNumericValue(value);
            if (!numericResult) {
              return numericResult; // Both are Expected, return directly
            }
          } else {
            auto stringResult = _eventDispatch.onStringValue(value);
            if (!stringResult) {
              return stringResult; // Both are Expected, return directly
            }
          }
          // If we just finished a mapping value, expect a key next
          if (!contextStack.empty() &&
              contextStack.top() == Context::MappingValue) {
            contextStack.pop();
          }
        }
        break;
      }
      };
      // End of YAML document or stream
      if (_event.type == YAML_STREAM_END_EVENT ||
          _event.type == YAML_DOCUMENT_END_EVENT)
        break;
    }
    return {};
  }

private:
  yaml_parser_t _parser;
  yaml_event_t _event;
  ParseEventDispatchT _eventDispatch;

  std::string getErrorText() {
    switch (_parser.error) {
    case YAML_NO_ERROR:
      return "NO ERROR";
    case YAML_MEMORY_ERROR:
      return "MEMORY ALLOCATION ERROR";
    case YAML_READER_ERROR:
      return "FAILED TO READ OR DECODE THE INPUT STREAM";
    case YAML_SCANNER_ERROR:
      return "FAILED TO SCAN THE INPUT STREAM";
    case YAML_PARSER_ERROR:
      return "FAILED TO PARSE THE INPUT STREAM";
    case YAML_COMPOSER_ERROR:
      return "FAILED TO COMPOSE A YAML DOCUMENT";
    case YAML_WRITER_ERROR:
      return "FAILED TO WRITE TO THE OUTPUT STREAM";
    case YAML_EMITTER_ERROR:
      return "FAILED TO EMIT A YAML STREAM";
    };
    return std::format("Unknown error error code ={}",
                       std::to_underlying(_parser.error));
  }
};

} // namespace moped