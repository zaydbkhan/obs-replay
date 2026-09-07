#include "segment-file.h"

/**
 * The literal files that the writers write to, and everything else reads from. Will have some
 * sort of consistent naming convention TBA, and if things get slow we can add some additional
 * infrastructure to reduce syscalls (but that's unlikely to be the main slowdown).
 */

SegmentFile *segment_file_create(const std::string &path, uint64_t start_timestamp)
{
	// TODO: open the file for writing and register it with the packet sidecar.
	return nullptr;
}

void segment_file_destroy(SegmentFile *segment_file)
{
	// TODO: flush and close the file.
}
