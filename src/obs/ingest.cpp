#include "ingest.h"

#include <obs.h>

#include "../core/obs-replay-api.h"

/**
 * Manages the OBS side of things for our writers, including calling the API when a
 * source changes, passing through obs video data, etc.
 */

#define INGEST_OUTPUT_ID "obs_replay_ingest_output"

struct Ingest {
	obs_source_t *source;
	obs_view_t *view;
	video_t *video;
	obs_encoder_t *encoder;
	obs_output_t *output;
};

bool get_first_source_callback(void *data, obs_source_t *source)
{
	auto **to_assign = static_cast<obs_source_t **>(data);
	*to_assign = obs_source_get_ref(source);
	return false;
}

static const char *ingest_output_get_name([[maybe_unused]] void *type_data)
{
	return "OBS Replay Ingest";
}

static void *ingest_output_create([[maybe_unused]] obs_data_t *settings, obs_output_t *output)
{
	return output;
}

static void ingest_output_destroy([[maybe_unused]] void *data) {}

static bool ingest_output_start(void *data)
{
	auto *output = static_cast<obs_output_t *>(data);
	if (!obs_output_can_begin_data_capture(output, 0))
		return false;
	if (!obs_output_initialize_encoders(output, 0))
		return false;
	return obs_output_begin_data_capture(output, 0);
}

static void ingest_output_stop(void *data, [[maybe_unused]] uint64_t ts)
{
	obs_output_end_data_capture(static_cast<obs_output_t *>(data));
}

static void ingest_output_encoded_packet([[maybe_unused]] void *data, encoder_packet *packet)
{
	obs_replay_submit_encoded_packet(packet);
}

void ingest_register_output()
{
	obs_output_info info{};
	info.id = INGEST_OUTPUT_ID;
	info.flags = OBS_OUTPUT_VIDEO | OBS_OUTPUT_ENCODED;
	info.get_name = ingest_output_get_name;
	info.create = ingest_output_create;
	info.destroy = ingest_output_destroy;
	info.start = ingest_output_start;
	info.stop = ingest_output_stop;
	info.encoded_packet = ingest_output_encoded_packet;
	obs_register_output(&info);
}

static obs_encoder_t *create_encoder(video_t *video)
{
	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "rate_control", "CRF");
	obs_data_set_int(settings, "crf", 18);
	obs_data_set_string(settings, "x264opts", "keyint=1 bframes=0");

	obs_encoder_t *encoder = obs_video_encoder_create("obs_x264", "obs-replay-ingest-encoder", settings, nullptr);
	obs_data_release(settings);

	obs_encoder_set_video(encoder, video);
	return encoder;
}

Ingest *ingest_create()
{
	obs_source_t *source = nullptr;
	obs_enum_sources(get_first_source_callback, &source);
	if (!source)
		return nullptr;

	auto *ingest = new Ingest{};
	ingest->source = source;

	obs_video_info video_info;
	obs_get_video_info(&video_info);
	video_info.base_height = obs_source_get_height(source);
	video_info.base_width = obs_source_get_width(source);
	video_info.output_height = video_info.base_height;
	video_info.output_width = video_info.base_width;
	video_info.output_format = VIDEO_FORMAT_NV12;

	ingest->view = obs_view_create();
	obs_view_set_source(ingest->view, 0, source);
	ingest->video = obs_view_add2(ingest->view, &video_info);

	ingest->encoder = create_encoder(ingest->video);
	ingest->output = obs_output_create(INGEST_OUTPUT_ID, "obs-replay-ingest-output", nullptr, nullptr);
	obs_output_set_video_encoder(ingest->output, ingest->encoder);

	obs_output_initialize_encoders(ingest->output, 0);
	obs_replay_update_source(ingest->encoder);
	obs_output_start(ingest->output);

	return ingest;
}

void ingest_destroy(Ingest *ingest)
{
	obs_output_force_stop(ingest->output);
	obs_output_release(ingest->output);
	obs_encoder_release(ingest->encoder);

	obs_view_set_source(ingest->view, 0, nullptr);
	obs_view_remove(ingest->view);
	obs_view_destroy(ingest->view);
	obs_source_release(ingest->source);
	delete ingest;
}
