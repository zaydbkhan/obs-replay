#include "packet-sidecar.h"
#include "writer.h"

/**
 * Maps global timestamps to segment byte offsets, with each file segment having its own mapping.
 * Above this, each source (1-8) has another mapping that indicates the span of each segment. This
 * allows easy translation between timestamps and file positions. Writes must write to files THEN
 * append to the sidecar atomically, otherwise there is a risk of something reading a partially
 * written or non-existent offset.
 */

PacketSidecar *create_packet_sidecar() {
    return new PacketSidecar();
}

void destroy_packet_sidecar(PacketSidecar *ps) {
    delete(ps);
}

void packet_sidecar_add_segment(PacketSidecar *ps, Writer *writer) {
    int ordinal = writer->ordinal;

    ps->segments_by_writer[ordinal].push_back(writer->current_segment);
    //ps->start_times_by_writer[ordinal].push_back(writer->current_segment->start_timestamp);
    //ps->end_times_by_writer[ordinal].push_back(writer->current_segment->end_timestamp);
}

std::vector<Segment *> packet_sidecar_get_segments_for_timeframe(PacketSidecar *ps, int writer_ordinal, uint64_t start_time, uint64_t end_time) {
    std::vector<Segment *> overlapping_segments;

    if(!ps) return overlapping_segments;
    
    auto it = ps->segments_by_writer.find(writer_ordinal);

    if(it == ps->segments_by_writer.end()) return overlapping_segments;
    // segments are so long that binary search doesn't really matter
    for(Segment* seg : it->second) {
        if(!seg) continue;

        if(seg->start_timestamp <= end_time && seg->end_timestamp >= start_time) {
            overlapping_segments.push_back(seg);
        }
    }
    return overlapping_segments;
}
