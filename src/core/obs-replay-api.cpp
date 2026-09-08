#include "obs-replay-api.h"

#include "write-manager.h"

/**
 * Translation layer between the rest of the app and OBS, giving callers like the router a stable,
 * testable surface instead of talking to OBS directly.
 */

struct Core {
	WriteManager *write_manager;
};

static struct Core *core = nullptr;

void obs_replay_core_init()
{
	core = new Core{};
	core->write_manager = write_manager_create();
}

void obs_replay_core_destroy()
{
	write_manager_destroy(core->write_manager);
	delete core;
}

void obs_replay_core_start_recording() {}

void obs_replay_core_stop_recording() {}

void obs_replay_submit_frame([[maybe_unused]] video_data frame_data) {}
