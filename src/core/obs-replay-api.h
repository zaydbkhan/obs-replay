#pragma once

#include <obs.h>

// OBS Replay API
/**
 * Translation layer between the rest of the app and OBS, giving callers like the router a stable,
 * testable surface instead of talking to OBS directly.
 */

/** Initializes the core library. */
void obs_replay_core_init();

/** Tears down everything the core owns. */
void obs_replay_core_destroy();

/** Starts all writers recording. */
void obs_replay_core_start_recording();

/** Stops all writers recording, finalizing the current segment. */
void obs_replay_core_stop_recording();

/** Submit a frame to the writer. */
void obs_replay_submit_frame([[maybe_unused]] video_data *frame_data);