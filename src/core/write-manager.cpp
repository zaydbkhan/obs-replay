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
	// create a pool of initial writers
	// we will expand this if we ever want to record a new source, but don't have a spare writer
	for (int i = 0; i < NUM_WRITERS_ON_INIT; i++) {
		Writer *vw = writer_create(WRITER_VIDEO, i + 1, manager->output_directory_path);
		Writer *aw = writer_create(WRITER_AUDIO, i + 1, manager->output_directory_path);

		manager->available_video_writers.push(vw);
		manager->available_audio_writers.push(aw);
	}

    manager->num_video_writers = NUM_WRITERS_ON_INIT;
    manager->num_audio_writers = NUM_WRITERS_ON_INIT;

	//manager->video_writer = writer_create(WRITER_VIDEO, 1, manager->output_directory_path);
	//manager->audio_writer = writer_create(WRITER_AUDIO, 1, manager->output_directory_path);

	return manager;
}

void write_manager_destroy(WriteManager *manager)
{
    while(!manager->available_audio_writers.empty()) {
        writer_destroy(manager->available_audio_writers.front());
        manager->available_audio_writers.pop();
    }

    while (!manager->available_video_writers.empty()) {
	    writer_destroy(manager->available_video_writers.front());
	    manager->available_video_writers.pop();
    }

    for(auto it : manager->source_name_to_audio_writers) {
        writer_destroy(it.second);
    }

    for (auto it : manager->source_name_to_video_writers) {
	    writer_destroy(it.second);
    }

	delete manager;
}

void write_manager_assign_source(WriteManager *wm, const std::string &source_name, bool is_video) {
	std::unique_lock lock(wm->map_mutex);
	if (write_manager_get_writer_for_source(wm, source_name, is_video) != nullptr)
		return;

	if (is_video) {
		// need to create a new video writer
		if (wm->available_video_writers.empty()) {
			Writer *vw = writer_create(WRITER_VIDEO, wm->num_video_writers + 1, wm->output_directory_path);
			wm->num_video_writers++;
			wm->source_name_to_video_writers[source_name] = vw;
		}
		// have an available video writer
		else {
			Writer *available_writer = wm->available_video_writers.front();
			wm->available_video_writers.pop();
			wm->source_name_to_video_writers[source_name] = available_writer;
		}
    }
    else {
	    // need to create a new audio writer
	    if (wm->available_audio_writers.empty()) {
		    Writer *aw = writer_create(WRITER_AUDIO, wm->num_audio_writers + 1, wm->output_directory_path);
		    wm->num_audio_writers++;
		    wm->source_name_to_audio_writers[source_name] = aw;
	    }
	    // have an available audio writer
	    else {
		    Writer *available_writer = wm->available_audio_writers.front();
		    wm->available_audio_writers.pop();
		    wm->source_name_to_audio_writers[source_name] = available_writer;
	    }
    }
}

void write_manager_release_source(WriteManager *wm, const std::string &source_name, bool is_video) {

    if(is_video) {
	    std::unique_lock lock(wm->map_mutex);
	    auto it = wm->source_name_to_video_writers.find(source_name);
	    if (it != wm->source_name_to_video_writers.end()) {
		    writer_reset(it->second);
		    wm->available_video_writers.push(it->second);
		    wm->source_name_to_video_writers.erase(it);
	    }
    }
    else {
	    std::unique_lock lock(wm->map_mutex);
	    auto it = wm->source_name_to_audio_writers.find(source_name);
	    if (it != wm->source_name_to_audio_writers.end()) {
		    writer_reset(it->second);
		    wm->available_audio_writers.push(it->second);
		    wm->source_name_to_audio_writers.erase(it);
	    }
    }
}

Writer *write_manager_get_writer_for_source(WriteManager *wm, const std::string &source_name, bool is_video)
{
	std::shared_lock lock(wm->map_mutex);
	if (is_video) {
		auto it = wm->source_name_to_video_writers.find(source_name);
		if (it == wm->source_name_to_video_writers.end()) {
			return nullptr;
		}
		return it->second;
	} else {
		auto it = wm->source_name_to_audio_writers.find(source_name);
		if (it == wm->source_name_to_audio_writers.end()) {
			return nullptr;
		}
		return it->second;
	}
}

void write_manager_update_source(WriteManager *manager, const std::string& source_name, const VideoInfo *info)
{
	Writer *target_video_writer = write_manager_get_writer_for_source(manager, source_name, true);
    if(target_video_writer) {
	    writer_update_source(target_video_writer, info);
    }
}

void write_manager_submit_packet(WriteManager *manager, const std::string& source_name, const Packet *packet)
{
	Writer *target_video_writer = write_manager_get_writer_for_source(manager, source_name, true);
	if (target_video_writer) {
		writer_submit_packet(target_video_writer, packet);
	}
	// otherwise destroy packet
	else {
		packet_destroy(packet);
	}
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
