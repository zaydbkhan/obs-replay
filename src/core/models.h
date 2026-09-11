#pragma once

#include <stdint.h>

// Models
/**
 * Plain data structures that cross the API boundary.
 */

#define MAX_PLANES 8

enum PixelFormat {
	PIXEL_FORMAT_I420,
	PIXEL_FORMAT_NV12,
	PIXEL_FORMAT_RGBA,
};

struct Frame {
	uint64_t timestamp_ns; // recording-relative (from the timeline)
	uint32_t width;
	uint32_t height;
	enum PixelFormat format;
	const uint8_t *data[MAX_PLANES];
	uint32_t linesize[MAX_PLANES];
};
