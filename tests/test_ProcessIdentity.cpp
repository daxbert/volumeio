#include <catch2/catch_test_macros.hpp>
#include "ProcessIdentity.h"

TEST_CASE("Synthetic identity equality", "[ProcessIdentity]") {
    ProcessIdentity a{1234, {0, 0}};
    ProcessIdentity b{1234, {0, 0}};
    REQUIRE(a == b);
    ProcessIdentity c{1234, {1, 0}};
    REQUIRE_FALSE(a == c);
}
