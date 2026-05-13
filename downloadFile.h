#ifndef DOWNLOAD_FILE_H
#define DOWNLOAD_FILE_H

#include <string>

/// @brief downloads file over https using http 1.1 and tls 1.2
int downloadFileHTTPS(const std::string URL, const std::string fileName, char *dataBuffer, unsigned long long *outputBufferSize, bool downloadIntoFile, void printFunction(const char *_format, ...));

#endif