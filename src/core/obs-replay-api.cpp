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

uint64_t obs_clock([[maybe_unused]] void *data)
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

static PixelFormat pixel_format_from_obs(video_format format)
{
	switch (format) {
	case VIDEO_FORMAT_NV12:
		return PIXEL_FORMAT_NV12;
	case VIDEO_FORMAT_RGBA:
		return PIXEL_FORMAT_RGBA;
	case VIDEO_FORMAT_I420:
	default:
		return PIXEL_FORMAT_I420;
	}
}

void obs_replay_update_source(video_t *video)
{
	const video_output_info *output_info = video_output_get_info(video);
	VideoInfo info{
		output_info->width,
		output_info->height,
		pixel_format_from_obs(output_info->format),
	};
	write_manager_update_source(core->write_manager, &info);
}

Frame frame_from_video_data(video_data *frame_data)
{
	Frame frame{};
	frame.timestamp_ns = timeline_translate(core->timeline, frame_data->timestamp);
	for (size_t i = 0; i < MAX_PLANES; i++) {
		frame.data[i] = frame_data->data[i];
		frame.linesize[i] = frame_data->linesize[i];
	}
	return frame;
}

void obs_replay_submit_frame(video_data *frame_data)
{
	[[maybe_unused]] Frame frame = frame_from_video_data(frame_data);
	submit_frame(core->write_manager);
}
