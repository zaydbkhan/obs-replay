#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <obs-module.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

struct MmapIO {
	uint8_t *base;
	size_t size;
	size_t pos;
};

struct ReplaySource {
	/*
    The source context data that is passed back and forth to OBS. As I understand it, each 
    function call (usually) passes this data in so that the functions themselves
    remain pure.
  */

	// handle back to self
	obs_source_t *source;

	int fd;
	struct MmapIO mmap_io;

	uint32_t width;
	uint32_t height;

	AVFrame *frame;

	gs_texture_t *tex = nullptr;
};

static const char *replay_source_name(void *unused);
static void *replay_source_create(obs_data_t *settings, obs_source_t *source);
static void replay_source_destroy(void *data);
static void replay_source_update(void *data, obs_data_t *settings);
static void replay_source_render(void *data, gs_effect_t *effect);
static uint32_t replay_source_width(void *data);
static uint32_t replay_source_height(void *data);

static int mmap_read(void *opaque, uint8_t *buf, int buf_size);
static int64_t mmap_seek(void *opaque, int64_t offset, int whence);

bool register_replay_source()
{
	// This is what OBS actually expects when loading a source. Constructed once and then never
	// touched by us again -- purely exists to pass the necessary functions/settings to OBS
	struct obs_source_info replay_source_info = {};

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

// https://github.com/leandromoreira/ffmpeg-libav-tutorial
static void *replay_source_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(settings);

	ReplaySource *replay_source = new ReplaySource();
	replay_source->source = source;

	const char *file_path = "/home/zayd/Dev/obs-replay/test_files/gorilla.mkv";

	// mmap file
	replay_source->fd = open(file_path, O_RDONLY);
	struct stat st;
	fstat(replay_source->fd, &st);
	replay_source->mmap_io.base =
		static_cast<uint8_t *>(mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, replay_source->fd, 0));
	replay_source->mmap_io.size = st.st_size;

	// custom AVIOContext for our mmaped scenario
	uint8_t *avio_buf = static_cast<uint8_t *>(av_malloc(4096));
	AVIOContext *avio_ctx =
		avio_alloc_context(avio_buf, 4096, 0, &(replay_source->mmap_io), mmap_read, nullptr, mmap_seek);

	// allocate format context, open file & get stream info
	AVFormatContext *format_ctx = avformat_alloc_context();
	format_ctx->pb = avio_ctx;
	avformat_open_input(&format_ctx, "", nullptr,
			    nullptr); // file is a nullptr because we pass it into the custom context earlier
	avformat_find_stream_info(format_ctx, NULL);

	// determine which codec to use and open it
	const AVCodec *codec = NULL;
	AVCodecParameters *codec_params = NULL;
	AVCodecContext *codec_ctx = NULL;
	int video_stream_index = -1;

	// loop through streams to find video stream
	for (uint32_t i = 0; i < format_ctx->nb_streams; i++) {
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
	// receive_frame returns EAGAIN until enough packets have been sent (B-frames,
	// etc.), so keep feeding packets until a frame actually comes out
	bool frame_decoded = false;
	while (!frame_decoded && av_read_frame(format_ctx, packet) >= 0) {
		if (packet->stream_index == video_stream_index) {
			avcodec_send_packet(codec_ctx, packet);
			if (avcodec_receive_frame(codec_ctx, frame) >= 0)
				frame_decoded = true;
		}
		av_packet_unref(packet);
	}

	// cleanup
	avformat_close_input(&format_ctx);
	avformat_free_context(format_ctx);
	av_freep(&avio_ctx->buffer); // must be freed before the context
	avio_context_free(&avio_ctx);
	avcodec_free_context(&codec_ctx);
	av_packet_free(&packet);

	replay_source->frame = frame;
	return replay_source;
}

static void replay_source_destroy(void *data)
{
	ReplaySource *replay_source = static_cast<ReplaySource *>(data);
	av_frame_free(&(replay_source->frame));

	// lock the graphics thread for thread-safe deletion
	if (replay_source->tex) {
		obs_enter_graphics();
		gs_texture_destroy(replay_source->tex);
		obs_leave_graphics();
	}

	// mmap cleanup
	munmap(replay_source->mmap_io.base, replay_source->mmap_io.size);
	close(replay_source->fd);

	delete replay_source;
};

static void replay_source_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
	return;
};

// key takeaway (approximately): texture is the image in memory, the effect is instructions on how to draw it
static void replay_source_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	ReplaySource *replay_source = static_cast<ReplaySource *>(data);

	// no decoded frame yet (e.g. hit EOF) -- nothing to draw, don't crash
	if (!replay_source->frame || !replay_source->frame->data[0])
		return;

	if (!replay_source->tex) {
		AVFrame *frame = replay_source->frame;

		// convert YUV to RGBA
		SwsContext *sws = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format,
						 frame->width, frame->height, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr,
						 nullptr, nullptr);

		uint8_t *rgba_data[4];
		int rgba_linesize[4];
		av_image_alloc(rgba_data, rgba_linesize, frame->width, frame->height, AV_PIX_FMT_RGBA, 1);
		sws_scale(sws, frame->data, frame->linesize, 0, frame->height, rgba_data, rgba_linesize);

		// create texture and bind the image to the texture
		replay_source->tex = gs_texture_create(frame->width, frame->height, GS_RGBA, 1, nullptr, GS_DYNAMIC);
		gs_texture_set_image(replay_source->tex, rgba_data[0], rgba_linesize[0], false);

		av_freep(&rgba_data[0]);
		sws_freeContext(sws);
	}

	// OBS has already begun the effect's "Draw" technique before calling
	// video_render, so draw directly with the active effect
	obs_source_draw(replay_source->tex, 0, 0, replay_source->width, replay_source->height, false);
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

// AVIOContext functions
static int mmap_read(void *opaque, uint8_t *buf, int buf_size)
{
	auto *io = static_cast<MmapIO *>(opaque);
	size_t remaining = io->size - io->pos;
	if (remaining == 0)
		return AVERROR_EOF;

	int n = std::min<size_t>(buf_size, remaining);
	memcpy(buf, io->base + io->pos, n);
	io->pos += n;
	return n;
}

static int64_t mmap_seek(void *opaque, int64_t offset, int whence)
{
	auto *io = static_cast<MmapIO *>(opaque);
	if (whence == AVSEEK_SIZE)
		return io->size;

	int64_t new_pos = (whence == SEEK_SET)   ? offset
			  : (whence == SEEK_CUR) ? io->pos + offset
			  : (whence == SEEK_END) ? io->size + offset
						 : -1;
	if (new_pos < 0 || (size_t)new_pos > io->size)
		return -1;
	io->pos = new_pos;
	return new_pos;
}