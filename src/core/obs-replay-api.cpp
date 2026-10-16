#include "obs-replay-api.h"

#include <util/platform.h>

#include "models.h"
#include "obs-mappings.h"
#include "timeline.h"
#include "write-manager.h"
#include "replay-manager.h"
#include "../plugin-support.h"



/**
 * Translation layer between the rest of the app and OBS, giving callers like the router a stable,
 * testable surface instead of talking to OBS directly.
 */

struct Segment;

struct Core {
	Timeline *timeline;
	WriteManager *write_manager;
    ReplayManager *replay_manager;

    std::function<void(const std::string &)> ui_callback = nullptr;
};

static struct Core *core = nullptr;

uint64_t obs_clock([[maybe_unused]] void *data)
{
	return os_gettime_ns();
}

void obs_replay_core_init(const char *recording_path)
{
	core = new Core{};
	core->timeline = timeline_create(obs_clock, nullptr);
	core->write_manager = write_manager_create(recording_path ? recording_path : "");
    core->replay_manager = replay_manager_create();
}

void obs_replay_core_destroy()
{
	timeline_destroy(core->timeline);
	write_manager_destroy(core->write_manager);
	delete core;
}

void obs_replay_core_start_recording()
{
	timeline_start_recording(core->timeline);
}

void obs_replay_core_stop_recording()
{
	timeline_stop_recording(core->timeline);
}

void obs_replay_update_source(obs_encoder_t *encoder, std::string source_name)
{
	const VideoInfo info = video_info_from_encoder(encoder);
	write_manager_update_source(core->write_manager, source_name, &info);
}

void obs_replay_submit_encoded_packet(encoder_packet *obs_packet, const std::string& source_name)
{
	if (!timeline_is_recording(core->timeline))
		return;

	const Packet *packet = packet_from_obs_packet(core->timeline, obs_packet);
	write_manager_submit_packet(core->write_manager, source_name, packet);
}

std::optional<std::string> obs_replay_core_start_recording_replay(std::string source_name) {
    auto replay_name = replay_manager_start_recording(core->replay_manager, core->write_manager, source_name, core->timeline);
    if(replay_name) {
        if(core->ui_callback) {
            core->ui_callback(*replay_name);
        }
        return replay_name;
    }
    else {
        obs_log(LOG_WARNING, "Failed to start replay");
        return std::nullopt;
    }
}

void obs_replay_core_stop_recording_replay(std::string replay_id)
{
	if (!core || !core->replay_manager)
		return;

	replay_manager_stop_recording(core->replay_manager, replay_id, core->timeline);

	obs_log(LOG_INFO, "Stopped replay %s", replay_id.c_str());
}

void obs_replay_core_set_ui_callback(std::function<void(const std::string &)> cb)
{
	if (core) {
		core->ui_callback = cb;
	}
}

std::optional<ReplayPlaybackData> obs_replay_core_get_replay_data(const std::string &replay_name) {
    if (!core || !core->replay_manager) return std::nullopt;

    Replay *replay = replay_manager_get_replay(core->replay_manager, replay_name);

    if(!replay || !replay->writer || replay->is_recording) return std::nullopt;

    ReplayPlaybackData replay_data = ReplayPlaybackData();

    replay_data.start_timestamp_ns = replay->start_time;
    replay_data.end_timestamp_ns = replay->end_time;
    replay_data.replay_name = replay->id;

    std::vector<Segment*> segments_to_play = packet_sidecar_get_segments_for_timeframe(core->write_manager->ps, replay->writer->ordinal, replay->start_time, replay->end_time);

    replay_data.segments = segments_to_play;
    return replay_data;


}
