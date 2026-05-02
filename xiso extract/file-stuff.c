#include "file-stuff.h"
#include <stdio.h>
#include <io.h>
#include <string.h>
#include <errno.h>

#include <sys/stat.h>
#include <stdlib.h>
#include "win32/dirent.h"

// #include <stdbool.h>

// List of needed functions:
/*
open: store FD, etc
close: close ALL related files
read: Transparent read across files
lseek: place file pointer in correct location
chdir: set CWD
getcwd: duh!
*/

#ifndef false
#define false 0
#endif

#ifndef true
#define true 1
#endif

#define arrayLength(x) (sizeof(x) / sizeof(x[0]))

typedef struct fileInfo
{
    int fd;
    bool isOpen;
} fileInfo;

static char currentWorkingDir[250] = "game:\\";

static fileInfo openFiles[10] = {
    {0, false},
};

static long long splitFilePointer = 0;

static bool isAbsolutePath(const char *path)
{
    return path != NULL && (strchr(path, ':') != NULL || path[0] == '\\' || path[0] == '/');
}

static void normalizePath(const char *inputPath, char *outputPath, size_t outputPathSize)
{
    char tempPath[300];
    char prefix[16] = "";
    char *segments[32];
    int segmentCount = 0;
    char *context = NULL;
    char *token = NULL;
    size_t prefixLength = 0;

    if (inputPath == NULL || outputPath == NULL || outputPathSize == 0)
    {
        return;
    }

    strncpy(tempPath, inputPath, sizeof(tempPath) - 1);
    tempPath[sizeof(tempPath) - 1] = '\0';

    for (size_t i = 0; tempPath[i] != '\0'; i++)
    {
        if (tempPath[i] == '/')
        {
            tempPath[i] = '\\';
        }
    }

    if (tempPath[0] != '\0' && tempPath[1] == ':')
    {
        prefix[0] = tempPath[0];
        prefix[1] = tempPath[1];
        prefix[2] = '\\';
        prefix[3] = '\0';
        prefixLength = 3;
    }
    else if (tempPath[0] == '\\')
    {
        if (currentWorkingDir[0] != '\0' && currentWorkingDir[1] == ':')
        {
            prefix[0] = currentWorkingDir[0];
            prefix[1] = currentWorkingDir[1];
            prefix[2] = '\\';
            prefix[3] = '\0';
            prefixLength = 3;
        }
        else
        {
            prefix[0] = '\\';
            prefix[1] = '\0';
            prefixLength = 1;
        }
    }

    token = strtok_s(tempPath + prefixLength, "\\", &context);
    while (token != NULL)
    {
        if (strcmp(token, ".") == 0 || token[0] == '\0')
        {
            token = strtok_s(NULL, "\\", &context);
            continue;
        }

        if (strcmp(token, "..") == 0)
        {
            if (segmentCount > 0)
            {
                segmentCount--;
            }
            token = strtok_s(NULL, "\\", &context);
            continue;
        }

        if (segmentCount < (int)arrayLength(segments))
        {
            segments[segmentCount++] = token;
        }
        token = strtok_s(NULL, "\\", &context);
    }

    outputPath[0] = '\0';
    if (prefix[0] != '\0')
    {
        strncpy(outputPath, prefix, outputPathSize - 1);
        outputPath[outputPathSize - 1] = '\0';
    }

    for (int i = 0; i < segmentCount; i++)
    {
        if (strlen(outputPath) > 0 && outputPath[strlen(outputPath) - 1] != '\\')
        {
            strncat(outputPath, "\\", outputPathSize - strlen(outputPath) - 1);
        }
        strncat(outputPath, segments[i], outputPathSize - strlen(outputPath) - 1);
    }

    if (outputPath[0] == '\0')
    {
        strncpy(outputPath, prefix[0] != '\0' ? prefix : ".", outputPathSize - 1);
        outputPath[outputPathSize - 1] = '\0';
    }
}

static void resolvePath(const char *inputPath, char *outputPath, size_t outputPathSize)
{
    char combinedPath[300];

    if (inputPath == NULL || outputPath == NULL || outputPathSize == 0)
    {
        return;
    }

    if (isAbsolutePath(inputPath) || currentWorkingDir[0] == '\0')
    {
        normalizePath(inputPath, outputPath, outputPathSize);
        return;
    }

    if (currentWorkingDir[strlen(currentWorkingDir) - 1] == '\\' || currentWorkingDir[strlen(currentWorkingDir) - 1] == '/')
    {
        sprintf(combinedPath, "%s%s", currentWorkingDir, inputPath);
    }
    else
    {
        sprintf(combinedPath, "%s\\%s", currentWorkingDir, inputPath);
    }

    normalizePath(combinedPath, outputPath, outputPathSize);
}

int customOpen(const char *filename,
               int oflag,
               int pmode)
{
    char baseFileName[250];
    char tempFileName[300];

    resolvePath(filename, tempFileName, sizeof(tempFileName));

    strcpy(baseFileName, tempFileName);
    baseFileName[strlen(baseFileName) - 4] = '\0'; // remove last 4 digits (.001)

    if (strcmp(&tempFileName[strlen(tempFileName) - 4], ".001") == 0) // check for split file
    {
        int partIndex = 1;
        // Now do the hard bit
        while (partIndex <= 10) // ok ChatGPT, I will add a bounds check
        {
            char tempBuff[250];
            sprintf(tempBuff, "%s.%03d", baseFileName, partIndex);
            if ((openFiles[partIndex - 1].fd = open(tempBuff, oflag, pmode)) < 0)
            {
                openFiles[partIndex - 1].isOpen = false;
                printf("WARNING! failed to open %s\n", tempBuff);
                break;
            }
            openFiles[partIndex - 1].isOpen = true;

            partIndex++;
        }
        splitFilePointer = 0;   // reset FP
        return openFiles[0].fd; // return file descriptor of first file. This can then be used for customRead, and customLseek
    }
    else
    {
        return open(tempFileName, oflag, pmode); // pass on the call, nothing special
    }
}

int customMkdir(const char *dirname)
{
    char tempDirName[300];

    resolvePath(dirname, tempDirName, sizeof(tempDirName));
    return mkdir(tempDirName);
}

// Source - https://stackoverflow.com/a/32496721
// Posted by Superlokkus, modified by community. See post 'Timeline' for change history
// Retrieved 2026-04-20, License - CC BY-SA 4.0

char *replace_char(char *str, char find, char replace)
{
    char *current_pos = strchr(str, find);
    while (current_pos)
    {
        *current_pos = replace;
        current_pos = strchr(current_pos, find);
    }
    return str;
}

// Will recersively create directories
int customForceMkdir(const char *dir)
{
    char tempDir[300] = "\0";
    char currentPath[300] = "\0";
    char *context = NULL;
    char *token = NULL;
    char *deviceSeparator = NULL;
    size_t prefixLength = 0;

    resolvePath(dir, tempDir, sizeof(tempDir));

    deviceSeparator = strchr(tempDir, ':');
    if (deviceSeparator != NULL)
    {
        prefixLength = (size_t)(deviceSeparator - tempDir) + 1;
        if (tempDir[prefixLength] == '\\' || tempDir[prefixLength] == '/')
        {
            prefixLength++;
        }

        strncpy(currentPath, tempDir, prefixLength);
        currentPath[prefixLength] = '\0';
    }
    else if (tempDir[0] == '\\')
    {
        currentPath[0] = '\\';
        currentPath[1] = '\0';
        prefixLength = 1;
    }

    token = strtok_s(tempDir + prefixLength, "\\", &context);
    while (token != NULL)
    {
        if (strlen(currentPath) > 0 && currentPath[strlen(currentPath) - 1] != '\\')
        {
            strncat(currentPath, "\\", sizeof(currentPath) - strlen(currentPath) - 1);
        }
        strncat(currentPath, token, sizeof(currentPath) - strlen(currentPath) - 1);

        if (mkdir(currentPath) != 0 && errno != EEXIST)
        {
            return -1;
        }

        token = strtok_s(NULL, "\\", &context);
    }

    return 0;
}

int customClose(int fd)
{
    if (openFiles[0].fd == fd)
    {
        for (int i = 1; i < 10; i++)
        {
            if (openFiles[i].isOpen)
            {
                close(openFiles[i].fd);
                openFiles[i].isOpen = false;
                openFiles[i].fd = -1;
            }
        }
        openFiles[0].isOpen = false;
        splitFilePointer = 0;                   // clear all global variables
        int returnVal = close(openFiles[0].fd); // return error code
        openFiles[0].fd = -1;
        return returnVal;
    }
    else
    {
        return close(fd);
    }
}

static int fpToFd(long long fp, long long *fpForSelectedFile, int *currentFile)
{
    struct _stat64 fileStat;

    int tempFp = 0;

    int currentFd = 0; // index of the current working file
    while (true)
    {
        if (currentFd >= 10)
        {
            return -2;
        }
        if (!openFiles[currentFd].isOpen)
        {
            *fpForSelectedFile = -1;
            *currentFile = currentFd;
            return -1; // fp must be past the end of the file.
        }

        _fstat64(openFiles[currentFd].fd, &fileStat);
        if (fp >= fileStat.st_size)
        {
            fp = fp - fileStat.st_size; // ready for next file
        }
        else
        {
            *currentFile = currentFd;
            *fpForSelectedFile = fp;
            return openFiles[currentFd].fd;
        }
        currentFd++;
    }
}

long long customRead(int const fd, void *const buffer, long long const buffer_size)
{
    if (fd == openFiles[0].fd)
    { // split file detected
        if (!openFiles[0].isOpen)
        {
            return -1; // file (descriptor) not found
        }

        long long newFp = 0;
        int currentFile = 0;
        int fileDescriptor = fpToFd(splitFilePointer, &newFp, &currentFile);
        if (fileDescriptor < 0)
        {
            return 0; // read past end of file
        }
        // int bytesRead = read(fileDescriptor, buffer, buffer_size);
        long long bytesRead = 0;
        _lseeki64(fileDescriptor, newFp, SEEK_SET);
        while (bytesRead < buffer_size && currentFile < (int)arrayLength(openFiles) && openFiles[currentFile].isOpen)
        {
            long long tempReadVal = read(openFiles[currentFile].fd, (char *)buffer + bytesRead, buffer_size - bytesRead);
            if (tempReadVal < 0)
            {
                return tempReadVal; // read error
            }

            bytesRead += tempReadVal;
            currentFile++;
            if (currentFile < (int)arrayLength(openFiles) && openFiles[currentFile].isOpen)
            {
                _lseeki64(openFiles[currentFile].fd, 0, SEEK_SET); // when moving to a new file, reset the file pointer to the start
            }
        }
        splitFilePointer += bytesRead;
        return bytesRead;
    }
    else
    {
        return read(fd, buffer, buffer_size);
    }
}

long long customLseek(int fd,
                      long long offset,
                      int origin)
{
    if (fd == openFiles[0].fd && openFiles[0].isOpen)
    {

        struct _stat64 fileStat;
        long long fileSize = 0;

        for (int i = 0; i < 10 && openFiles[i].isOpen == true; i++)
        {
            _fstat64(openFiles[i].fd, &fileStat);
            fileSize += fileStat.st_size; // calculate size of ALL files in split group
        }
        if (offset > fileSize || offset < (0 - fileSize))
        {
            printf("ERROR: offset out of bounds. max: %lli, given: %lli\n", fileSize, offset);
            return -1;
        }

        switch (origin)
        {
        case SEEK_SET: // start of file
            if (offset < 0)
            {
                printf("ERROR: invalid offset, given: %lli\n", offset);
                return -1; // invalid offset
            }
            splitFilePointer = offset;
            break;
        case SEEK_CUR: // add to current fp
            if (splitFilePointer + offset > fileSize || (splitFilePointer + offset) < 0)
            {
                printf("ERROR: seek past end/start of file\n");
                return -1; // past end or before start of file
            }
            splitFilePointer += offset;
            break;
        case SEEK_END:
            if (offset > 0)
            {
                printf("ERROR: invalid offset, given: %lli\n", offset);
                return -1; // invalid offset
            }
            splitFilePointer = fileSize + offset; // calculate distance from end of fileS
            break;
        default:
            printf("ERROR: invalid origin\n");
            return -1; // invalid origin
            break;
        }
        return splitFilePointer;
    }
    else
    {
        return _lseeki64(fd, offset, origin); // just the normal
    }
}

int chdir(const char *dirname)
{
    char tempDirName[300];

    resolvePath(dirname, tempDirName, sizeof(tempDirName));
    strncpy(currentWorkingDir, tempDirName, sizeof(currentWorkingDir) - 1);
    currentWorkingDir[sizeof(currentWorkingDir) - 1] = '\0';
    return 0; // success
}

char *getcwd(
    char *buffer,
    int maxlen)
{
    if (buffer == NULL && maxlen == 0)
    {
        size_t cwdLength = strlen(currentWorkingDir) + 1;
        char *cwdBuffer = (char *)malloc(cwdLength);
        if (cwdBuffer == NULL)
        {
            return NULL;
        }
        memcpy(cwdBuffer, currentWorkingDir, cwdLength);
        return cwdBuffer;
    }
    if (maxlen <= strlen(currentWorkingDir))
    {
        return NULL; // buffer too small
    }
    return strncpy(buffer, currentWorkingDir, maxlen);
}

int deleteDirectory(const char *directoryToDelete, const size_t len)
{
    DIR *dir = opendir(directoryToDelete);
    struct dirent *entity;
    entity = readdir(dir);
    while (entity != NULL)
    {
        char path[256] = "";
		strcpy(path, directoryToDelete);

        printf("%s\n", entity->d_name);
        strcat(path, entity->d_name);
        printf("PAth: %s\n", path);
        remove(path);
        entity = readdir(dir);
    }
    char path1[256] = "";
	strcpy(path1, directoryToDelete);
    rmdir(path1);
    closedir(dir);
    // char out[256] = "OUTPUT/";
    // char fol_file[256];
    // sprintf(fol_file, "%s\\", out);
    // printf("%s", fol_file);
    return EXIT_SUCCESS;
}
