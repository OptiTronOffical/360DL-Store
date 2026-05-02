#ifndef DOWNLOAD_FILE_H
#define DOWNLOAD_FILE_H

/// @brief downloads file over https using http 1.1 and tls 1.2
bool downloadFileHTTPS(const char *URL, const char *fileName, char *dataBuffer, unsigned long long *outputBufferSize, void printFunction(const char *_format, ...) );

#endif