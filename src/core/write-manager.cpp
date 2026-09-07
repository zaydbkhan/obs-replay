#include "write-manager.h"

#include "writer.h"

/**
 * Thin manager that owns the Writers and coordinates the multi-threading around them, keeping
 * that complexity out of Ingest.
 */

WriteManager *write_manager_create()
{
	// TODO: create the video and audio writers.
	return nullptr;
}

void write_manager_destroy(WriteManager *write_manager)
{
	// TODO: destroy the writers, then the manager.
}
