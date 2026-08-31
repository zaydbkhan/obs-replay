#include <gtest/gtest.h>

// Nothing in src/core or src/obs has real logic yet (they're doc-only
// stubs), so this just proves the GoogleTest/CTest wiring itself works.
// Replace/extend once those components have actual behavior.
TEST(Smoke, TestRunnerIsWired)
{
	EXPECT_TRUE(true);
}
