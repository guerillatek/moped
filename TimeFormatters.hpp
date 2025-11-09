#pragma once
#include <chrono>
#include <format>
#include <string>
#include <time.h>

#include "concepts.hpp"

namespace moped {

template <typename FractionalDurationT = std::chrono::milliseconds>
class DefaultTimePointFormatter {
public:
  static std::string format(moped::TimePoint value) {
    auto fractionalUnits =
        std::chrono::duration_cast<FractionalDurationT>(epochTime).count();
    return std::to_string(fractionalUnits);
  }
};

class GMTFormatter {
public:
  static std::string format(moped::TimePoint value) {
    std::time_t timeT = std::chrono::system_clock::to_time_t(value);
    std::tm tm = *std::gmtime(&timeT);
    char buffer[30];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return std::string(buffer);
  }
};

template <typename FractionalDurationT = std::chrono::milliseconds>
class ISO8601Formatter {
public:
  static std::string format(moped::TimePoint value) {
    std::time_t timeT = std::chrono::system_clock::to_time_t(value);
    std::tm tm = *std::gmtime(&timeT);
    char buffer[30];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &tm);
    auto fractionalUnits = std::chrono::duration_cast<FractionalDurationT>(
                               value.time_since_epoch())
                               .count() %
                           FractionalDurationT::period::den;
    return std::format("{}.{}Z", buffer, fractionalUnits);
  }
};

template <typename FractionalDurationT = std::chrono::milliseconds>
class LocalTimeFormatter {
public:
  static std::string format(moped::TimePoint value) {
    std::time_t timeT = std::chrono::system_clock::to_time_t(value);
    std::tm tm = *std::localtime(&timeT);
    char buffer[30];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &tm);
    auto fractionalUnits = std::chrono::duration_cast<FractionalDurationT>(
                               value.time_since_epoch())
                               .count() %
                           FractionalDurationT::period::den;

    return std::format("{}.{}Z", buffer, fractionalUnits);
  }
};

} // namespace moped