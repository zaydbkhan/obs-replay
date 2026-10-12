#pragma once

#include <filesystem>

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
	std::filesystem::path output_directory_path;
};

WriteManager *write_manager_create(const std::filesystem::path &recording_path);
void write_manager_destroy(WriteManager *manager);

bool determine_output_directory(WriteManager *manager, const std::filesystem::path &recording_path);
void write_manager_update_source(WriteManager *manager, const VideoInfo *info);

void write_manager_submit_packet(WriteManager *manager, const Packet *packet);
