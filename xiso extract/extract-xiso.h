#ifndef EXTRACT_XISO_H
#define EXTRACT_XISO_H

#include <OutputConsole.h>

// #define getcwd _getcwd
// #define chdir _chdir

#define true 1
#define false 0

#define _WIN32
#define WIN32

#define READFLAGS O_RDONLY | O_BINARY
#define WRITEFLAGS O_WRONLY | O_CREAT | O_TRUNC | O_BINARY
#define READWRITEFLAGS O_RDWR | O_BINARY

int extractIso(const char *isoPath, const char *pathToExtractTo);

#endif