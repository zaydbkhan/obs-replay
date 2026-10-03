#pragma once

#include "segment.h"
#include <libavformat/avformat.h>

#define SEGMENT_FILE_NAME_BASE "replay_source_"

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
    std::string base_file_name;
};

Writer *writer_create(WriterType type, int ordinal);

void writer_destroy(Writer *writer);

AVFormatContext* init_segmented_writer(bool is_video, AVCodecParameters *obs_codec_params);

void submit_frame([[maybe_unused]] Writer *writer);