#pragma once

#include <obs.h>
#include <string>
#include <functional>

// OBS Replay API
/**
 * Translation layer between the rest of the app and OBS, giving callers like the router a stable,
 * testable surface instead of talking to OBS directly.
 */

struct Segment;

struct ReplayPlaybackData {
	std::string replay_name;
	uint64_t start_timestamp_ns;
	uint64_t end_timestamp_ns;
	std::vector<Segment *> segments;
};

/** Initializes the core library. */
void obs_replay_core_init(const char *recording_path);

/** Tears down everything the core owns. */
void obs_replay_core_destroy();

/** Starts all writers recording. */
void obs_replay_core_start_recording();

/** Stops all writers recording, finalizing the current segment. */
void obs_replay_core_stop_recording();

/** Updates the writers with the video info of a new or changed source. */
void obs_replay_update_source(obs_encoder_t *encoder, std::string source_name);

/** Submit an encoded packet to the writer. */
void obs_replay_submit_encoded_packet(struct encoder_packet *obs_packet, const std::string& source_name);

/** Starts recording a replay on specified source */ 
std::optional<std::string> obs_replay_core_start_recording_replay(std::string source_name);
void obs_replay_core_stop_recording_replay(std::string replay_id);
void obs_replay_core_set_ui_callback(std::function<void(const std::string &)> cb);
std::optional<ReplayPlaybackData> obs_replay_core_get_replay_data(const std::string& replay_name);

