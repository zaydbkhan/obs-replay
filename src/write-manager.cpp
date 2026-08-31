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

#include "write-manager.h"

/**
 * A very thin class that owns/can talk to our writers for management purposes. Potentially over-
 * engineered, (the ingest could just talk to the writer directly), but its not too confusing and
 * is consistent with the existence of the API. Also handles the multi-threading stuff. Ew!
 */
