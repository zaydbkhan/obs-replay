#pragma once

// Write Manager
/**
 * Thin manager that owns the Writers and coordinates the multi-threading around them, keeping
 * that complexity out of Ingest.
 */

struct Writer;

struct WriteManager {
	Writer *video_writer;
	Writer *audio_writer;
};

WriteManager *write_manager_create();
void write_manager_destroy(WriteManager *manager);

void submit_frame([[maybe_unused]] WriteManager *manager);