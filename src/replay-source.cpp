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

	// placeholder, can use whatever method of loading the file you want
	// as long as it gets mapped into memory
	std::filesystem::path fpath;

	// mmap information, not comprehensive (maybe want start/end/current instead)
	void *mmap_base;
	size_t mmap_len;

	// demux/decode state (this is an example from claude but honestly idk what goes here yet)
	AVIOContext *avio_ctx;
	AVFormatContext *fmt_ctx;
	AVCodecContext *codec_ctx;
	SwsContext *sws_ctx;

	// GPU resource -- used to manage frame rendering through OBS's graphics library
	// for now I think we can render frames inline rather than in a separate thread/worker
	gs_texture_t *tex;

	// the width and height of the source, don't load these until you actually know
	// the dimensions of the video (return 0 initially)
	uint32_t width, height;
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
	return "Replay";
};

// TODO: fill the rest of these in
static void *replay_source_create(obs_data_t *settings, obs_source_t *source)
{
	return;
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