#include "moped/MappedEnumFlags.hpp"
#include <cstdint>
#include <format>
#include <moped/concepts.hpp>

#include <catch2/catch.hpp>

inline constexpr char flag1[] = "flag1";
inline constexpr char flag2[] = "flag2";
inline constexpr char flag3[] = "flag3";

enum class TestEnumFlag : std::uint8_t {
  Flag1 = 1 << 0,
  Flag2 = 1 << 1,
  Flag3 = 1 << 2
};

using TestMappedEnumT =
    moped::MappedEnum<flag1, TestEnumFlag::Flag1, flag2, TestEnumFlag::Flag2,
                      flag3, TestEnumFlag::Flag3>;

static_assert(moped::is_mapped_enum<TestMappedEnumT>,
              "TestMappedEnumT should be a MappedEnum type");

#define ASSERT_EQ(A, B) REQUIRE(A == B)

TEST_CASE("MappedEnum basic functionality tests",
          "[MAPPED ENUM BASIC FUNCTIONALITY]") {
  ASSERT_EQ(TestMappedEnumT{"flag1"}, TestMappedEnumT{TestEnumFlag::Flag1});

  ASSERT_EQ(TestMappedEnumT{"flag2"}, TestMappedEnumT{TestEnumFlag::Flag2});

  ASSERT_EQ(TestMappedEnumT{"flag3"}, TestMappedEnumT{TestEnumFlag::Flag3});

  TestMappedEnumT testEnum;
  CHECK_THROWS_AS(testEnum = "InvalidFlag", std::invalid_argument);

  // Check conversion to string
  std::string_view strValue = "flag3";
  testEnum = strValue;
  ASSERT_EQ(to_string(TestMappedEnumT{TestEnumFlag::Flag3}), "flag3");
}

TEST_CASE("MappedEnum associative operators tests",
          "[MAPPED ENUM ASSOCIATIVE OPERATORS]") {
  TestMappedEnumT enum1{TestEnumFlag::Flag1};
  TestMappedEnumT enum2{TestEnumFlag::Flag2};
  TestMappedEnumT enum3{TestEnumFlag::Flag3};

  REQUIRE(enum1 < enum2);
  REQUIRE(enum2 > enum1);
  REQUIRE(enum1 <= enum2);
  REQUIRE(enum2 >= enum1);
  REQUIRE(enum1 != enum2);
  REQUIRE(enum1 == TestMappedEnumT{TestEnumFlag::Flag1});
}

TEST_CASE("MappedEnum stream output tests", "[MAPPED ENUM STREAM OUTPUT]") {
  TestMappedEnumT testEnum{TestEnumFlag::Flag1};
  std::ostringstream oss;
  oss << testEnum;
  ASSERT_EQ(oss.str(), "flag1");

  testEnum = TestMappedEnumT{TestEnumFlag::Flag2};
  oss.str(""); // Clear the stream
  oss << testEnum;
  ASSERT_EQ(oss.str(), "flag2");

  testEnum = TestMappedEnumT{TestEnumFlag::Flag3};
  oss.str(""); // Clear the stream
  oss << testEnum;
  ASSERT_EQ(oss.str(), "flag3");
}

// Enum flags tests

using TestMappedEnumFlagsT = moped::MappedEnumFlags<TestMappedEnumT>;

static_assert(moped::IsMOPEDPushCollectionC<TestMappedEnumFlagsT>,
              "TestMappedEnumFlagsT should be a moped collection type");

TEST_CASE("MappedEnum flags basic functionality tests",
          "[MAPPED ENUM FLAGS BASIC FUNCTIONALITY]") {
  TestMappedEnumFlagsT flags;
  flags.emplace_back("flag1");
  flags.emplace_back("flag2");
  REQUIRE(flags.has_value(TestEnumFlag::Flag1));
  REQUIRE(flags.has_value(TestEnumFlag::Flag2));
  REQUIRE(!flags.has_value(TestEnumFlag::Flag3));
  REQUIRE(!flags.empty());
  flags.erase(TestEnumFlag::Flag1);
  REQUIRE(!flags.has_value(TestEnumFlag::Flag1));
  REQUIRE(flags.has_value(TestEnumFlag::Flag2));
  flags.clear();
  REQUIRE(flags.empty());
}
