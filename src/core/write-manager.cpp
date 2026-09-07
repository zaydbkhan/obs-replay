#include "write-manager.h"

#include "writer.h"

/**
 * Thin manager that owns the Writers and coordinates the multi-threading around them, keeping
 * that complexity out of Ingest.
 */

WriteManager *write_manager_create()
{
	return nullptr;
}

void write_manager_destroy(WriteManager *write_manager) {}
