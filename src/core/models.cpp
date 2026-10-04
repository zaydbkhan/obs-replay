#include <cstring>

#include "models.h"

/**
 * Plain data structures that cross the API boundary.
 */

const Packet *packet_create(uint64_t timestamp_ns, const uint8_t *data, size_t size)
{
	uint8_t *data_copy = new uint8_t[size];
	std::memcpy(data_copy, data, size);

	Packet *packet = new Packet{};
	packet->timestamp_ns = timestamp_ns;
	packet->data = data_copy;
	packet->size = size;

	return packet;
}

void packet_destroy(const Packet *packet)
{
	delete[] packet->data;
	delete packet;
}