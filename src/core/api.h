#pragma once

#include <string>

// API
/**
 * Translation layer between the rest of the app and OBS, giving callers like the router a stable,
 * testable surface instead of talking to OBS directly.
 */

/** Settings for a recording, passed down the chain (write manager -> writers -> segment files). */
struct RecordingConfig {
	std::string recording_path;   // Directory segments are written into.
	uint32_t segment_length_seconds; // Max length of a segment before the writer rolls over.
	uint32_t source_id;           // Which source (1-8) this recording pipeline belongs to.
};

/** Initializes the core: spins up the write manager and writers for the given config. */
void obs_replay_core_init(const RecordingConfig *config);

/** Tears down everything the core owns. */
void obs_replay_core_destroy();

/** Starts all writers recording. */
void api_start_recording();

/** Stops all writers recording, finalizing the current segment. */
void api_stop_recording();
