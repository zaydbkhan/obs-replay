#include "write-manager.h"
#include "writer.h"

/**
 * Thin manager that owns the Writers and coordinates the multi-threading around them, keeping
 * that complexity out of Ingest.
 */

WriteManager *write_manager_create()
{
	WriteManager *manager = new WriteManager{};
	determine_output_directory(manager,
				   "/Users/isaackhabra/Documents/Programming/projects/obs_replay/test/recording_files");
	manager->video_writer = writer_create(WRITER_VIDEO, 1, manager->output_directory_path);
	manager->audio_writer = writer_create(WRITER_AUDIO, 1, manager->output_directory_path);

	return manager;
}

void write_manager_destroy(WriteManager *manager)
{
	writer_destroy(manager->video_writer);
	writer_destroy(manager->audio_writer);
	delete manager;
}

void write_manager_update_source(WriteManager *manager, const VideoInfo *info)
{
	writer_update_source(manager->video_writer, info);
}

void write_manager_submit_packet([[maybe_unused]] WriteManager *manager, [[maybe_unused]] const Packet *packet)
{
	return;
}

bool determine_output_directory(WriteManager *manager, std::string path)
{
	if (path == "") {
		// handle this in some way
		return false;
	}
	// Check the directory is valid higher up
	else {
		manager->output_directory_path = path;
	}
	return true;
}