#pragma once

// Timeline
/**
 * The absolute truth on time. Additionally handles start/stop recording. Whatever backing clock
 * is injected/mockable so the rest of the app is testable in reasonable time. No arrows because
 * everything talks to this.
 */
