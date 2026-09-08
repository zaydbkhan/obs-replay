#include "write-manager.h"

#include "writer.h"

/**
 * Thin manager that owns the Writers and coordinates the multi-threading around them, keeping
 * that complexity out of Ingest.
 */

WriteManager *write_manager_create()
{
	WriteManager *manager = new WriteManager{};
	manager->video_writer = writer_create(WRITER_VIDEO);
	manager->audio_writer = writer_create(WRITER_AUDIO);
	return manager;
}

void write_manager_destroy(WriteManager *manager)
{
	writer_destroy(manager->video_writer);
	writer_destroy(manager->audio_writer);
	delete manager;
}

void submit_frame([[maybe_unused]] WriteManager *manager)
{
	return;
}
