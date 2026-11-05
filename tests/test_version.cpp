// Smoke test: proves GoogleTest is wired up and links against rtos_core.
#include <gtest/gtest.h>

#include "rtos/Version.hpp"

TEST(Version, IsSemVer) {
    EXPECT_EQ(rtos::version(), "0.1.0");
}
