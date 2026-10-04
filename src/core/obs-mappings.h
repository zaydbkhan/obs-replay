#pragma once

#include <obs.h>

#include "models.h"
#include "timeline.h"

// OBS Mappings
/**
 * Translations from OBS types to our own models.
 */

PixelFormat pixel_format_from_obs(video_format format);
VideoCodec video_codec_from_obs(const char *codec);
VideoInfo video_info_from_encoder(obs_encoder_t *encoder);
Packet packet_from_obs_packet(Timeline *timeline, encoder_packet *obs_packet);
