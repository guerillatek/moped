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
                           std::vector<std::string> &errors) {
    if (!_participating) {
      return std::unexpected("No active parse candidate type at the moment");
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
                           std::vector<std::string> &errors) {
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

} // namespace moped