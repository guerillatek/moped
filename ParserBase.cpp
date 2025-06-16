#include "ParserBase.hpp"

namespace moped {

std::regex ParserBase::integerPattern{R"(^[+-]?\d+$)",
                                          std::regex::optimize};
std::regex ParserBase::floatPattern{R"(^[+-]?\d*\.\d+([eE][+-]?\d+)?$)",
                                        std::regex::optimize};
std::regex ParserBase::identifierPattern{R"(^[A-Za-z_][A-Za-z0-9_]*$)",
                                             std::regex::optimize};
} // namespace moped
