#include "timeline.h"

/**
 * The single source of truth for time; also handles start/stop recording. The backing clock is
 * injected/mockable so the rest of the app can be tested without waiting on real time.
 */
