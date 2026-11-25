#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "RateCalculator.h"
#include "ProcessStats.h"

TEST_CASE("Rate scaling and zero-delta guard", "[RateCalculator]") {
    std::vector<ProcessStatsEntry> cur, prev;
    ProcessStatsEntry e{};
    e.ReadBytesTotal = 2000;
    cur.push_back(e);
    prev.push_back(ProcessStatsEntry{});   // previous = 0

    std::vector<RateView> views;

    // Normal delta
    RateCalculator::Calculate(cur, prev, 2.0, views);
    REQUIRE(views[0].ReadPerSecond == Catch::Approx(1000.0));

    // Zero-delta guard — reset previous so the byte delta is still 2000
    prev.clear();
    prev.push_back(ProcessStatsEntry{});
    RateCalculator::Calculate(cur, prev, 0.0, views);
    REQUIRE(views[0].ReadPerSecond == Catch::Approx(2000.0));
}
