#include "api.h"

/**
 * Translation layer between the rest of the app and OBS, giving callers like the router a stable,
 * testable surface instead of talking to OBS directly.
 */

void obs_replay_core_init(const RecordingConfig *config)
{
	// TODO: create the write manager and its writers from the config.
}

void obs_replay_core_destroy()
{
	// TODO: destroy the write manager and its writers.
}

void api_start_recording()
{
	// TODO: tell the write manager to start recording.
}

void api_stop_recording()
{
	// TODO: tell the write manager to stop recording and finalize the current segment.
}
