#pragma once

// Preview State
/**
 * Tracks which replays we're playing back, and where we're at in their playback. In the case of
 * multiple replays being played back at once, will always return 2 frames (A and B channel).
 * Compares against the timeline to avoid speeding/lagging.
 */
