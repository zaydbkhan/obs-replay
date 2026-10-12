#pragma once



// Ingest
/**
 * Manages the OBS side of things for our writers, including calling the API when a
 * source changes, passing through obs video data, etc.
 */

#ifdef __cplusplus
#include <string>
extern "C" {
#endif

struct Ingest;

void ingest_register_output();

struct Ingest *ingest_create();
void ingest_destroy(struct Ingest *ingest);

struct IngestOutputContext {
	obs_output_t *output;
	std::string source_name;
};

#ifdef __cplusplus
}
#endif
