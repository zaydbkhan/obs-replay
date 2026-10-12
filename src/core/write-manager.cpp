#include "write-manager.h"
#include "writer.h"

#include "constants.h"

/**
 * Thin manager that owns the Writers and coordinates the multi-threading around them, keeping
 * that complexity out of Ingest.
 */

WriteManager *write_manager_create(const std::filesystem::path &recording_path)
{
	WriteManager *manager = new WriteManager{};
	determine_output_directory(manager, recording_path);
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

void write_manager_submit_packet(WriteManager *manager, const Packet *packet)
{
	writer_submit_packet(manager->video_writer, packet);
}

bool determine_output_directory(WriteManager *manager, const std::filesystem::path &recording_path)
{
	if (recording_path.empty())
		return false;

	manager->output_directory_path = recording_path / OUTPUT_DIRECTORY_NAME;

	std::error_code error;
	std::filesystem::create_directories(manager->output_directory_path, error);
	return !error;
}
