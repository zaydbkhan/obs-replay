#pragma once

#include <obs.h>
#include <string>
#include <atomic>
#include <thread>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
#include "../core/segment.h"
// Preview (OBS Source)
/**
 * Replay playback, shows the replays, transitions between them, etc. Owns 3 (or more) private OBS
 * sources, including an A/B channel for replay mixing and a transition/stinger.
 */

struct ReplaySourceContext {
    obs_source_t *source;
    std::string current_replay_name;

    std::vector<Segment*> segments_to_play;
    int current_segment_index = 0;

    std::thread playback_thread;
    std::atomic<bool> playing{false};

    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *codec_ctx = nullptr;
};

void register_replay_source();
void push_frame_to_obs(ReplaySourceContext *context, uint8_t *video_data, uint64_t timestamp_ns);
void play_replay_in_current_scene(const std::string &replay_name);
void playback_thread_loop(ReplaySourceContext *context);
