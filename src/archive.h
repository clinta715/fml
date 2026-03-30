#ifndef ARCHIVE_H
#define ARCHIVE_H

#include "fml.h"

bool archive_is_supported(const char *filename);
int archive_extract(const char *archive_path, const char *dest_dir);

#endif
