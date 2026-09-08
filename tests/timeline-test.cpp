#include <gtest/gtest.h>

#include "timeline.h"

struct FakeClock {
	uint64_t now = 0;
};

static uint64_t fake_clock(void *data)
{
	return static_cast<FakeClock *>(data)->now;
}

class TimelineTest : public ::testing::Test {
protected:
	FakeClock clock;
	Timeline *timeline = nullptr;

	void SetUp() override { timeline = timeline_create(fake_clock, &clock); }
	void TearDown() override
	{
		if (timeline)
			timeline_destroy(timeline);
	}
};

TEST_F(TimelineTest, IsNotRecordingBeforeStart)
{
	EXPECT_FALSE(timeline_is_recording(timeline));
}

TEST_F(TimelineTest, IsRecordingAfterStart)
{
	timeline_start_recording(timeline);
	EXPECT_TRUE(timeline_is_recording(timeline));
}

TEST_F(TimelineTest, IsNotRecordingAfterStop)
{
	timeline_start_recording(timeline);
	timeline_stop_recording(timeline);
	EXPECT_FALSE(timeline_is_recording(timeline));
}

TEST_F(TimelineTest, NowReportsRawClockTime)
{
	clock.now = 1234;
	EXPECT_EQ(timeline_now(timeline), 1234u);

	clock.now = 5678;
	EXPECT_EQ(timeline_now(timeline), 5678u);
}

TEST_F(TimelineTest, TranslatesRelativeToRecordingStart)
{
	clock.now = 1000;
	timeline_start_recording(timeline);

	EXPECT_EQ(timeline_translate(timeline, 1000), 0u);
	EXPECT_EQ(timeline_translate(timeline, 1500), 500u);
	EXPECT_EQ(timeline_translate(timeline, 2000), 1000u);
}

TEST_F(TimelineTest, RecordedElapsedAdvancesWhileRecording)
{
	clock.now = 1000;
	timeline_start_recording(timeline);

	clock.now = 1600;
	EXPECT_EQ(timeline_recorded_elapsed(timeline), 600u);
}

TEST_F(TimelineTest, RecordedElapsedFreezesAtStop)
{
	clock.now = 1000;
	timeline_start_recording(timeline);
	clock.now = 1600;
	timeline_stop_recording(timeline);

	clock.now = 9900;
	EXPECT_EQ(timeline_recorded_elapsed(timeline), 600u);
}

TEST_F(TimelineTest, SecondSessionResumesAtFirstSessionEnd)
{
	// Session 1: 1000 -> 1600, i.e. 600ns recorded.
	clock.now = 1000;
	timeline_start_recording(timeline);
	clock.now = 1600;
	timeline_stop_recording(timeline);

	// One hour of real time passes with no frames arriving.
	clock.now += 3600 * 1000000000ull;

	// Session 2 must continue counting up from 600 as if no time passed.
	timeline_start_recording(timeline);
	EXPECT_EQ(timeline_translate(timeline, clock.now), 600u);

	clock.now += 400;
	EXPECT_EQ(timeline_translate(timeline, clock.now), 1000u);
	EXPECT_EQ(timeline_recorded_elapsed(timeline), 1000u);
}

TEST_F(TimelineTest, ThirdSessionResumesAtSecondSessionEnd)
{
	clock.now = 1000;
	timeline_start_recording(timeline);
	clock.now = 1600;
	timeline_stop_recording(timeline);

	clock.now = 5000; // gap
	timeline_start_recording(timeline);
	clock.now = 5400;
	timeline_stop_recording(timeline);

	clock.now = 90000; // bigger gap
	timeline_start_recording(timeline);

	EXPECT_EQ(timeline_translate(timeline, clock.now), 1000u);
}
