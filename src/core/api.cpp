#include "api.h"

/**
 * Translation layer between the rest of the app and OBS, giving callers like the router a stable,
 * testable surface instead of talking to OBS directly.
 */

void obs_replay_core_init() {}

void obs_replay_core_destroy() {}

void api_start_recording() {}

void api_stop_recording() {}
