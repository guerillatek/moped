#pragma once

#include "moped/MappedObjectParseEncoderDispatcher.hpp"

#include <expected>
namespace moped {

template <typename CompositeT, MemberIdTraitsC MemberIdTraits>
  requires IsMOPEDCompositeC<CompositeT, MemberIdTraits>
struct CompositeParserEventDispatcher {
  using CompositeMOPEDHandler = std::decay_t<
      decltype(CompositeT::template getMOPEDHandler<MemberIdTraits>())>;
  using MOPEDHandlerStack = std::stack<IMOPEDHandler<MemberIdTraits> *>;

  template <typename... Args>
  CompositeParserEventDispatcher(Args &&...args)
      : _compositeMOPEDHandler{CompositeT::template getMOPEDHandler<
            MemberIdTraits>()},
        _composite{std::forward<Args>(args)...} {
    _compositeMOPEDHandler.setTargetMember(_composite);
  }

  void onMember(std::string_view memberName) {
    _mopedHandlerStack.top()->onMember(_mopedHandlerStack, memberName);
  }
  void onObjectStart() {
    if (_mopedHandlerStack.empty()) {
      _compositeMOPEDHandler.onObjectStart(_mopedHandlerStack);
      return;
    }
    _mopedHandlerStack.top()->onObjectStart(_mopedHandlerStack);
  }
  void onObjectFinish() {
    _mopedHandlerStack.top()->onObjectFinish(_mopedHandlerStack);
  }

  void onArrayStart() {
    _mopedHandlerStack.top()->onArrayStart(_mopedHandlerStack);
  }
  void onArrayFinish() {
    _mopedHandlerStack.top()->onArrayFinish(_mopedHandlerStack);
  }

  void onStringValue(std::string_view value) {
    _mopedHandlerStack.top()->onStringValue(value);
  }

  void onNumericValue(std::string_view value) {
    _mopedHandlerStack.top()->onNumericValue(value);
  }

  void onBooleanValue(bool value) {
    _mopedHandlerStack.top()->onBooleanValue(value);
  }
  auto &&getComposite() { return std::move(_composite); }

private:
  CompositeMOPEDHandler _compositeMOPEDHandler;
  CompositeT _composite;
  MOPEDHandlerStack _mopedHandlerStack;
};

struct StringMemberIdTraits {
  using MemberIdType = std::string_view;
  static MemberIdType getMemberId(std::string_view name) { return name; }
  static std::string getDisplayName(MemberIdType memberId) {
    return std::string{memberId};
  }
};

static_assert(MemberIdTraitsC<StringMemberIdTraits>,
              "StringMemberIdTraits should satisfy MemberIdTraitsC concept");

} // namespace moped