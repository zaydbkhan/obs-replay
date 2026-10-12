#include "writer.h"

#include <cstring>

#include "constants.h"

/**
 * Two types (Audio/Video). Live in their own lower priority threads with small internal queues to
 * not disrupt/lag the rest of the process and minimize dropped frames. Break recordings into
 * segments after every 20 minutes, or when a source changes, and records timestamps via the
 * timeline API.
 */

static const AVRational TIMESTAMP_TIME_BASE = {1, 1000000000};

static const char *writer_type_name(WriterType type)
{
	return type == WRITER_VIDEO ? "video" : "audio";
}

static AVCodecID codec_id_from_video_codec(VideoCodec codec)
{
	switch (codec) {
	case VIDEO_CODEC_HEVC:
		return AV_CODEC_ID_HEVC;
	case VIDEO_CODEC_AV1:
		return AV_CODEC_ID_AV1;
	case VIDEO_CODEC_H264:
	default:
		return AV_CODEC_ID_H264;
	}
}

static int pixel_format_to_av(PixelFormat format)
{
	switch (format) {
	case PIXEL_FORMAT_NV12:
		return AV_PIX_FMT_NV12;
	case PIXEL_FORMAT_RGBA:
		return AV_PIX_FMT_RGBA;
	case PIXEL_FORMAT_I420:
	default:
		return AV_PIX_FMT_YUV420P;
	}
}

static void fill_codec_parameters(AVCodecParameters *codec_params, const VideoInfo *info)
{
	codec_params->codec_type = AVMEDIA_TYPE_VIDEO;
	codec_params->codec_id = codec_id_from_video_codec(info->codec);
	codec_params->format = pixel_format_to_av(info->format);
	codec_params->width = info->width;
	codec_params->height = info->height;

	if (info->extra_data_size > 0) {
		codec_params->extradata =
			static_cast<uint8_t *>(av_mallocz(info->extra_data_size + AV_INPUT_BUFFER_PADDING_SIZE));
		if (codec_params->extradata) {
			std::memcpy(codec_params->extradata, info->extra_data, info->extra_data_size);
			codec_params->extradata_size = info->extra_data_size;
		}
	}
}

static AVFormatContext *open_segment_muxer(const std::filesystem::path &path, const VideoInfo *info)
{
	const std::string path_string = path.string();

	AVFormatContext *fmt_ctx = nullptr;
	if (avformat_alloc_output_context2(&fmt_ctx, nullptr, "mp4", path_string.c_str()) < 0)
		return nullptr;

	AVStream *stream = avformat_new_stream(fmt_ctx, nullptr);
	if (!stream) {
		avformat_free_context(fmt_ctx);
		return nullptr;
	}
	stream->time_base = TIMESTAMP_TIME_BASE;
	fill_codec_parameters(stream->codecpar, info);

	if (avio_open(&fmt_ctx->pb, path_string.c_str(), AVIO_FLAG_WRITE) < 0) {
		avformat_free_context(fmt_ctx);
		return nullptr;
	}

	AVDictionary *options = nullptr;
	av_dict_set(&options, "movflags", "empty_moov+default_base_moof+frag_keyframe", 0);
	int ret = avformat_write_header(fmt_ctx, &options);
	av_dict_free(&options);

	if (ret < 0) {
		avio_closep(&fmt_ctx->pb);
		avformat_free_context(fmt_ctx);
		return nullptr;
	}
	return fmt_ctx;
}

static void close_segment_muxer(AVFormatContext *fmt_ctx)
{
	if (!fmt_ctx)
		return;

	av_write_trailer(fmt_ctx);
	avio_closep(&fmt_ctx->pb);
	avformat_free_context(fmt_ctx);
}

static void close_current_segment(Writer *writer)
{
	close_segment_muxer(writer->fmt_ctx);
	writer->fmt_ctx = nullptr;
	segment_destroy(writer->current_segment);
	writer->current_segment = nullptr;
}

static void write_packet(Writer *writer, const Packet *packet)
{
	if (!writer->segment_has_packets) {
		writer->current_segment->start_timestamp = packet->timestamp_ns;
		writer->segment_has_packets = true;
	}
	writer->current_segment->end_timestamp = packet->timestamp_ns;

	AVPacket *pkt = av_packet_alloc();
	if (!pkt)
		return;

	if (av_new_packet(pkt, static_cast<int>(packet->size)) == 0) {
		std::memcpy(pkt->data, packet->data, packet->size);

		int64_t segment_relative_ns =
			static_cast<int64_t>(packet->timestamp_ns - writer->current_segment->start_timestamp);
		pkt->pts = pkt->dts = segment_relative_ns;
		pkt->flags = AV_PKT_FLAG_KEY;
		pkt->stream_index = 0;
		av_packet_rescale_ts(pkt, TIMESTAMP_TIME_BASE, writer->fmt_ctx->streams[0]->time_base);

		av_interleaved_write_frame(writer->fmt_ctx, pkt);
	}

	av_packet_free(&pkt);
}

Writer *writer_create(WriterType type, int ordinal, const std::filesystem::path &directory_path)
{
	Writer *writer = new Writer{};
	writer->type = type;
	writer->ordinal = ordinal;
	writer->directory_path = directory_path;
	writer->base_file_name =
		SEGMENT_FILE_NAME_BASE + std::to_string(ordinal) + "_" + writer_type_name(type) + "_";
	return writer;
}

void writer_destroy(Writer *writer)
{
	close_current_segment(writer);
	delete writer;
}

void writer_update_source(Writer *writer, const VideoInfo *info)
{
	close_current_segment(writer);

	std::filesystem::path path =
		writer->directory_path / (writer->base_file_name + std::to_string(writer->segment_index++) + ".mp4");
	writer->current_segment = segment_create(path, 0);
	writer->current_segment->video_info = *info;
	writer->segment_has_packets = false;
	writer->fmt_ctx = open_segment_muxer(path, info);
}

void writer_submit_packet(Writer *writer, const Packet *packet)
{
	if (writer->fmt_ctx)
		write_packet(writer, packet);

	packet_destroy(packet);
}
