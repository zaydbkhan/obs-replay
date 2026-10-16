#pragma once

#include <vector>
#include <unordered_map>



// Packet Sidecar
/**
 * Maps global timestamps to segment byte offsets, with each file segment having its own mapping.
 * Above this, each source (1-8) has another mapping that indicates the span of each segment. This
 * allows easy translation between timestamps and file positions. Writes must write to files THEN
 * append to the sidecar atomically, otherwise there is a risk of something reading a partially
 * written or non-existent offset.
 */

struct Writer;
struct Segment;

struct PacketSidecar {
    // map all segments and their start and end times to the ordinal of the writer that wrote them
	std::unordered_map<int, std::vector <Segment *>> segments_by_writer;
    //std::unordered_map<int, std::vector<uint64_t>> start_times_by_writer;
    //std::unordered_map<int, std::vector<uint64_t>> end_times_by_writer;
};

PacketSidecar *create_packet_sidecar();
void destroy_packet_sidecar(PacketSidecar* ps);
void packet_sidecar_add_segment(PacketSidecar* ps, Writer* writer);

// use our timeline time stamps to get all valid segments within the timeframe
std::vector<Segment*> packet_sidecar_get_segments_for_timeframe(PacketSidecar* ps, int writer_ordinal, uint64_t start_time, uint64_t end_time);
