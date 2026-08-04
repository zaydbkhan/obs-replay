#include <filesystem>
#include <obs-source.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

struct ReplaySource {
	/*
    The source context data that is passed back and forth to OBS. As I understand it, each 
    function call (usually) passes this data in so that the functions themselves
    remain pure.
  */

	// handle back to self
	obs_source_t *source;

	uint32_t width;
	uint32_t height;

	AVFrame *frame;
};

// This is what OBS actually expects when loading a source. Constructed once and then never
// touched by us again -- purely exists to pass the necessary functions/settings to OBS
struct obs_source_info replay_source_info;

bool register_replay_source()
{
	replay_source_info.id = "replay-source";
	replay_source_info.type = OBS_SOURCE_TYPE_INPUT;
	replay_source_info.output_flags = OBS_SOURCE_VIDEO;
	replay_source_info.get_name = replay_source_name;
	replay_source_info.create = replay_source_create;
	replay_source_info.destroy = replay_source_destroy;
	replay_source_info.update = replay_source_update;
	replay_source_info.video_render = replay_source_render;
	replay_source_info.get_width = replay_source_width;
	replay_source_info.get_height = replay_source_height;

	obs_register_source(&replay_source_info);
	return true;
}

const static char *replay_source_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Instant Replay";
};

// TODO: fill the rest of these in

// https://github.com/leandromoreira/ffmpeg-libav-tutorial
static void *replay_source_create(obs_data_t *settings, obs_source_t *source)
{
	ReplaySource *replay_source = new ReplaySource();
	replay_source->source = source;

	// allocate format context, open file & get stream info
	AVFormatContext *format_ctx = avformat_alloc_context();
	avformat_open_input(&format_ctx, "/home/zayd/Dev/obs-replay/test_files/poc.mkv", NULL,
			    NULL); // replace w/ your path, idk i'm just hardcoding it for now
	avformat_find_stream_info(format_ctx, NULL);

	// determine which codec to use and open it
	const AVCodec *codec = NULL;
	AVCodecParameters *codec_params = NULL;
	AVCodecContext *codec_ctx = NULL;
	int video_stream_index = -1;

	// loop through streams to find video stream
	for (int i = 0; i < format_ctx->nb_streams; i++) {
		AVCodecParameters *tmp_codec_params = NULL;
		tmp_codec_params = format_ctx->streams[i]->codecpar;

		// Grab just video codec
		if (tmp_codec_params->codec_type == AVMEDIA_TYPE_VIDEO) {
			codec = avcodec_find_decoder(tmp_codec_params->codec_id);
			codec_params = tmp_codec_params;
			video_stream_index = i;
			break;
		}
	}

	replay_source->width = codec_params->width;
	replay_source->height = codec_params->height;

	codec_ctx = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(codec_ctx, codec_params);
	avcodec_open2(codec_ctx, codec, NULL);

	// read & decode a single frame (video frame, not av frame)
	AVPacket *packet = av_packet_alloc();
	AVFrame *frame = av_frame_alloc();

	// go through packets and decode one frame
	while (av_read_frame(format_ctx, packet) >= 0) {
		if (packet->stream_index == video_stream_index) {
			avcodec_send_packet(codec_ctx, packet);
			avcodec_receive_frame(codec_ctx, frame);
		}
	}

	// cleanup
	avformat_close_input(&format_ctx);
	avformat_free_context(format_ctx);
	avcodec_free_context(&codec_ctx);
	av_packet_free(&packet);

	replay_source->frame = frame;
	return replay_source;
}

static void replay_source_destroy(void *data)
{
	ReplaySource *replay_source = static_cast<ReplaySource *>(data);
	av_frame_free(&(replay_source->frame));
	delete replay_source;
	return;
};

static void replay_source_update(void *data, obs_data_t *settings)
{
	return;
};

static void replay_source_render(void *data, gs_effect_t *effect)
{
	return;
};

static uint32_t replay_source_width(void *data)
{
	ReplaySource *replay_source = static_cast<ReplaySource *>(data);
	return replay_source->width;
};

static uint32_t replay_source_height(void *data)
{
	ReplaySource *replay_source = static_cast<ReplaySource *>(data);
	return replay_source->height;
};