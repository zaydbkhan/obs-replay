#include "preview-source.h"
#include "../core/obs-replay-api.h"
#include <obs-frontend-api.h>
#include <plugin-support.h>
#include <util/platform.h>

/**
 * Replay playback, shows the replays, transitions between them, etc. Owns 3 (or more) private OBS
 * sources, including an A/B channel for replay mixing and a transition/stinger.
 */

static const char *replay_source_get_name([[maybe_unused]] void *type_data)
{
	return "Replay Preview Source";
}

static void *replay_source_create([[maybe_unused]] obs_data_t *settings, obs_source_t *source)
{
	auto *context = new ReplaySourceContext();
	context->source = source;

	const char *target_replay = obs_data_get_string(settings, "replay_target");

	if (target_replay && strlen(target_replay) > 0) {
		auto playback_data = obs_replay_core_get_replay_data(target_replay);

		if (playback_data) {
			context->segments_to_play = playback_data->segments;
			context->current_segment_index = 0;
			context->playing = true;

			context->playback_thread = std::thread(playback_thread_loop, context);
		} else {
			obs_log(LOG_WARNING, "Could not find replay data for: %s", target_replay);
		}
	}

	return context;
}

static void replay_source_destroy(void *data)
{
	auto *context = static_cast<ReplaySourceContext *>(data);

	if (context->playback_thread.joinable()) {
		context->playback_thread.join();
	}

	delete context;
}

struct obs_source_info replay_source_info = {};

void register_replay_source()
{
	replay_source_info.id = "obs_replay_playback";
	replay_source_info.type = OBS_SOURCE_TYPE_INPUT; // It's an input source

	// ASYNC_VIDEO is critical: it tells OBS this source will push frames at its own pace
	replay_source_info.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO;

	replay_source_info.get_name = replay_source_get_name;
	replay_source_info.create = replay_source_create;
	replay_source_info.destroy = replay_source_destroy;

	obs_register_source(&replay_source_info);
}

void push_frame_to_obs(ReplaySourceContext *context, uint8_t *video_data, uint64_t timestamp_ns)
{
	struct obs_source_frame frame = {};

	// Fill out the format based on how you saved it (likely NV12 based on your ingest)
	frame.format = VIDEO_FORMAT_NV12;
	frame.width = 1920;
	frame.height = 1080;

	// Pass the actual video memory buffer
	frame.data[0] = video_data;

	// The linesize is usually just the width for NV12 luma
	frame.linesize[0] = 1920;

	frame.timestamp = timestamp_ns;

	// Send the frame to OBS's render pipeline!
	obs_source_output_video(context->source, &frame);
}

void play_replay_in_current_scene(const std::string &replay_name)
{
	// 1. Get the current active scene
	obs_source_t *current_scene_source = obs_frontend_get_current_scene();
	obs_scene_t *scene = obs_scene_from_source(current_scene_source);

	// 2. Create custom playback source
	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "replay_target", replay_name.c_str());

	obs_source_t *playback_source = obs_source_create("obs_replay_playback", "Instant Replay", settings, nullptr);

	// 3. Add it to the scene
	obs_scene_add(scene, playback_source);

	// Clean up references
	obs_source_release(playback_source);
	obs_data_release(settings);
	obs_source_release(current_scene_source);
}

void playback_thread_loop(ReplaySourceContext *context)
{
	if (context->segments_to_play.empty())
		return;

	// 1. Open the first segment file written by your Writer
	Segment *current_segment = context->segments_to_play[context->current_segment_index];
	avformat_open_input(&context->fmt_ctx, current_segment->path.string().c_str(), nullptr, nullptr);
	avformat_find_stream_info(context->fmt_ctx, nullptr);

	// 2. Setup Decoder (Assuming stream 0 is video based on your writer code)
	AVCodecParameters *codecpar = context->fmt_ctx->streams[0]->codecpar;
	const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
	context->codec_ctx = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(context->codec_ctx, codecpar);
	avcodec_open2(context->codec_ctx, codec, nullptr);

	AVPacket *pkt = av_packet_alloc();
	AVFrame *frame = av_frame_alloc();

	// 3. The Read Loop
	uint64_t base_timestamp_ns = os_gettime_ns(); // Current OBS clock time

	while (context->playing) {
		if (av_read_frame(context->fmt_ctx, pkt) < 0) {
			// End of file. Here you would increment current_segment_index
			// and open the next segment in the vector if one exists!
			break;
		}

		if (pkt->stream_index == 0) {
			avcodec_send_packet(context->codec_ctx, pkt);

			while (avcodec_receive_frame(context->codec_ctx, frame) == 0) {
				// 4. Map FFmpeg frame to OBS frame
				struct obs_source_frame obs_frame = {};

				// Map the pixel format (Your writer uses NV12 mostly)
				obs_frame.format = VIDEO_FORMAT_NV12;
				obs_frame.width = frame->width;
				obs_frame.height = frame->height;

				// Copy the data pointers and line sizes
				for (int i = 0; i < MAX_AV_PLANES; i++) {
					obs_frame.data[i] = frame->data[i];
					obs_frame.linesize[i] = frame->linesize[i];
				}

				// Calculate when OBS should display this frame
				// pkt->pts is in the stream's time_base. Convert it to nanoseconds
				uint64_t frame_offset_ns =
					pkt->pts * av_q2d(context->fmt_ctx->streams[0]->time_base) * 1000000000ULL;
				obs_frame.timestamp = base_timestamp_ns + frame_offset_ns;

				// 5. Push to OBS!
				obs_source_output_video(context->source, &obs_frame);

				// Optional: Sleep briefly to avoid filling OBS's async buffer instantly
				// os_sleepto_ns(obs_frame.timestamp);
			}
		}
		av_packet_unref(pkt);
	}

	av_frame_free(&frame);
	av_packet_free(&pkt);
	// ... free contexts ...
}
