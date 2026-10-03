#pragma once

#include "models.h"

// Write Manager
/**
 * Thin manager that owns the Writers and coordinates the multi-threading around them, keeping
 * that complexity out of Ingest.
 */

struct Writer;

struct WriteManager {
	Writer *video_writer;
	Writer *audio_writer;
	VideoInfo video_info;
};

WriteManager *write_manager_create();
void write_manager_destroy(WriteManager *manager);

void write_manager_update_source(WriteManager *manager, const VideoInfo *info);

void submit_frame([[maybe_unused]] WriteManager *manager);