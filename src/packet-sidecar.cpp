/*
Plugin Name
Copyright (C) <Year> <Developer> <Email Address>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "packet-sidecar.h"

/**
 * Maps global timestamps to segment byte offsets, with each file segment having its own mapping.
 * Above this, each source (1-8) has another mapping that indicates the span of each segment. This
 * allows easy translation between timestamps and file positions. Writes must write to files THEN
 * append to the sidecar atomically, otherwise there is a risk of something reading a partially
 * written or non-existent offset.
 */
