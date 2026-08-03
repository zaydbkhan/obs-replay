#include <filesystem>
#include <obs-source.h>

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
	auto *replay_source = new ReplaySource();
	replay_source->source = source;

	// allocate format context, open file & get stream info
	AVFormatContext *fmt_ctx = avformat_alloc_context();
	avformat_open_input(&fmt_ctx, "/home/zayd/Dev/obs-replay/test_files/poc.mkv", NULL,
			    NULL); // replace w/ your path, idk i'm just hardcoding it for now
	avformat_find_stream_info(fmt_ctx, NULL);

	AVCodec *pCodec = NULL;
	// this component describes the properties of a codec used by the stream i
	// https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html
	AVCodecParameters *pCodecParameters = NULL;
	int video_stream_index = -1;

	return replay_source;
};

static void replay_source_destroy(void *data)
{
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
	return;
};

static uint32_t replay_source_height(void *data)
{
	return;
};