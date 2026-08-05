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
#include "plugin-support.h"
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

	int video_stream_index;
	AVFormatContext *format_ctx;
	AVIOContext *avio_ctx;
	AVCodecContext *codec_ctx;
	AVPacket *packet;
	AVFrame *frame;

	SwsContext *sws = nullptr;
	gs_texture_t *tex = nullptr;
};

static const char *replay_source_name(void *unused);
static void *replay_source_create(obs_data_t *settings, obs_source_t *source);
static void replay_source_destroy(void *data);
static void replay_source_update(void *data, obs_data_t *settings);
static void replay_source_tick(void *data, float seconds);
static void replay_source_render(void *data, gs_effect_t *effect);
static uint32_t replay_source_width(void *data);
static uint32_t replay_source_height(void *data);

// helper functions for file mmap, creation of avio resources
// i don't necessarily love how these were broken up, they seem like
// they have some implicit dependencies on each other i.e. you need
// to call them in order, but it seemed necessary to break them up
// at least a little and i was too lazy to do myself
static void open_input(ReplaySource *rs, const char *path);
static void open_format(ReplaySource *rs);
static void open_codec(ReplaySource *rs);
static void open_sws(ReplaySource *rs);
static void open_texture(ReplaySource *rs);
static bool decode_next_frame(ReplaySource *rs);

// AVIOContext custom functions
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
	replay_source_info.video_tick = replay_source_tick;
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

static void *replay_source_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(settings);

	ReplaySource *rs = new ReplaySource();
	rs->source = source;

	const char *file_path = "/home/zayd/Dev/obs-replay/test_files/gorilla.mkv";

	open_input(rs, file_path);
	open_format(rs);
	open_codec(rs);
	open_sws(rs);
	open_texture(rs);

	// decode the first frame so there's something to render
	rs->frame = av_frame_alloc();
	decode_next_frame(rs);

	return rs;
}

static void replay_source_destroy(void *data)
{
	ReplaySource *rs = static_cast<ReplaySource *>(data);

	av_frame_free(&(rs->frame));
	av_packet_free(&rs->packet);
	avcodec_free_context(&rs->codec_ctx);
	// frees the format context; our custom AVIOContext (CUSTOM_IO) is left
	// untouched, so it is freed below
	avformat_close_input(&rs->format_ctx);
	av_freep(&rs->avio_ctx->buffer); // must be freed before the context
	avio_context_free(&rs->avio_ctx);

	// sws cleanup
	sws_freeContext(rs->sws);

	// lock the graphics thread for thread-safe deletion
	if (rs->tex) {
		obs_enter_graphics();
		gs_texture_destroy(rs->tex);
		obs_leave_graphics();
	}

	// mmap cleanup
	munmap(rs->mmap_io.base, rs->mmap_io.size);
	close(rs->fd);

	delete rs;
};

static void replay_source_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
	return;
};

// load the next frame. in practice this should take into account seconds
// but for this POC we assume it's called reasonably often and just advance the frame
// it might be called too fast so we get to see aaron gorilla.mkv in superspeed. eh good enough
static void replay_source_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	ReplaySource *rs = static_cast<ReplaySource *>(data);
	decode_next_frame(rs); // we don't care if this succeeds or fails for now
}

// key takeaway (approximately): texture is the image in memory, the effect is instructions on how to draw it
static void replay_source_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	ReplaySource *rs = static_cast<ReplaySource *>(data);

	// no decoded frame yet (e.g. hit EOF) -- nothing to draw, don't crash
	if (!rs->frame || !rs->frame->data[0])
		return;

	obs_log(LOG_INFO, "Render frame: %dx%d, format=%d, linesize[0]=%d, data[0]=%p", rs->frame->width,
		rs->frame->height, rs->frame->format, rs->frame->linesize[0], rs->frame->data[0]);

	uint8_t *rgba_data[4];
	int rgba_linesize[4];
	av_image_alloc(rgba_data, rgba_linesize, rs->frame->width, rs->frame->height, AV_PIX_FMT_RGBA, 1);
	sws_scale(rs->sws, rs->frame->data, rs->frame->linesize, 0, rs->frame->height, rgba_data, rgba_linesize);

	obs_log(LOG_INFO, "RGBA allocated: linesize=%d, size=%d", rgba_linesize[0],
		rgba_linesize[0] * rs->frame->height);

	gs_texture_set_image(rs->tex, rgba_data[0], rgba_linesize[0], false);

	av_freep(&rgba_data[0]);

	// OBS has already begun the effect's "Draw" technique before calling
	// video_render, so draw directly with the active effect
	obs_source_draw(rs->tex, 0, 0, rs->width, rs->height, false);
};

static uint32_t replay_source_width(void *data)
{
	ReplaySource *rs = static_cast<ReplaySource *>(data);
	return rs->width;
};

static uint32_t replay_source_height(void *data)
{
	ReplaySource *rs = static_cast<ReplaySource *>(data);
	return rs->height;
};

// https://github.com/leandromoreira/ffmpeg-libav-tutorial
static void open_input(ReplaySource *rs, const char *path)
{
	// mmap file
	rs->fd = open(path, O_RDONLY);
	struct stat st;
	fstat(rs->fd, &st);
	rs->mmap_io.base = static_cast<uint8_t *>(mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, rs->fd, 0));
	rs->mmap_io.size = st.st_size;
}

static void open_format(ReplaySource *rs)
{
	// custom AVIOContext for our mmaped scenario
	uint8_t *avio_buf = static_cast<uint8_t *>(av_malloc(4096));
	rs->avio_ctx = avio_alloc_context(avio_buf, 4096, 0, &(rs->mmap_io), mmap_read, nullptr, mmap_seek);

	// allocate format context, open file & get stream info
	rs->format_ctx = avformat_alloc_context();
	rs->format_ctx->pb = rs->avio_ctx;
	avformat_open_input(&rs->format_ctx, "", nullptr,
			    nullptr); // file is a nullptr because we pass it into the custom context earlier
	avformat_find_stream_info(rs->format_ctx, NULL);
}

static void open_codec(ReplaySource *rs)
{
	// determine which codec to use and open it
	const AVCodec *codec = NULL;

	// loop through streams to find video stream
	for (uint32_t i = 0; i < rs->format_ctx->nb_streams; i++) {
		AVCodecParameters *codec_params = rs->format_ctx->streams[i]->codecpar;

		// Grab just video codec
		if (codec_params->codec_type == AVMEDIA_TYPE_VIDEO) {
			codec = avcodec_find_decoder(codec_params->codec_id);
			rs->video_stream_index = i;
			rs->width = codec_params->width;
			rs->height = codec_params->height;

			obs_log(LOG_INFO, "Video stream found: %dx%d, codec: %s, pix_fmt: %d", rs->width, rs->height,
				codec->name, codec_params->format);

			rs->codec_ctx = avcodec_alloc_context3(codec);
			avcodec_parameters_to_context(rs->codec_ctx, codec_params);
			avcodec_open2(rs->codec_ctx, codec, NULL);

			obs_log(LOG_INFO, "Codec context opened: %dx%d, pix_fmt: %d, thread_count: %d",
				rs->codec_ctx->width, rs->codec_ctx->height, rs->codec_ctx->pix_fmt,
				rs->codec_ctx->thread_count);
			break;
		}
	}
}

// advances the playhead one frame; returns false once the file has been fully
// decoded (the last decoded frame stays in place, like a real player)
static bool decode_next_frame(ReplaySource *rs)
{
	if (!rs->packet)
		rs->packet = av_packet_alloc();

	// decode into a scratch frame -- avcodec_receive_frame unrefs the frame it
	// is given even when it returns EAGAIN, so the frame render is currently
	// showing must not be passed in directly
	AVFrame *tmp = av_frame_alloc();
	bool decoded = false;
	bool flushed = false;
	static int frame_count = 0;

	while (true) {
		int ret = avcodec_receive_frame(rs->codec_ctx, tmp);

		if (ret >= 0) {
			// got a frame -- hand it to the playhead
			obs_log(LOG_INFO, "Decoded frame %d: %dx%d, format=%d, linesize[0]=%d, pts=%ld", ++frame_count,
				tmp->width, tmp->height, tmp->format, tmp->linesize[0], tmp->pts);
			av_frame_move_ref(rs->frame, tmp);
			decoded = true;
			break;
		}
		if (ret == AVERROR_EOF)
			break; // codec fully drained -- no more frames

		// EAGAIN -- feed the codec more data. receive_frame can stay EAGAIN
		// across several packets (B-frames, etc.), so keep reading until a
		// frame actually comes out
		if (av_read_frame(rs->format_ctx, rs->packet) < 0) {
			// EOF -- flush the codec so buffered frames still come out
			if (!flushed) {
				avcodec_send_packet(rs->codec_ctx, NULL);
				flushed = true;
			}
			continue;
		}

		if (rs->packet->stream_index == rs->video_stream_index)
			avcodec_send_packet(rs->codec_ctx, rs->packet);
		av_packet_unref(rs->packet);
	}

	av_frame_free(&tmp);
	return decoded;
}

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

static void open_sws(ReplaySource *rs)
{
	obs_log(LOG_INFO, "Creating sws context: src %dx%d fmt=%d, dst %dx%d fmt=RGBA", rs->width, rs->height,
		rs->codec_ctx->pix_fmt, rs->width, rs->height);
	rs->sws = sws_getContext(rs->width, rs->height, rs->codec_ctx->pix_fmt, rs->width, rs->height, AV_PIX_FMT_RGBA,
				 SWS_BILINEAR, nullptr, nullptr, nullptr);
}

static void open_texture(ReplaySource *rs)
{
	obs_log(LOG_INFO, "Creating texture: %dx%d", rs->width, rs->height);
	obs_enter_graphics();
	rs->tex = gs_texture_create(rs->width, rs->height, GS_RGBA, 1, nullptr, GS_DYNAMIC);
	obs_leave_graphics();
}