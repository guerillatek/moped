#pragma once

#include "moped/MappedObjectParseEncoderDispatcher.hpp"
#include "moped/TimeFormatters.hpp"

#include <expected>
namespace moped {

using Expected = std::expected<void, std::string>;

template <typename CompositeT, MemberIdTraitsC MemberIdTraits>
  requires IsMOPEDCompositeC<CompositeT, MemberIdTraits>
struct CompositeParserEventDispatcher {
  using CompositeMOPEDHandler = std::decay_t<
      decltype(getMOPEDHandlerForParser<CompositeT, MemberIdTraits>())>;
  using MOPEDHandlerStack = std::stack<IMOPEDHandler<MemberIdTraits> *>;

  template <typename... Args>
  CompositeParserEventDispatcher(Args &&...args)
      : _compositeMOPEDHandler{getMOPEDHandlerForParser<CompositeT,
                                                        MemberIdTraits>()},
        _composite{std::forward<Args>(args)...} {
    _compositeMOPEDHandler.setTargetMember(_composite);
  }

  Expected onMember(std::string_view memberName) {
    return _mopedHandlerStack.top()->onMember(_mopedHandlerStack, memberName);
  }
  Expected onObjectStart() {
    if (_mopedHandlerStack.empty()) {
      return _compositeMOPEDHandler.onObjectStart(_mopedHandlerStack);
    }
    return _mopedHandlerStack.top()->onObjectStart(_mopedHandlerStack);
  }
  Expected onObjectFinish() {
    return _mopedHandlerStack.top()->onObjectFinish(_mopedHandlerStack);
  }

  Expected onArrayStart() {
    return _mopedHandlerStack.top()->onArrayStart(_mopedHandlerStack);
  }
  Expected onArrayFinish() {
    return _mopedHandlerStack.top()->onArrayFinish(_mopedHandlerStack);
  }

  Expected onStringValue(std::string_view value) {
    return _mopedHandlerStack.top()->onStringValue(value);
  }

  Expected onNumericValue(std::string_view value) {
    return _mopedHandlerStack.top()->onNumericValue(value);
  }

  Expected onBooleanValue(bool value) {
    return _mopedHandlerStack.top()->onBooleanValue(value);
  }

  auto &&moveComposite() { return std::move(_composite); }

  auto &getComposite() { return _composite; }

  template <typename... Args> void reset(Args &&...args) {
    _composite = CompositeT{
        std::forward<Args>(args)...}; // Reset composite with new args
    while (!_mopedHandlerStack.empty()) {
      _mopedHandlerStack.pop();
    }
  }

private:
  CompositeMOPEDHandler _compositeMOPEDHandler;
  CompositeT _composite;
  MOPEDHandlerStack _mopedHandlerStack;
};

template <typename TimePointFormatter = DurationSinceEpochFormatter<>>
struct StringMemberIdTraits {
  using MemberIdType = std::string_view;
  static MemberIdType getMemberId(std::string_view name) { return name; }
  static std::string getDisplayName(MemberIdType memberId) {
    return std::string{memberId};
  }
  static constexpr std::string_view EmbeddingMemberId = "";
  using TimePointFormaterT = TimePointFormatter;
};

static_assert(MemberIdTraitsC<StringMemberIdTraits<>>,
              "StringMemberIdTraits should satisfy MemberIdTraitsC concept");

} // namespace moped