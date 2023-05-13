#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN // REQUIRED: Enable custom main()
#include <doctest.h>

TEST_CASE("demo test") {
    CHECK(2 + 2 == 4);
}
