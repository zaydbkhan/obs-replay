#pragma once

// Writers
/**
 * Two types (Audio/Video). Live in their own lower priority threads with small internal queues to
 * not disrupt/lag the rest of the process and minimize dropped frames. Break recordings into
 * segments after every 20 minutes, or when a source changes, and records timestamps via the
 * timeline API.
 */
