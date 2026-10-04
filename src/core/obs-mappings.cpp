#include "obs-mappings.h"

#include <cstring>

/**
 * Translations from OBS types to our own models.
 */

PixelFormat pixel_format_from_obs(video_format format)
{
	switch (format) {
	case VIDEO_FORMAT_NV12:
		return PIXEL_FORMAT_NV12;
	case VIDEO_FORMAT_RGBA:
		return PIXEL_FORMAT_RGBA;
	case VIDEO_FORMAT_I420:
	default:
		return PIXEL_FORMAT_I420;
	}
}

VideoCodec video_codec_from_obs(const char *codec)
{
	if (codec && strcmp(codec, "hevc") == 0)
		return VIDEO_CODEC_HEVC;
	if (codec && strcmp(codec, "av1") == 0)
		return VIDEO_CODEC_AV1;
	return VIDEO_CODEC_H264;
}

VideoInfo video_info_from_encoder(obs_encoder_t *encoder)
{
	const video_t *video = obs_encoder_video(encoder);
	const video_output_info *output_info = video_output_get_info(video);
	VideoInfo info{};
	info.width = obs_encoder_get_width(encoder);
	info.height = obs_encoder_get_height(encoder);
	info.codec = video_codec_from_obs(obs_encoder_get_codec(encoder));
	info.format = pixel_format_from_obs(output_info->format);

	uint8_t *extra_data;
	size_t extra_data_size;
	obs_encoder_get_extra_data(encoder, &extra_data, &extra_data_size);
	info.extra_data = extra_data;
	info.extra_data_size = extra_data_size;

	return info;
}

Packet *packet_from_obs_packet(Timeline *timeline, encoder_packet *obs_packet)
{
	return packet_create(timeline_translate(timeline, obs_packet->sys_dts_usec * 1000), obs_packet->data,
			     obs_packet->size);
}
