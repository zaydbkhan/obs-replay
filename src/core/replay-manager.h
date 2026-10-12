#pragma once

#include <unordered_map>
#include <optional>
#include "writer.h"
#include "timeline.h"


// Replay Manager
/**
 * Manages the actual replays, which consist of timestamps, source #s (1-8), workspace #s (1-20),
 * and any other replay properties to be aware of (e.g. playback speed).
 */



 // for now, work on only a single replay
 struct ReplayManager {
    std::unordered_map<std::string, Replay*> replays;
};

struct Replay {
    // the writer that wrote the source this replay comes from
    Writer* writer;
    // must be unique
    std::string id;
    uint64_t start_time;
    uint64_t end_time;
    bool is_recording;
};

ReplayManager* replay_manager_create();
void replay_manager_destroy(ReplayManager* rm);
// returns the id of the current replay
std::optional<int> replay_manager_start_recording(ReplayManager *rm, WriteManager *wm, const std::string &source_name,
						  Timeline *timeline);

Replay *replay_start_recording(Writer * writer);
void replay_stop_recording(Replay * replay);
std::string replay_generate_unique_name(ReplayManager* rm, const std::string &source_name);
