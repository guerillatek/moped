#pragma once

#include "concepts.hpp"

#include <iomanip>
#include <string>

#include <expected>
#include <sstream>
#include <string_view>
#include <tuple>

namespace moped {
template <typename PayloadHandlerT>
class ParsedObjectPayloadDispatcher : IMOPEDHandler {

  ParsedObjectPayloadDispatcher(PayloadHandlerT &&payloadHandler)
      : _payloadHandler{std::move(payloadHandler)} {}

  PayloadHandlerT _payloadHandler;

  void onMember(MOPEDHandlerStack &handlerStack,
                const std::string &memberName) {
    _objectPayload << '"' << memberName << '"';
  }
  void onObjectStart(MOPEDHandlerStack &handlerStack) {
    _objectPayload << '{';
    ++_objectNestingLevel
  }
  void onObjectFinish(MOPEDHandlerStack &handlerStack) {
    _objectPayload << '}';
    --_objectNestingLevel;
    if (_objectNestingLevel == 0) {
      // dispatch the payload
      _payloadHandler(_objectPayload.str());
      // clear the payload
      _objectPayload.clear();
      _objectPayload.str(std::string{});
      handlerStack.pop();
    }
  }
  void onArrayStart(MOPEDHandlerStack &handlerStack) { _objectPayload << '['; }
  void onArrayFinish(MOPEDHandlerStack &handlerStack) { _objectPayload << ']'; }
  void onStringValue(const std::string &value) {
    _objectPayload << '"';
    for (char c : value) {
      switch (c) {
      case '\"':
        _objectPayload << "\\\"";
        break;
      case '\\':
        _objectPayload << "\\\\";
        break;
      case '\b':
        _objectPayload << "\\b";
        break;
      case '\f':
        _objectPayload << "\\f";
        break;
      case '\n':
        _objectPayload << "\\n";
        break;
      case '\r':
        _objectPayload << "\\r";
        break;
      case '\t':
        _objectPayload << "\\t";
        break;
      default:
        if (static_cast<unsigned char>(c) < 0x20 ||
            static_cast<unsigned char>(c) > 0x7E) {
          oss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
              << static_cast<int>(static_cast<unsigned char>(c));
        } else {
          _objectPayload << c;
        }
      }
    }
    _objectPayload << '"';
  }

  void onNumericValue(const std::string &value) { _objectPayload << value; }

public:
  std::stringstream _objectPayload;
  std::uint32_t _objectNestingLevel = 0;
};

} // namespace moped
