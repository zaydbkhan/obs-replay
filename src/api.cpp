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

#include "api.h"

/**
 * It may be advantageous to have a real API for testing, and it doesn't really hurt to have this
 * layer here. Simplifies the router too. This is the only module of ours that knows about OBS,
 * and it performs translations. For now we don't expose anything we wouldn't expose otherwise.
 */
