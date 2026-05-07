#ifndef FAT32_H
#define FAT32_H

int Fat32RenameUsb1(const char *oldPath, const char *newPath);
int Fat32RenameOnDevice(const char *driveName, const char *devicePath, const char *oldPath, const char *newPath);

#endif
