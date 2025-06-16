#include "moped/MappedEnumFlags.hpp"
#include <boost/test/unit_test.hpp>
#include <cstdint>
#include <format>
#include <moped/concepts.hpp>

inline constexpr char flag1[] = "flag1";
inline constexpr char flag2[] = "flag2";
inline constexpr char flag3[] = "flag3";

enum class TestEnumFlag : std::uint8_t {
  Flag1 = 1 << 0,
  Flag2 = 1 << 1,
  Flag3 = 1 << 2
};

using TestMappedEnumT =
    moped::MappedEnum<TestEnumFlag::Flag1, flag1, TestEnumFlag::Flag2, flag2,
                      TestEnumFlag::Flag3, flag3>;

static_assert(moped::is_mapped_enum<TestMappedEnumT>,
              "TestMappedEnumT should be a MappedEnum type");

struct TestFixture {};

BOOST_FIXTURE_TEST_SUITE(ScaledIntegertests, TestFixture);

BOOST_AUTO_TEST_CASE(EnumMemberTests) {
  BOOST_CHECK_EQUAL(TestMappedEnumT{"flag1"},
                    TestMappedEnumT{TestEnumFlag::Flag1});

  BOOST_CHECK_EQUAL(TestMappedEnumT{"flag2"},
                    TestMappedEnumT{TestEnumFlag::Flag2});

  BOOST_CHECK_EQUAL(TestMappedEnumT{"flag3"},
                    TestMappedEnumT{TestEnumFlag::Flag3});

  TestMappedEnumT testEnum;
  BOOST_CHECK_THROW(testEnum = "InvalidFlag", std::invalid_argument);

  // Check conversion to string
  std::string_view strValue = "flag3";
  testEnum = strValue;
  BOOST_CHECK_EQUAL(to_string(TestMappedEnumT{TestEnumFlag::Flag3}), "flag3");
}
BOOST_AUTO_TEST_CASE(EnumComparisonTests) {
  TestMappedEnumT enum1{TestEnumFlag::Flag1};
  TestMappedEnumT enum2{TestEnumFlag::Flag2};
  TestMappedEnumT enum3{TestEnumFlag::Flag3};

  BOOST_CHECK(enum1 < enum2);
  BOOST_CHECK(enum2 > enum1);
  BOOST_CHECK(enum1 <= enum2);
  BOOST_CHECK(enum2 >= enum1);
  BOOST_CHECK(enum1 != enum2);
  BOOST_CHECK(enum1 == TestMappedEnumT{TestEnumFlag::Flag1});
}
BOOST_AUTO_TEST_CASE(EnumStreamOutputTests) {
  TestMappedEnumT testEnum{TestEnumFlag::Flag1};
  std::ostringstream oss;
  oss << testEnum;
  BOOST_CHECK_EQUAL(oss.str(), "flag1");

  testEnum = TestMappedEnumT{TestEnumFlag::Flag2};
  oss.str(""); // Clear the stream
  oss << testEnum;
  BOOST_CHECK_EQUAL(oss.str(), "flag2");

  testEnum = TestMappedEnumT{TestEnumFlag::Flag3};
  oss.str(""); // Clear the stream
  oss << testEnum;
  BOOST_CHECK_EQUAL(oss.str(), "flag3");
}

// Enum flags tests

using TestMappedEnumFlagsT = moped::MappedEnumFlags<TestMappedEnumT>;


static_assert(moped::IsMOPEDPushCollectionC<TestMappedEnumFlagsT>,
              "TestMappedEnumFlagsT should be a moped collection type");

BOOST_AUTO_TEST_CASE(EnumFlagsTests) {
  TestMappedEnumFlagsT flags;
  flags.emplace_back("flag1");
  flags.emplace_back("flag2");
  BOOST_CHECK(flags.has_value(TestEnumFlag::Flag1));
  BOOST_CHECK(flags.has_value(TestEnumFlag::Flag2));
  BOOST_CHECK(!flags.has_value(TestEnumFlag::Flag3));
  BOOST_CHECK(!flags.empty());
  flags.erase(TestEnumFlag::Flag1);
  BOOST_CHECK(!flags.has_value(TestEnumFlag::Flag1));
  BOOST_CHECK(flags.has_value(TestEnumFlag::Flag2));
  flags.clear();
  BOOST_CHECK(flags.empty());
}

BOOST_AUTO_TEST_SUITE_END();