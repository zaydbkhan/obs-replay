#pragma once

// Write Manager
/**
 * A very thin class that owns/can talk to our writers for management purposes. Potentially over-
 * engineered, (the ingest could just talk to the writer directly), but its not too confusing and
 * is consistent with the existence of the API. Also handles the multi-threading stuff. Ew!
 */
