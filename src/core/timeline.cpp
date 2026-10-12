#include "timeline.h"

/**
 * The single source of truth for time; also handles start/stop recording. The backing clock is
 * injected/mockable so the rest of the app can be tested without waiting on real time.
 */

struct Timeline {
	TimelineClock clock;
	void *clock_data;

	/** external_ns - core_offset = recording-relative ns. */
	uint64_t core_offset = 0;
	uint64_t recorded_elapsed = 0;
	bool recording = false;
};

Timeline *timeline_create(TimelineClock clock, void *clock_data)
{
	return new Timeline{clock, clock_data};
}

void timeline_destroy(Timeline *timeline)
{
	delete timeline;
}

void timeline_start_recording(Timeline *timeline)
{
	uint64_t now = timeline->clock(timeline->clock_data);
	timeline->core_offset = now - timeline->recorded_elapsed;
	timeline->recording = true;
}

void timeline_stop_recording(Timeline *timeline)
{
	timeline->recorded_elapsed = timeline_translate(timeline, timeline->clock(timeline->clock_data));
	timeline->recording = false;
}

bool timeline_is_recording(Timeline *timeline)
{
	return timeline->recording;
}

uint64_t timeline_recorded_elapsed(Timeline *timeline)
{
	if (!timeline->recording)
		return timeline->recorded_elapsed;
	return timeline_translate(timeline, timeline->clock(timeline->clock_data));
}

uint64_t timeline_now(Timeline *timeline)
{
	return timeline->clock(timeline->clock_data);
}

uint64_t timeline_translate(Timeline *timeline, uint64_t external_ns)
{
	return external_ns - timeline->core_offset;
}
