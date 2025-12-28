#pragma once

#include "JSONStreamParser.hpp"
#include "JSONViewParser.hpp"
#include "moped/CompositeParseEventDispatcher.hpp"
#include "moped/concepts.hpp"

#include <type_traits>

namespace moped {

template <DecodingTraitsC DecodingTraits, typename... T>
struct CandidateTypeHarness {};

template <DecodingTraitsC DecodingTraits, typename T> struct LastHarness {

  template <typename... Args>
  LastHarness(Args &&...args)
      : _candidateMopedHandler{std::forward<Args>(args)...} {}

  using MOPEDHandlerT = CompositeParserEventDispatcher<T, DecodingTraits>;
  using DecodingTraitsT = DecodingTraits;
  MOPEDHandlerT _candidateMopedHandler;
  bool _participating = true;

  template <typename... Args> void reset(Args &&...args) {
    _participating = true;
    _candidateMopedHandler.reset(std::forward<Args>(args)...);
  }

  Expected applyParseEvent(auto &&parseEvent, int &activeParticipants,
                           std::vector<ParseError> &errors) {
    if (!_participating) {
      return std::unexpected("No active  candidate type at the moment");
    }
    auto result = parseEvent(_candidateMopedHandler);
    if (!result) {
      if (activeParticipants == 0) {
        errors.push_back(result.error());
        _participating = false;
      }
    } else {
      activeParticipants++;
    }
    return {};
  }

  inline Expected applyParseEvent(auto &&parseEvent) {
    if (!_participating) {
      return std::unexpected("No active candidate type at the moment");
    }
    return parseEvent(_candidateMopedHandler);
  }

  Expected applyHandler(auto &&handlerFunc) {
    if (!_participating) {
      return std::unexpected("No active candidate type at the moment");
    }
    return handlerFunc(_candidateMopedHandler.getComposite());
  }

  template <typename SetT> void setActiveComposite() {
    if constexpr (std::is_same_v<SetT, T>) {
      _participating = true;
    } else {
      _participating = false;
    }
  }
};

template <typename DecodingTraits, typename T, typename... TailTypes>
static constexpr auto GetCandidateHarnessType() {
  if constexpr (sizeof...(TailTypes) == 0) {
    return LastHarness<DecodingTraits, T>{};
  } else {
    return CandidateTypeHarness<DecodingTraits, T, TailTypes...>{};
  }
}
template <DecodingTraitsC DecodingTraits, typename T, typename... TailTypes>
struct CandidateTypeHarness<DecodingTraits, T, TailTypes...> {
  using MOPEDHandlerT = CompositeParserEventDispatcher<T, DecodingTraits>;

  using TailingCandidateHarness =
      decltype(GetCandidateHarnessType<DecodingTraits, TailTypes...>());
  using DecodingTraitsT = DecodingTraits;
  T _candidateComposite;
  MOPEDHandlerT _candidateMopedHandler;
  TailingCandidateHarness _tailingCandidates;
  bool _participating = true;

  template <typename... Args>
  CandidateTypeHarness(Args &&...args)
      : _candidateMopedHandler{std::forward<Args>(args)...},
        _tailingCandidates{std::forward<Args>(args)...} {}

  template <typename... Args> void reset(Args &&...args) {
    _participating = true;
    _candidateMopedHandler.reset(std::forward<Args>(args)...);
    _tailingCandidates.reset(std::forward<Args>(args)...);
  }

  template <typename SetT> void setActiveComposite() {
    if constexpr (std::is_same_v<SetT, T>) {
      _participating = true;
    } else {
      _participating = false;
    }
    _tailingCandidates.template setActiveComposite<SetT>();
  }

  Expected applyParseEvent(auto &&parseEvent, int &activeParticipants,
                           std::vector<ParseError> &errors) {
    if (!_participating) {
      return _tailingCandidates.applyParseEvent(parseEvent, activeParticipants,
                                                errors);
    }
    auto result = parseEvent(_candidateMopedHandler);
    if (!result) {
      if (activeParticipants == 0) {
        errors.push_back(result.error());
        _participating = false;
      }
    } else {
      activeParticipants++;
    }
    return _tailingCandidates.applyParseEvent(parseEvent, activeParticipants,
                                              errors);
  }

  inline Expected applyParseEvent(auto &&parseEvent) {
    if (_participating) {
      return parseEvent(_candidateMopedHandler);
    }
    return _tailingCandidates.applyParseEvent(parseEvent);
  }

  Expected applyHandler(auto &&handlerFunc) {
    if (!_participating) {
      return _tailingCandidates.applyHandler(handlerFunc);
    }
    return handlerFunc(_candidateMopedHandler.getComposite());
  }
};

template <typename CandidateHarnessT> struct AutoTypeSelectingParserHandler {
  using DecodingTraits = typename CandidateHarnessT::DecodingTraitsT;

public:
  template <typename... Args>
  AutoTypeSelectingParserHandler(Args &&...args)
      : _candidateHarness{std::forward<Args>(args)...} {}

  Expected onMember(std::string_view memberName) {
    if (_compositeSet) {
      return _candidateHarness.applyParseEvent(
          [&](auto &handler) { return handler.onMember(memberName); });
    }
    _errors.clear();
    int activeParticipants = 0;
    auto result = _candidateHarness.applyParseEvent(
        [&](auto &handler) { return handler.onMember(memberName); },
        activeParticipants, _errors);
    if ((activeParticipants == 0) && (!_errors.empty())) {
      return std::unexpected(_errors.front());
    }
    return {};
  }

  Expected onObjectStart() {
    if (_compositeSet) {
      return _candidateHarness.applyParseEvent(
          [&](auto &handler) { return handler.onObjectStart(); });
    }
    _errors.clear();
    int activeParticipants = 0;
    _candidateHarness.applyParseEvent(
        [&](auto &handler) { return handler.onObjectStart(); },
        activeParticipants, _errors);
    if ((activeParticipants == 0) && (!_errors.empty())) {
      return std::unexpected(_errors.front());
    }
    return {};
  }

  Expected onObjectFinish() {
    if (_compositeSet) {
      return _candidateHarness.applyParseEvent(
          [&](auto &handler) { return handler.onObjectFinish(); });
    }
    _errors.clear();
    int activeParticipants = 0;
    _candidateHarness.applyParseEvent(
        [&](auto &handler) { return handler.onObjectFinish(); },
        activeParticipants, _errors);
    if ((activeParticipants == 0) && (!_errors.empty())) {
      return std::unexpected(_errors.front());
    }
    return {};
  }

  Expected onArrayStart() {
    if (_compositeSet) {
      return _candidateHarness.applyParseEvent(
          [&](auto &handler) { return handler.onArrayStart(); });
    }
    _errors.clear();
    int activeParticipants = 0;
    auto result = _candidateHarness.applyParseEvent(
        [&](auto &handler) { return handler.onArrayStart(); },
        activeParticipants, _errors);
    if ((activeParticipants == 0) && (!_errors.empty())) {
      return std::unexpected(_errors.front());
    }
    return {};
  }

  Expected onArrayFinish() {
    if (_compositeSet) {
      return _candidateHarness.applyParseEvent(
          [&](auto &handler) { return handler.onArrayFinish(); });
    }
    _errors.clear();
    int activeParticipants = 0;
    _candidateHarness.applyParseEvent(
        [&](auto &handler) { return handler.onArrayFinish(); },
        activeParticipants, _errors);
    if ((activeParticipants == 0) && (!_errors.empty())) {
      return std::unexpected(_errors.front());
    }
    return {};
  }

  Expected onStringValue(std::string_view value) {
    if (_compositeSet) {
      return _candidateHarness.applyParseEvent(
          [&](auto &handler) { return handler.onStringValue(value); });
    }
    _errors.clear();
    int activeParticipants = 0;
    _candidateHarness.applyParseEvent(
        [&](auto &handler) { return handler.onStringValue(value); },
        activeParticipants, _errors);
    if ((activeParticipants == 0) && (!_errors.empty())) {
      return std::unexpected(_errors.front());
    }
    return {};
  }

  Expected onNumericValue(std::string_view value) {
    if (_compositeSet) {
      return _candidateHarness.applyParseEvent(
          [&](auto &handler) { return handler.onNumericValue(value); });
    }
    _errors.clear();
    int activeParticipants = 0;
    _candidateHarness.applyParseEvent(
        [&](auto &handler) { return handler.onNumericValue(value); },
        activeParticipants, _errors);
    if ((activeParticipants == 0) && (!_errors.empty())) {
      return std::unexpected(_errors.front());
    }
    return {};
  }

  Expected onBooleanValue(bool value) {
    if (_compositeSet) {
      return _candidateHarness.applyParseEvent(
          [&](auto &handler) { return handler.onBooleanValue(value); });
    }
    _errors.clear();
    int activeParticipants = 0;
    _candidateHarness.applyParseEvent(
        [&](auto &handler) { return handler.onBooleanValue(value); },
        activeParticipants, _errors);
    if ((activeParticipants == 0) && (!_errors.empty())) {
      return std::unexpected(_errors.front());
    }
    return {};
  }

  Expected onNullValue() {
    if (_compositeSet) {
      return _candidateHarness.applyParseEvent(
          [&](auto &handler) { return handler.onNullValue(); });
    }
    _errors.clear();
    int activeParticipants = 0;
    _candidateHarness.applyParseEvent(
        [&](auto &handler) { return handler.onNullValue(); },
        activeParticipants, _errors);
    if ((activeParticipants == 0) && (!_errors.empty())) {
      return std::unexpected(_errors.front());
    }
    return {};
  }

  template <typename... Args> void reset(Args &&...args) {
    _candidateHarness.reset(std::forward<Args>(args)...);
    _errors.clear();
    _compositeSet = false;
  }

  auto applyHandler(auto &&handlerFunc) {
    return _candidateHarness.applyHandler(handlerFunc);
  }

  template <typename SetT> void setActiveComposite() {
    _candidateHarness.template setActiveComposite<SetT>();
    _compositeSet = true;
  }

private:
  bool _compositeSet{false};
  std::vector<ParseError> _errors;
  CandidateHarnessT _candidateHarness;
};

} // namespace moped