#pragma once

#include <obs.h>
#include <string>

// Ingest
/**
 * Manages the OBS side of things for our writers, including calling the API when a
 * source changes, passing through obs video data, etc.
 */


struct Ingest;

void ingest_register_output();

struct Ingest *ingest_create(const std::string &target_source_name);
void ingest_destroy(struct Ingest *ingest);

