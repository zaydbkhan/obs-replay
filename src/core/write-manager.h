#pragma once

// Write Manager
/**
 * Thin manager that owns the Writers and coordinates the multi-threading around them, keeping
 * that complexity out of Ingest.
 */

struct Writer;
struct VideoInfo;
struct Packet;

struct WriteManager {
	Writer *video_writer;
	Writer *audio_writer;
	std::string output_directory_path;
};

WriteManager *write_manager_create();
void write_manager_destroy(WriteManager *manager);

void submit_frame([[maybe_unused]] WriteManager *manager);

// By default, we will probably just use OBS's recording directory
// However, we may also allow the user to specify their own directory for the replays to appear in.
// We will just take in the path from some other part of the program, probably the API
// Returns whether there was a path passed in or not
bool determine_output_directory(std::string path);
void write_manager_update_source(WriteManager *manager, const VideoInfo *info);

void write_manager_submit_packet([[maybe_unused]] WriteManager *manager, [[maybe_unused]] const Packet *packet);
