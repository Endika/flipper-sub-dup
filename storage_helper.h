#pragma once

#include "app_state.h"

bool storage_scan_directory(SubDupFinderApp *app, const char *dir, ScanStats *stats);
bool storage_delete_file(const char *path);
