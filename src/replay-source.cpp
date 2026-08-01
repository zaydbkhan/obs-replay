#include <obs-source.h>

struct obs_source_info replay_source;

bool register_replay_source()
{
	replay_source.id = "replay-source";
	replay_source.type = OBS_SOURCE_TYPE_INPUT;
	replay_source.output_flags = OBS_SOURCE_VIDEO;
	replay_source.get_name = replay_source_get_name;
	replay_source.create = replay_source_create;
	replay_source.destroy = replay_source_destroy;
	replay_source.update = replay_source_update;
	replay_source.video_render = replay_source_render;
	replay_source.get_width = replay_source_width;
	replay_source.get_height = replay_source_height;

	obs_register_source(&replay_source);
	return true;
}

const char *replay_source_name(void *type_data)
{
	return "Replay";
};

void *replay_source_create(obs_data_t *settings, obs_source_t *source)
{
	return;
};

void replay_source_destroy(void *data)
{
	return;
};

void replay_source_update(void *data, obs_data_t *settings)
{
	return;
};

void replay_source_render(void *data, gs_effect_t *effect)
{
	return;
};

uint32_t replay_source_width(void *data)
{
	return;
};

uint32_t replay_source_height(void *data)
{
	return;
};