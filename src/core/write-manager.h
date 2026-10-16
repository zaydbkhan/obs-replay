#pragma once

#include <filesystem>
#include <unordered_map>
#include <vector>
#include <utility>
#include <queue>
#include <shared_mutex>

#include "packet-sidecar.h"

// Write Manager
/**
 * Thin manager that owns the Writers and coordinates the multi-threading around them, keeping
 * that complexity out of Ingest.
 */

struct Writer;
struct VideoInfo;
struct Packet;

struct WriteManager {
	//Writer *video_writer;
	//Writer *audio_writer;
	std::filesystem::path output_directory_path;

    std::queue<Writer*> available_video_writers;
    std::queue<Writer*> available_audio_writers;

    int num_video_writers;
    int num_audio_writers;

    std::unordered_map<std::string, Writer*> source_name_to_video_writers;
    std::unordered_map<std::string, Writer*> source_name_to_audio_writers;
    std::shared_mutex map_mutex;

    PacketSidecar* ps;
};

WriteManager *write_manager_create(const std::filesystem::path &recording_path);
void write_manager_destroy(WriteManager *manager);
void write_manager_assign_source(WriteManager* wm, const std::string& source_name, bool is_video);
void write_manager_release_source(WriteManager* wm, const std::string& source_name, bool is_video);

Writer* write_manager_get_writer_for_source(WriteManager* wm, const std::string& source_name, bool is_video);


bool determine_output_directory(WriteManager *manager, const std::filesystem::path &recording_path);
void write_manager_update_source(WriteManager *manager, const std::string& source_name, const VideoInfo *info);

void write_manager_submit_packet(WriteManager *manager, const std::string& source_name, const Packet *packet);
