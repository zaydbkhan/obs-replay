#pragma once

#include <unordered_map>
#include <optional>
#include <functional>
#include "writer.h"
#include "timeline.h"
#include "replay-manager.h"
#include "write-manager.h"



// Replay Manager
/**
 * Manages the actual replays, which consist of timestamps, source #s (1-8), workspace #s (1-20),
 * and any other replay properties to be aware of (e.g. playback speed).
 */




struct Replay {
    // the writer that wrote the source this replay comes from
    Writer* writer;
    // must be unique
    std::string id;
    uint64_t start_time;
    uint64_t end_time;
    bool is_recording;
};

// for now, work on only a single replay
struct ReplayManager {
	std::unordered_map<std::string, Replay *> replays;
	std::function<void(const std::string &)> ui_notify_callback;
};

ReplayManager* replay_manager_create();
void replay_manager_destroy(ReplayManager* rm);
void replay_manager_set_ui_callback(ReplayManager* rm, std::function<void(const std::string&)> cb);
// returns the id of the current replay
std::optional<std::string> replay_manager_start_recording(ReplayManager *rm, WriteManager *wm, const std::string &source_name,
						  Timeline *timeline);
void replay_manager_stop_recording(ReplayManager *rm,
							  const std::string &replay_id, Timeline *timeline);

Replay* replay_manager_get_replay(ReplayManager* rm, const std::string& id);
Replay *replay_start_recording(Writer * writer, uint64_t start_time);
void replay_stop_recording(Replay * replay, uint64_t end_time);
std::string replay_generate_unique_name(ReplayManager* rm, const std::string &source_name);


