#pragma once

#include <obs.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum replay_control_action {
	REPLAY_CONTROL_LOAD,
	REPLAY_CONTROL_FIRST,
	REPLAY_CONTROL_PREVIOUS,
	REPLAY_CONTROL_PLAY_PAUSE,
	REPLAY_CONTROL_NEXT,
	REPLAY_CONTROL_LAST,
	REPLAY_CONTROL_RESTART,
	REPLAY_CONTROL_STOP,
	REPLAY_CONTROL_REMOVE,
	REPLAY_CONTROL_CLEAR,
	REPLAY_CONTROL_SAVE,
	REPLAY_CONTROL_SLOWER,
	REPLAY_CONTROL_FASTER,
	REPLAY_CONTROL_HALF_SPEED,
	REPLAY_CONTROL_NORMAL_SPEED,
	REPLAY_CONTROL_DOUBLE_SPEED,
	REPLAY_CONTROL_TRIM_FRONT,
	REPLAY_CONTROL_TRIM_END,
	REPLAY_CONTROL_TRIM_RESET,
	REPLAY_CONTROL_REVERSE,
	REPLAY_CONTROL_FORWARD,
	REPLAY_CONTROL_BACKWARD,
	REPLAY_CONTROL_DISABLE,
	REPLAY_CONTROL_ENABLE,
	REPLAY_CONTROL_PREVIOUS_FRAME,
	REPLAY_CONTROL_NEXT_FRAME,
	REPLAY_CONTROL_PREVIOUS_N_FRAMES,
	REPLAY_CONTROL_NEXT_N_FRAMES,
};

struct replay_control_snapshot {
	int64_t duration_ms;
	int64_t position_ms;
	int replay_index;
	int replay_count;
	float speed_percent;
	enum obs_media_state state;
	bool backward;
	bool capture_enabled;
	bool saving;
};

struct replay_control_item {
	int id;
	int64_t in_ms;
	int64_t out_ms;
	int64_t duration_ms;
	float speed_percent;
};

bool replay_control_is_source(const obs_source_t *source);
bool replay_control_execute(obs_source_t *source, enum replay_control_action action);
bool replay_control_get_snapshot(obs_source_t *source, struct replay_control_snapshot *snapshot);
size_t replay_control_get_items(obs_source_t *source, struct replay_control_item *items, size_t capacity);
bool replay_control_select_item(obs_source_t *source, int index);
bool replay_control_set_time(obs_source_t *source, int64_t position_ms);

#ifdef __cplusplus
}
#endif
