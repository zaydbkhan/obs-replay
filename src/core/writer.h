#pragma once

#include <filesystem>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
}

#include "segment.h"

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
	int ordinal;
	std::filesystem::path directory_path;
	std::string base_file_name;
	uint32_t segment_index;
	Segment *current_segment;
	AVFormatContext *fmt_ctx;
	bool segment_has_packets;
};

Writer *writer_create(WriterType type, int ordinal, const std::filesystem::path &directory_path);
void writer_destroy(Writer *writer);

void writer_update_source(Writer *writer, const VideoInfo *info);

void writer_submit_packet(Writer *writer, const Packet *packet);
