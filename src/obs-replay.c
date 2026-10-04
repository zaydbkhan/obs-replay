/*
OBS Replay
Copyright (C) <2026> <Isaac Khabra, Zayd Khan, Aaron Zhao> <zaydbkhan@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>

#include "core/obs-replay-api.h"
#include "obs/ingest.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

static struct Ingest *ingest = NULL;

static void frontend_event_callback(enum obs_frontend_event event, void *data)
{
	(void)data;

	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING: {
		char *recording_path = obs_frontend_get_current_record_output_path();
		obs_replay_core_init(recording_path);
		bfree(recording_path);

		ingest = ingest_create();
		obs_replay_core_start_recording();
		break;
	}
	case OBS_FRONTEND_EVENT_EXIT:
		obs_replay_core_stop_recording();
		if (ingest) {
			ingest_destroy(ingest);
			ingest = NULL;
		}
		obs_replay_core_destroy();
		break;
	default:
		break;
	}
}

bool obs_module_load(void)
{
	ingest_register_output();
	obs_frontend_add_event_callback(frontend_event_callback, NULL);

	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(frontend_event_callback, NULL);

	obs_log(LOG_INFO, "plugin unloaded");
}
