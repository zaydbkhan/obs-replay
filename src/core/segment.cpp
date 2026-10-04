#include "segment.h"

/**
 * The literal files that the writers write to, and everything else reads from. Will have some
 * sort of consistent naming convention TBA, and if things get slow we can add some additional
 * infrastructure to reduce syscalls (but that's unlikely to be the main slowdown).
 */

Segment *segment_create(const std::filesystem::path &path, uint64_t start_timestamp)
{
	return new Segment{path, start_timestamp};
}

void segment_destroy([[maybe_unused]] Segment *segment)
{
	delete segment;
}
