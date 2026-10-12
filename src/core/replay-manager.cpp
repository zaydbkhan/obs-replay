#include "replay-manager.h"
#include "write-manager.h"

/**
 * Manages the actual replays, which consist of timestamps, source #s (1-8), workspace #s (1-20),
 * and any other replay properties to be aware of (e.g. playback speed).
 */

ReplayManager *replay_manager_create() {
    ReplayManager* rm = new ReplayManager;
    return rm;
}
void replay_manager_destroy(ReplayManager *rm) {
    for(auto it : rm->replays) {
        delete(it.second);
    }
    delete rm;
}

std::optional<int> replay_manager_start_recording(ReplayManager *rm, WriteManager* wm, const std::string& source_name, Timeline *timeline)
{
	uint64_t start_time = timeline_now(timeline);
    Writer* videoWriter = write_manager_get_writer_for_source(wm, source_name, true);
    Writer* audioWriter = write_manager_get_writer_for_source(wm, source_name, false);

	Replay *r = replay_start_recording(videoWriter, start_time);
	if (r) {
		const std::string replay_name = replay_generate_unique_name(rm, source_name);
		r->id = replay_name;
        rm->replays[replay_name] = r;
		int index = rm->replays.size();
		return index;
	}
    return std::nullopt;
}

Replay *replay_start_recording(Writer *writer, uint64_t start_time)
{
	Replay *replay = new Replay();
	replay->writer = writer;
	replay->start_time = start_time;
    replay->is_recording = true;
	return replay;
}

void replay_stop_recording(Replay *replay, uint64_t end_time) {
    if(!replay->is_recording) return;
    replay->end_time = end_time;
    replay->is_recording = false;
}

std::string replay_generate_unique_name(ReplayManager* rm, const std::string& source_name) {
	std::string base_name = source_name + "_Replay";
	std::string candidate = base_name;

	int counter = 1;
	// Keep incrementing until the map does NOT contain the name
	while (rm->replays.find(candidate) != rm->replays.end()) {
		candidate = base_name + "_" + std::to_string(counter);
		counter++;
	}

	return candidate;
}