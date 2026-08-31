#pragma once

// Files / Segments
/**
 * The literal files that the writers write to, and everything else reads from. Will have some
 * sort of consistent naming convention TBA, and if things get slow we can add some additional
 * infrastructure to reduce syscalls (but that's unlikely to be the main slowdown).
 */
