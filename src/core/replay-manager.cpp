#include "replay-manager.h"
#include "write-manager.h"
#include <plugin-support.h>
#include <obs.h>

/**
 * Manages the actual replays, which consist of timestamps, source #s (1-8), workspace #s (1-20),
 * and any other replay properties to be aware of (e.g. playback speed).
 */

ReplayManager *replay_manager_create()
{
	ReplayManager *rm = new ReplayManager;
	rm->ui_notify_callback = nullptr;
	return rm;
}
void replay_manager_destroy(ReplayManager *rm)
{
	for (auto it : rm->replays) {
		delete (it.second);
	}
	delete rm;
}

void replay_manager_set_ui_callback(ReplayManager *rm, std::function<void(const std::string &)> cb)
{
	rm->ui_notify_callback = cb;
}

std::optional<std::string> replay_manager_start_recording(ReplayManager *rm, WriteManager *wm,
							  const std::string &source_name, Timeline *timeline)
{
	uint64_t start_time = timeline_recorded_elapsed(timeline);
	Writer *videoWriter = write_manager_get_writer_for_source(wm, source_name, true);
	//Writer* audioWriter = write_manager_get_writer_for_source(wm, source_name, false);

	if (!videoWriter) {
		obs_log(LOG_WARNING,
			"Cannot start replay: No active video writer for source, will try to create one '%s'",
			source_name.c_str());
		write_manager_assign_source(wm, source_name, true);
		Writer *videoWriter = write_manager_get_writer_for_source(wm, source_name, true);

		if (!videoWriter) {
			obs_log(LOG_WARNING, "Failed to create writer for source: '%s'", source_name.c_str());
			return std::nullopt;
		}
	}

	Replay *r = replay_start_recording(videoWriter, start_time);
	if (r) {
		const std::string replay_name = replay_generate_unique_name(rm, source_name);
		r->id = replay_name;
		rm->replays[replay_name] = r;

		if (rm->ui_notify_callback) {
			rm->ui_notify_callback(replay_name);
		}

		return replay_name;
	}
	return std::nullopt;
}

void replay_manager_stop_recording(ReplayManager *rm, const std::string &replay_id, Timeline *timeline)
{
	uint64_t end_time = timeline_recorded_elapsed(timeline);

	Replay *r = replay_manager_get_replay(rm, replay_id);
	if (r) {
		replay_stop_recording(r, end_time);
	}
	return;
}

Replay *replay_manager_get_replay(ReplayManager *rm, const std::string &id)
{
	auto it = rm->replays.find(id);
	if (it != rm->replays.end()) {
		return it->second;
	}
	return nullptr;
}

Replay *replay_start_recording(Writer *writer, uint64_t start_time)
{
	Replay *replay = new Replay();
	replay->writer = writer;
	replay->start_time = start_time;
	replay->is_recording = true;
	return replay;
}

void replay_stop_recording(Replay *replay, uint64_t end_time)
{
	if (!replay->is_recording)
		return;
	replay->end_time = end_time;
	replay->is_recording = false;
}

std::string replay_generate_unique_name(ReplayManager *rm, const std::string &source_name)
{
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
