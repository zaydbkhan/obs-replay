#include "writer.h"
#include <string>

/**
 * Two types (Audio/Video). Live in their own lower priority threads with small internal queues to
 * not disrupt/lag the rest of the process and minimize dropped frames. Break recordings into
 * segments after every 20 minutes, or when a source changes, and records timestamps via the
 * timeline API.
 */

Writer *writer_create(WriterType type, int ordinal)
{
	Writer *writer = new Writer{};
	writer->type = type;
	writer->current_segment = segment_create("/home/zayd/Dev/obs-replay/test_files/test.fmp4", 0);
    // perhaps we may want to check this corresponds to a valid obs source slot later on
    writer->ordinal = ordinal;
    writer->base_file_name = SEGMENT_FILE_NAME_BASE + std::to_string(ordinal);
	return writer;
}

void writer_destroy(Writer *writer)
{
	segment_destroy(writer->current_segment);
	delete writer;
}

AVFormatContext* init_segmented_fmp4_writer(std::string base_file_name, bool is_video, AVCodecParameters *obs_codec_params) {
    AVFormatContext *fmt_ctx;
    avformat_alloc_output_context2(&fmt_ctx, nullptr, "segment", base_file_name.c_str());
    AVStream* stream = avformat_new_stream(fmt_ctx, nullptr);
    avcodec_parameters_copy(stream->codecpar, obs_codec_params);

    AVDictionary *opt = nullptr;
    av_dict_set(&opt, "segment_time", "1200", 0);
    av_dict_set(&opt, "segment_format", "mp4", 0);
    av_dict_set(&opt, "reset_timestamps", "1", 0);

    av_dict_set(&opt, "movflags", "empty_moov+default_base_moof+frag_keyframe", 0);

    avio_open(&fmt_ctx->pb, base_file_name.c_str(), AVIO_FLAG_WRITE);
    avformat_write_header(fmt_ctx, &opt);

}

void submit_frame([[maybe_unused]] Writer *writer)
{
    
	return;
}
