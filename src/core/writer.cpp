#include "writer.h"

/**
 * Two types (Audio/Video). Live in their own lower priority threads with small internal queues to
 * not disrupt/lag the rest of the process and minimize dropped frames. Break recordings into
 * segments after every 20 minutes, or when a source changes, and records timestamps via the
 * timeline API.
 */

Writer *writer_create(WriterType type)
{
	Writer *writer = new Writer{};
	writer->type = type;
	writer->current_segment = segment_create("/home/zayd/Dev/obs-replay/test_files/test.fmp4", 0);
	return writer;
}

void writer_destroy(Writer *writer)
{
	segment_destroy(writer->current_segment);
	delete writer;
}

void submit_frame([[maybe_unused]] Writer *writer)
{
	return;
}
