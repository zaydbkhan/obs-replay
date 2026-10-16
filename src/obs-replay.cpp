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
#include <unordered_map>
#include <QMainWindow>
#include <QMetaObject>
#include <QObject>
#include <QComboBox>
#include <QLabel>

#include "plugin-support.h"
#include "core/obs-replay-api.h"
#include "obs/dock.h"
#include "obs/ingest.h"
#include "obs/preview-source.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

static struct Ingest *ingest = NULL;
ReplayDock *global_dock = nullptr;

obs_hotkey_id start_hotkey_id = OBS_INVALID_HOTKEY_ID;
obs_hotkey_id stop_hotkey_id = OBS_INVALID_HOTKEY_ID;

static std::unordered_map<std::string, std::string> active_replays;

void update_ingest_target(const std::string &new_target)
{
	obs_enter_graphics();

	// Kill the old ingest
	if (ingest) {
		ingest_destroy(ingest);
		ingest = nullptr;
	}

	// Start a new one on the new source
	if (!new_target.empty()) {
		ingest = ingest_create(new_target);
		obs_log(LOG_INFO, "Ingest switched to target: %s", new_target.c_str());
	}

	obs_leave_graphics();
}

static void frontend_event_callback(enum obs_frontend_event event, void *data)
{
	(void)data;

	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING: {
		char *recording_path = obs_frontend_get_current_record_output_path();
		obs_replay_core_init(recording_path);
		bfree(recording_path);

		QMainWindow *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
		global_dock = new ReplayDock(mainWindow);
		obs_frontend_add_dock_by_id("obs_replay_dock", "Replay Manager", global_dock);

		obs_replay_core_set_ui_callback([](const std::string &replay_name) {
			if (global_dock) {
				// Thread-safe UI update
				QMetaObject::invokeMethod(
					global_dock, [replay_name]() { global_dock->addReplayToList(replay_name); },
					Qt::QueuedConnection);
			}
		});
		std::string target_source = global_dock->getSelectedTargetSource();

		obs_enter_graphics();
		ingest = ingest_create(target_source);
		obs_leave_graphics();
		if (ingest) {
			obs_replay_core_start_recording();
		} else {
			obs_log(LOG_WARNING, "Ingest creation failed (no source). Recording aborted.");
		}

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

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(frontend_event_callback, NULL);

	obs_log(LOG_INFO, "plugin unloaded");
}

void start_replay_hotkey_pressed([[maybe_unused]] void *data, [[maybe_unused]] obs_hotkey_id id,
				 [[maybe_unused]] obs_hotkey_t *hotkey, [[maybe_unused]] bool pressed)
{
	// Ignore the key-release event so we don't start two replays
	if (!pressed)
		return;

	if (!global_dock)
		return;

	std::string target = global_dock->getSelectedTargetSource();

	if (!target.empty()) {
		obs_log(LOG_INFO, "Hotkey pressed: Starting replay for %s", target.c_str());
		auto replay_name = obs_replay_core_start_recording_replay(target);
		if (replay_name) {
			active_replays[target] = *replay_name;
			obs_log(LOG_INFO, "Started replay %s for %s", replay_name.value().c_str(), target.c_str());
		}
	} else {
		obs_log(LOG_WARNING, "Hotkey pressed, but no source selected in Replay Dock!");
	}
}

void stop_replay_hotkey_pressed([[maybe_unused]] void *data, [[maybe_unused]] obs_hotkey_id id,
				[[maybe_unused]] obs_hotkey_t *hotkey, [[maybe_unused]] bool pressed)
{
	// Ignore the key-release event
	if (!pressed)
		return;

	if (!global_dock)
		return;

	std::string target = global_dock->getSelectedTargetSource();

	if (!target.empty()) {
		auto it = active_replays.find(target);
		if (it != active_replays.end()) {
			std::string replay_id = it->second;
			obs_replay_core_stop_recording_replay(replay_id);

			// Remove it from the active map since it's stopped
			active_replays.erase(it);
		}
		obs_log(LOG_INFO, "Hotkey pressed: Stopping replay for %s", target.c_str());
		obs_replay_core_stop_recording_replay(target);
	} else {
		obs_log(LOG_WARNING, "Hotkey pressed, but no source selected in Replay Dock!");
	}
}

void register_replay_hotkeys()
{
	start_hotkey_id = obs_hotkey_register_frontend("obs_replay_start", "Start Recording Replay",
						       start_replay_hotkey_pressed, nullptr);
	stop_hotkey_id = obs_hotkey_register_frontend("obs_replay_stop", "Stop Recording Replay",
						      stop_replay_hotkey_pressed, nullptr);
}

bool obs_module_load(void)
{
	ingest_register_output();
	register_replay_hotkeys();
	obs_frontend_add_event_callback(frontend_event_callback, NULL);
	register_replay_source();

	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}
