#pragma once
#include "AutoTypeSelectingParserHandler.hpp"
#include "JSONStreamParser.hpp"
#include "JSONViewParser.hpp"
#include "PivotMemberTypeSelector.hpp"

#include <tuple>

namespace moped {

template <typename CandidateHarnessT, typename... PivotMapTypesT>
class AutoTypeSelectingParserDispatcher {

  using ParserHandlerT = AutoTypeSelectingParserHandler<CandidateHarnessT>;
  ParserHandlerT _parserHandler;

  using PivotSelectorsT = std::tuple<PivotMapTypesT...>;
  using PivotSelectorStatesT =
      std::tuple<PivotMemberTypeSelector<ParserHandlerT, PivotMapTypesT>...>;
  static constexpr bool kHasPivotSelectors =
      (std::tuple_size_v<PivotSelectorsT> > 0);

  PivotSelectorStatesT _pivotSelectors{
      PivotMemberTypeSelector<ParserHandlerT, PivotMapTypesT>{
          _parserHandler}...};

public:
  template <typename... Args>
  AutoTypeSelectingParserDispatcher(Args &&...args)
      : _parserHandler{std::forward<Args>(args)...},
        _pivotSelectors{PivotMemberTypeSelector<ParserHandlerT, PivotMapTypesT>{
            _parserHandler}...} {}

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
    applyToPivotSelectors([](auto &selector) { selector.reset(); });
  }

  Expected onMember(std::string_view memberName) {
    applyToPivotSelectors(
        [&](auto &selector) { selector.onMember(memberName); });
    return _parserHandler.onMember(memberName);
  }

  Expected onStringValue(std::string_view value) {
    applyToPivotSelectors(
        [&](auto &selector) { selector.onStringValue(value); });
    return _parserHandler.onStringValue(value);
  }

  Expected onNumericValue(std::string_view value) {
    applyToPivotSelectors(
        [&](auto &selector) { selector.onNumericValue(value); });
    return _parserHandler.onNumericValue(value);
  }

  Expected onObjectStart() {
    applyToPivotSelectors([](auto &selector) { selector.onObjectStart(); });
    return _parserHandler.onObjectStart();
  }

  Expected onObjectFinish() {
    auto result = _parserHandler.onObjectFinish();
    applyToPivotSelectors([](auto &selector) { selector.onObjectFinish(); });
    return result;
  }
  Expected onArrayStart() { return _parserHandler.onArrayStart(); }
  Expected onArrayFinish() { return _parserHandler.onArrayFinish(); }
  Expected onBooleanValue(bool value) {
    return _parserHandler.onBooleanValue(value);
  }
  Expected onNullValue() { return _parserHandler.onNullValue(); }

private:
  template <typename CallableT>
  void applyToPivotSelectors(CallableT &&callable) {
    if constexpr (!kHasPivotSelectors) {
      return;
    } else {
      std::apply([&](auto &...selectors) { (callable(selectors), ...); },
                 _pivotSelectors);
    }
  }
};

} // namespace moped