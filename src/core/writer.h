#pragma once

#include "segment.h"
#include <libavformat/avformat.h>
#include <filesystem>


// Writers
/**
 * Two types (Audio/Video). Live in their own lower priority threads with small internal queues to
 * not disrupt/lag the rest of the process and minimize dropped frames. Break recordings into
 * segments after every 20 minutes, or when a source changes, and records timestamps via the
 * timeline API.
 */

enum WriterType {
	WRITER_VIDEO,
	WRITER_AUDIO,
};

struct Writer {
	WriterType type;
	Segment *current_segment;
    // Which source slot this writer corresponds to basically
    int ordinal;
    AVFormatContext* fmt_ctx;
    std::string base_file_name;
};

Writer *writer_create(WriterType type, int ordinal, std::filesystem::path directory_path);

void writer_destroy(Writer *writer);

AVFormatContext* init_segmented_writer(bool is_video, AVCodecParameters *obs_codec_params);
void writer_update_source(Writer *writer, const VideoInfo *info);

void writer_submit_packet([[maybe_unused]] Writer *writer, [[maybe_unused]] const Packet *packet);