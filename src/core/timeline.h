#pragma once

#include <stdint.h>

// Timeline
/**
 * The single source of truth for time; also handles start/stop recording. The backing clock is
 * injected/mockable so the rest of the app can be tested without waiting on real time.
 */

typedef uint64_t (*TimelineClock)(void *data);

struct Timeline;

Timeline *timeline_create(TimelineClock clock, void *clock_data);
void timeline_destroy(Timeline *timeline);

void timeline_start_recording(Timeline *timeline);
void timeline_stop_recording(Timeline *timeline);
bool timeline_is_recording(Timeline *timeline);

uint64_t timeline_recorded_elapsed(Timeline *timeline);

uint64_t timeline_now(Timeline *timeline);

uint64_t timeline_translate(Timeline *timeline, uint64_t external_ns);
