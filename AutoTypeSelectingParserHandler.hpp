#pragma once

#include "moped/concepts.hpp"
#include "moped/moped.hpp"
#include "JSONStreamParser.hpp"
#include "JSONViewParser.hpp"

#include <type_traits>

namespace moped {

template <DecodingTraitsC DecodingTraits, typename... T>
struct CandidateTypeHarness {};

template <DecodingTraitsC DecodingTraits, typename T> struct LastHarness {

  template <typename... Args>
  LastHarness(Args &&...args)
      : _candidateMopedHandler{std::forward<Args>(args)...} {}

  using MOPEDHandlerT = CompositeParserEventDispatcher<T, DecodingTraits>;
  MOPEDHandlerT _candidateMopedHandler;
  bool _participating = true;

  template <typename... Args> void reset(Args &&...args) {
    _participating = true;
    _candidateMopedHandler.reset(std::forward<Args>(args)...);
  }

  void applyParseEvent(auto &&parseEvent, int &activeParticipants,
                       std::vector<std::string> &errors) {
    if (!_participating) {
      return;
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
  }

  inline Expected applyParseEvent(auto &&parseEvent) {
    if (!_participating) {
      return std::unexpected("No active candidate type at the moment");
    }
    return parseEvent(_candidateMopedHandler);
  }

  auto applyTypeHandler(auto &&handlerFunc) {
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

template <DecodingTraitsC DecodingTraits, typename T, typename... TailTypes>
struct CandidateTypeHarness<DecodingTraits, T, TailTypes...> {
  using MOPEDHandlerT = CompositeParserEventDispatcher<T, DecodingTraits>;

  using TailingCandidateHarness =
      std::conditional_t<(sizeof...(TailTypes) == 0),
                         CandidateTypeHarness<DecodingTraits, TailTypes...>,
                         LastHarness<DecodingTraits, T>>;
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

  void applyParseEvent(auto &&parseEvent, int &activeParticipants,
                       std::vector<std::string> &errors) {
    if (!_participating) {
      _tailingCandidates.applyParseEvent(parseEvent, activeParticipants,
                                         errors);
      return;
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
    _tailingCandidates.applyParseEvent(parseEvent, activeParticipants, errors);
  }

  inline Expected applyParseEvent(auto &&parseEvent) {
    if (_participating) {
      return parseEvent(_candidateMopedHandler);
    }
    return _tailingCandidates.applyParseEvent(parseEvent);
  }

  auto applyTypeHandler(auto &&handlerFunc) {
    if (!_participating) {
      return _tailingCandidates.applyTypeHandler(handlerFunc);
    }
    return handlerFunc(_candidateMopedHandler.getComposite());
  }
};

template <DecodingTraitsC DecodingTraits, typename... CompositeTypes>
struct AutoTypeSelectingParserHandler {
  using CandidateHarness = std::conditional_t<
      (sizeof...(CompositeTypes) > 1),
      CandidateTypeHarness<DecodingTraits, CompositeTypes...>,
      LastHarness<DecodingTraits, CompositeTypes...>>;

  using MOPEDHandlerStack = std::stack<IMOPEDHandler<DecodingTraits> *>;

public:
  template <typename... Args>
  AutoTypeSelectingParserHandler(Args &&...args)
      : _candidateHarness{std::forward<Args>(args)...} {}

  Expected onMember(std::string_view memberName) {
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
    _errors.clear();
    int activeParticipants = 0;
    _candidateHarness.applyParseEvent(
        [&](auto &handler) { return handler.onObjectStart(); },
        activeParticipants, _errors);
    if ((activeParticipants == 0) && (!_errors.empty())) {
      return std::unexpected(_errors.front());
    }
  }

  Expected onObjectFinish() {
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

  template <typename... Args> void reset(Args &&...args) {
    _candidateHarness.reset(std::forward<Args>(args)...);
  }

private:
  std::vector<std::string> _errors;
  CandidateHarness _candidateHarness;
};


} // namespace moped