#pragma once
#include <regex>
#include <string>

namespace moped {

class ParserBase {

protected:
  enum ParseState { Object, MemberName, OpenValue, Array, CloseValue };

  static bool inline isNumericChar(char c) {
    switch (c) {
    case '.':
    case '-':
    case '+':
    case 'e':
    case 'E':
      return true;
    default:
      return isdigit(c);
    }
    return false;
  }

  static bool isNumeric(const std::string &text) {
    return std::regex_match(text, integerPattern) ||
           std::regex_match(text, floatPattern);
  }

  static bool isIdentifier(const std::string &text) {
    return std::regex_match(text, identifierPattern);
  }

protected:
  std::string _reusableBuffer;
  std::string _activeMemberName;

private:
  static std::regex integerPattern;
  static std::regex floatPattern;
  static std::regex identifierPattern;
};

} // namespace moped