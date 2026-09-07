#pragma once

#include <cstdint>
#include <string>

// Files / Segments
/**
 * The literal files that the writers write to, and everything else reads from. Will have some
 * sort of consistent naming convention TBA, and if things get slow we can add some additional
 * infrastructure to reduce syscalls (but that's unlikely to be the main slowdown).
 */

struct SegmentFile {
	std::string path;
	uint64_t start_timestamp;
	uint64_t end_timestamp;
};

SegmentFile *segment_file_create(const std::string &path);
void segment_file_destroy(SegmentFile *segment_file);
