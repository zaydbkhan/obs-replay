#include "writer.h"
#include "constants.h"
#include <string>
#include <optional>

/**
 * Two types (Audio/Video). Live in their own lower priority threads with small internal queues to
 * not disrupt/lag the rest of the process and minimize dropped frames. Break recordings into
 * segments after every 20 minutes, or when a source changes, and records timestamps via the
 * timeline API.
 */

Writer *writer_create(WriterType type, int ordinal, std::filesystem::path directory_path)
{
	Writer *writer = new Writer{};
	writer->type = type;

	// perhaps we may want to check this corresponds to a valid obs source slot later on
	writer->ordinal = ordinal;
	writer->base_file_name = SEGMENT_FILE_NAME_BASE + std::to_string(ordinal) + "_";

	writer->current_segment = segment_create(directory_path / (writer->base_file_name + "0"), 0);
	return writer;
}

void writer_destroy(Writer *writer)
{
	segment_destroy(writer->current_segment);
	delete writer;
}

std::optional<AVFormatContext*> init_segmented_fmp4_writer(std::string base_file_name, bool is_video,
					    AVCodecParameters *obs_codec_params)
{
    if(is_video) {
        // fill in later
    }
	AVFormatContext *fmt_ctx;
	avformat_alloc_output_context2(&fmt_ctx, nullptr, "segment", base_file_name.c_str());
	AVStream *stream = avformat_new_stream(fmt_ctx, nullptr);
	avcodec_parameters_copy(stream->codecpar, obs_codec_params);

	AVDictionary *opt = nullptr;
	av_dict_set(&opt, "segment_time", "1200", 0);
	av_dict_set(&opt, "segment_format", "mp4", 0);
	av_dict_set(&opt, "reset_timestamps", "1", 0);

	av_dict_set(&opt, "movflags", "empty_moov+default_base_moof+frag_keyframe", 0);

	avio_open(&fmt_ctx->pb, base_file_name.c_str(), AVIO_FLAG_WRITE);
	int ret = avformat_write_header(fmt_ctx, &opt);
    if (ret < 0) {
        // handle error
        return std::nullopt;
    }
    return fmt_ctx;
}

void writer_update_source(Writer *writer, const VideoInfo *info)
{
	writer->current_segment->video_info = *info;

	// create our codec params based on the info from OBS
	AVCodecParameters *codec_params = avcodec_parameters_alloc();

	codec_params->codec_type = AVMEDIA_TYPE_VIDEO;
	switch (info->codec) {
	case VIDEO_CODEC_H264:
		codec_params->codec_id = AV_CODEC_ID_H264;
		break;
	case VIDEO_CODEC_AV1:
		codec_params->codec_id = AV_CODEC_ID_AV1;
		break;
	case VIDEO_CODEC_HEVC:
		codec_params->codec_id = AV_CODEC_ID_HEVC;
		break;
	default:
        ;
		// handle this later
	}

	switch (info->format) {
	case PIXEL_FORMAT_I420:
		codec_params->format = AV_PIX_FMT_YUV420P;
		break;
	case PIXEL_FORMAT_NV12:
		codec_params->format = AV_PIX_FMT_NV12;
		break;
	case PIXEL_FORMAT_RGBA:
		codec_params->format = AV_PIX_FMT_RGBA;
		break;
	default:   
        ;
		// handle this later
	}

	codec_params->width = info->width;
	codec_params->height = info->height;

    if(auto ret = init_segmented_fmp4_writer(writer->current_segment->path.string(), true, codec_params))
    {
	    writer->fmt_ctx = *ret;
    } else {
        // handle error
    }
}

void writer_submit_packet([[maybe_unused]] Writer *writer, [[maybe_unused]] const Packet *packet)
{
	AVPacket *pkt = av_packet_alloc();
	if (!pkt) {
		// handle memory allocation failure
		return;
	}

	av_new_packet(pkt, packet->size);
	memcpy(pkt->data, packet->data, packet->size);

	pkt->dts = pkt->pts = packet->timestamp_ns;
	pkt->flags = AV_PKT_FLAG_KEY;
	pkt->stream_index = 0;

	int ret = av_interleaved_write_frame(writer->fmt_ctx, pkt);

	if (ret < 0) {
		// Handle error (e.g., print av_err2str(ret))
	}

    packet_destroy(packet);
	av_packet_free(&pkt);
}
