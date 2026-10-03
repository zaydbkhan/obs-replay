#include "obs-replay-api.h"

#include <util/platform.h>

#include "models.h"
#include "timeline.h"
#include "write-manager.h"

/**
 * Translation layer between the rest of the app and OBS, giving callers like the router a stable,
 * testable surface instead of talking to OBS directly.
 */

struct Core {
	Timeline *timeline;
	WriteManager *write_manager;
};

static struct Core *core = nullptr;

uint64_t obs_clock(void *data)
{
	return os_gettime_ns();
}

void obs_replay_core_init()
{
	core = new Core{};
	core->timeline = timeline_create(obs_clock, nullptr);
	core->write_manager = write_manager_create();
}

void obs_replay_core_destroy()
{
	write_manager_destroy(core->write_manager);
	delete core;
}

void obs_replay_core_start_recording() {}

void obs_replay_core_stop_recording() {}

void obs_replay_submit_frame(video_data *frame_data)
{
	auto frame = new Frame{
		timeline_translate(core->timeline, frame_data->timestamp),

	};

	submit_frame(core->write_manager);
}
