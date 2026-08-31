#include "preview-source.h"

/**
 * Replay playback, shows the replays, transitions between them, etc. Owns 3 (or more) private OBS
 * sources, including an A/B channel for replay mixing and a transition/stinger.
 */
