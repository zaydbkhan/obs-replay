#pragma once

#include <stddef.h>
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

enum VideoCodec {
	VIDEO_CODEC_H264,
	VIDEO_CODEC_HEVC,
	VIDEO_CODEC_AV1,
};

struct VideoInfo {
	uint32_t width;
	uint32_t height;
	enum PixelFormat format;
	enum VideoCodec codec;
	const uint8_t *extra_data;
	size_t extra_data_size;
};

struct Packet {
	uint64_t timestamp_ns; // recording-relative (from the timeline)
	const uint8_t *data;
	size_t size;
};