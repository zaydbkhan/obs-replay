#include "ingest.h"

#include <obs.h>

/**
 * Manages the OBS side of things for our writers, including calling the API when a
 * source changes, passing through obs video data, etc.
 */

struct Ingest {
	obs_view_t *view;
	video_t *video;
};

bool get_first_source_callback(void *data, obs_source_t *source)
{
	auto **to_assign = static_cast<obs_source_t **>(data);
	*to_assign = obs_source_get_ref(source);
	return false;
}

Ingest *ingest_create()
{
	auto *ingest = new Ingest{};

	obs_source_t *source = nullptr;
	obs_enum_sources(get_first_source_callback, source);

	obs_video_info video_info;
	obs_get_video_info(&video_info);
	video_info.base_height = obs_source_get_height(source);
	video_info.base_width = obs_source_get_width(source);
	video_info.output_height = video_info.base_height;
	video_info.output_width = video_info.base_width;

	ingest->view = obs_view_create();
	ingest->video = obs_view_add2(ingest->view, &video_info);

	return ingest;
}

void ingest_destroy(Ingest *ingest)
{
	obs_view_destroy(ingest->view);
	delete ingest;
}
