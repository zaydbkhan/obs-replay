#include "writer.h"

/**
 * Two types (Audio/Video). Live in their own lower priority threads with small internal queues to
 * not disrupt/lag the rest of the process and minimize dropped frames. Break recordings into
 * segments after every 20 minutes, or when a source changes, and records timestamps via the
 * timeline API.
 */

Writer *writer_create(WriterType type, uint32_t source_id)
{
	// TODO: spin up the writer thread, its internal queue, and its first segment.
	return nullptr;
}

void writer_destroy(Writer *writer)
{
	// TODO: drain the queue, finalize the open segment, and join the thread.
}
