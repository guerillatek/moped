#pragma once
#include "AutoTypeSelectingParserHandler.hpp"
#include "JSONStreamParser.hpp"
#include "JSONViewParser.hpp"
#include "PivotMemberTypeSelector.hpp"

namespace moped {

template <typename CandidateHarnessT, typename PivotMapT = EmptyPivotMapT>
class AutoTypeSelectingParserDispatcher {

  using ParserHandlerT = AutoTypeSelectingParserHandler<CandidateHarnessT>;
  ParserHandlerT _parserHandler;

  using PivotSelectorT = PivotMemberTypeSelector<ParserHandlerT, PivotMapT>;
  PivotSelectorT _pivotSelector{_parserHandler};

public:
  template <typename... Args>
  AutoTypeSelectingParserDispatcher(Args &&...args)
      : _parserHandler{std::forward<Args>(args)...} {}

  Expected parseAndDispatchJSONSteam(std::istream &jsonStream,
                                     auto &&handlerFunction) {
    JSONStreamParser<AutoTypeSelectingParserDispatcher> parser{*this};
    if (auto result = parser.parse(jsonStream); !result) {
      return std::unexpected(result.error());
    }
    auto result = _parserHandler.applyHandler(handlerFunction);
    if (!result) {
      return std::unexpected(result.error());
    }
    _parserHandler.reset();
    return result;
  }

  Expected parseAndDispatchJSONView(std::string_view jsonView,
                                    auto &&handlerFunction) {

    JSONViewParser<AutoTypeSelectingParserDispatcher> parser{*this};
    if (auto result = parser.parse(jsonView); !result) {
      return std::unexpected(result.error());
    }
    auto result = _parserHandler.applyHandler(handlerFunction);
    if (!result) {
      return std::unexpected(result.error());
    }
    return result;
  }

  template <typename... Args> void reset(Args &&...args) {
    _parserHandler.reset(std::forward<Args>(args)...);
    _pivotSelector.reset();
  }

  Expected onMember(std::string_view memberName) {
    if (!_pivotSelector.compositeSet()) {
      _pivotSelector.onMember(memberName);
    }
    return _parserHandler.onMember(memberName);
  }

  Expected onStringValue(std::string_view value) {
    if (!_pivotSelector.compositeSet()) {
      _pivotSelector.onStringValue(value);
    }
    return _parserHandler.onStringValue(value);
  }

  Expected onNumericValue(std::string_view value) {
    if (!_pivotSelector.compositeSet()) {
      _pivotSelector.onNumericValue(value);
    }
    return _parserHandler.onNumericValue(value);
  }

  Expected onObjectStart() { return _parserHandler.onObjectStart(); }
  Expected onObjectFinish() { return _parserHandler.onObjectFinish(); }
  Expected onArrayStart() { return _parserHandler.onArrayStart(); }
  Expected onArrayFinish() { return _parserHandler.onArrayFinish(); }
  Expected onBooleanValue(bool value) {
    return _parserHandler.onBooleanValue(value);
  }
};

} // namespace moped