#include "packet-sidecar.h"

/**
 * Maps global timestamps to segment byte offsets, with each file segment having its own mapping.
 * Above this, each source (1-8) has another mapping that indicates the span of each segment. This
 * allows easy translation between timestamps and file positions. Writes must write to files THEN
 * append to the sidecar atomically, otherwise there is a risk of something reading a partially
 * written or non-existent offset.
 */
