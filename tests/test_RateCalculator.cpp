#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "RateCalculator.h"

TEST_CASE("Rate scaling and zero-delta guard", "[RateCalculator]") {
    std::vector<ProcessStatsEntry> cur, prev;
    ProcessStatsEntry e{};
    e.ReadBytesTotal = 2000;
    cur.push_back(e);
    prev.push_back(ProcessStatsEntry{}); // zero previous

    std::vector<RateView> views;
    RateCalculator::Calculate(cur, prev, 2.0, views);
    REQUIRE(views[0].ReadPerSecond == Catch::Approx(1000.0));

    RateCalculator::Calculate(cur, prev, 0.0, views);
    REQUIRE(views[0].ReadPerSecond == Catch::Approx(2000.0)); // forced to 1.0
}
