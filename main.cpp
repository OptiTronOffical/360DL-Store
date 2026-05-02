#include "OutputConsole.h"
#include "AtgConsole.h"
#include "AtgInput.h"
#include "AtgUtil.h"
#include "Corona4G.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <iostream>

#include "downloadFile.h"
#include "decompress7z.h"
#include <extract-xiso.h>
#include <ui.h>

#include <win32/dirent.h>

#include <file-stuff.h>

#define SETTINGS_FILE "game:\\settings.txt"

struct Settings
{
	char outputPath[256];
};

bool CheckGameMounted()
{
	FILE *fd;
	FILE *fd1;
	if (fopen_s(&fd, "game:\\test.tmp", "w") != 0)
	{
		dprintf("GAME_NOT_MOUNTED_TRYING_USB\n");
		fclose(fd);
		if (mount("game:", "\\Device\\Mass0") != 0)
		{
			dprintf("GAME_NOT_MOUNTED_TRYING_HDD\n");
			if (mount("game:", "\\Device\\Harddisk0\\Partition1") != 0)
			{
				dprintf("GAME_NOT_MOUNTED\n");
				return false;
			}
		}
	}
	else
	{
		fclose(fd);
		remove("game:\\test.tmp");
	}

	// Check USB0 is mounted
	if (fopen_s(&fd1, "Usb0:\\test.tmp", "w") != 0)
	{
		// dprintf(MSG_ERROR MSG_GAME_NOT_MOUNTED_TRYING_USB);
		// fclose(fd1);
		if (mount("Usb0:", "\\Device\\Mass0") != 0)
		{
			dprintf("Warning: USB0 not mounted\n");
			return false;
		}
	}
	else
	{
		fclose(fd1);
		remove("Usb0:\\test.tmp");
	}

	// Check USB1 is mounted
	if (fopen_s(&fd1, "Usb1:\\test.tmp", "w") != 0)
	{
		// dprintf(MSG_ERROR MSG_GAME_NOT_MOUNTED_TRYING_USB);
		// fclose(fd1);
		if (mount("Usb1:", "\\Device\\Mass1") != 0)
		{
			dprintf("Warning: USB1 not mounted\n");
			return false;
		}
	}
	else
	{
		fclose(fd1);
		remove("Usb1:\\test.tmp");
	}

	// Check HDD is mounted
	if (fopen_s(&fd1, "Hdd:\\test.tmp", "w") != 0)
	{
		if (mount("Hdd:", "\\Device\\Mass1") != 0)
		{
			dprintf("Warning: Hdd not mounted\n");
			return false;
		}
	}
	else
	{
		fclose(fd1);
		remove("Hdd:\\test.tmp");
	}
	return true;
}

int findIso(const char *folder, char *isoFile, int len)
{
	// Source - https://stackoverflow.com/a/612176
	// Posted by Peter Parker, modified by community. See post 'Timeline' for change history
	// Retrieved 2026-04-26, License - CC BY-SA 4.0

	std::cout << "\nSearching for ISO in " << folder << "\n";

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(folder)) != NULL)
	{
		/* print all the files and directories within directory */
		while ((ent = readdir(dir)) != NULL)
		{
			std::cout << ent->d_name << "\n";
			int nameLength = strlen(ent->d_name);

			if (strcmp(&(ent->d_name[nameLength - strlen(".iso.001")]), ".iso.001") == 0)
			{ // identical strings
				strncpy(isoFile, ent->d_name, len);
				return (strlen(ent->d_name) < len) ? EXIT_SUCCESS : EXIT_FAILURE;
			}
		}
		closedir(dir);
	}
	else
	{
		/* could not open directory */
		std::cout << "Directory not found\n";
		perror("");
		return EXIT_FAILURE;
	}
	return -1;
}

static bool IsDigitString(const char *text)
{
	if (!text || !*text)
		return false;

	for (int i = 0; text[i] != '\0'; ++i)
	{
		if (!isdigit((unsigned char)text[i]))
			return false;
	}

	return true;
}

static bool StartsWith(const char *text, const char *prefix)
{
	return strncmp(text, prefix, strlen(prefix)) == 0;
}

static int DeleteSplitFiles(const char *firstPartFile)
{
	char directory[512];
	char fileName[256];
	const char *lastSlash = strrchr(firstPartFile, '\\');
	const char *lastForwardSlash = strrchr(firstPartFile, '/');

	if (!lastSlash || (lastForwardSlash && lastForwardSlash > lastSlash))
		lastSlash = lastForwardSlash;

	if (lastSlash)
	{
		size_t directoryLen = (lastSlash - firstPartFile) + 1;
		if (directoryLen >= sizeof(directory))
			return EXIT_FAILURE;

		strncpy(directory, firstPartFile, directoryLen);
		directory[directoryLen] = '\0';
		strncpy(fileName, lastSlash + 1, sizeof(fileName) - 1);
		fileName[sizeof(fileName) - 1] = '\0';
	}
	else
	{
		strcpy(directory, ".\\");
		strncpy(fileName, firstPartFile, sizeof(fileName) - 1);
		fileName[sizeof(fileName) - 1] = '\0';
	}

	char *lastDot = strrchr(fileName, '.');
	if (!lastDot || !IsDigitString(lastDot + 1))
	{
		return remove(firstPartFile);
	}

	char splitPrefix[256];
	size_t splitPrefixLen = (lastDot - fileName) + 1;
	if (splitPrefixLen >= sizeof(splitPrefix))
		return EXIT_FAILURE;

	strncpy(splitPrefix, fileName, splitPrefixLen);
	splitPrefix[splitPrefixLen] = '\0';

	DIR *dir = opendir(directory);
	if (!dir)
		return remove(firstPartFile);

	int result = EXIT_SUCCESS;
	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL)
	{
		if (!StartsWith(ent->d_name, splitPrefix))
			continue;

		if (!IsDigitString(ent->d_name + splitPrefixLen))
			continue;

		char fileToDelete[512];
		if (strlen(directory) + strlen(ent->d_name) >= sizeof(fileToDelete))
		{
			result = EXIT_FAILURE;
			continue;
		}

		strcpy(fileToDelete, directory);
		strcat(fileToDelete, ent->d_name);

		if (remove(fileToDelete) != 0)
			result = EXIT_FAILURE;
	}

	closedir(dir);
	return result;
}

int getGame(const char *URL, const char *sevenZipFile, const char *isoFolder, const char *outputFolder)
{
	if (downloadFileHTTPS(URL, sevenZipFile, NULL, NULL, dprintf) == false)
	{
		return EXIT_FAILURE;
	}

	customForceMkdir(isoFolder);

	printf("Extracting %s\n", sevenZipFile); //Debug

	if (decompressSevenZipFile(sevenZipFile, isoFolder) == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	if (DeleteSplitFiles(sevenZipFile) != EXIT_SUCCESS)
	{
		dprintf("Warning: failed to delete all 7z parts after 7z extraction\n");
	}

	char isoFile[250];
	char temp[250];

	if (findIso(isoFolder, isoFile, sizeof(isoFile)) == EXIT_FAILURE)
	{
		dprintf("Failed to find .iso.001 file in %s\n", isoFolder);
		return EXIT_FAILURE;
	}

	strcpy(temp, isoFolder);

	if (temp[strlen(temp) - 1] != '\\' && temp[strlen(temp) - 1] != '/')
	{
		strcat(temp, "\\");
	}

	strcat(temp, isoFile);

	customForceMkdir(outputFolder);

	if (extractIso(temp, outputFolder) != 0)
	{
		return EXIT_FAILURE;
	}

	if (DeleteSplitFiles(temp) != EXIT_SUCCESS)
	{
		dprintf("Warning: failed to delete all ISO parts after ISO extraction\n");
	}

	char isoFolderToDelete[250];
	strncpy(isoFolderToDelete, isoFolder, sizeof(isoFolderToDelete) - 2);
	isoFolderToDelete[sizeof(isoFolderToDelete) - 2] = '\0';

	size_t isoFolderToDeleteLen = strlen(isoFolderToDelete);
	if (isoFolderToDeleteLen > 0 &&
		isoFolderToDelete[isoFolderToDeleteLen - 1] != '\\' &&
		isoFolderToDelete[isoFolderToDeleteLen - 1] != '/')
	{
		strcat(isoFolderToDelete, "\\");
		isoFolderToDeleteLen++;
	}

	if (deleteDirectory(isoFolderToDelete, isoFolderToDeleteLen) == EXIT_FAILURE)
	{
		dprintf("Warning: failed to delete %s after ISO extraction\n", isoFolder);
	}

	dprintf("Processing files Success! Your game has been downloaded and processed!\n");

	return EXIT_SUCCESS;
}

static bool IsInvalidFolderChar(char c)
{
	return c == '<' || c == '>' || c == ':' || c == '"' ||
		   c == '/' || c == '\\' || c == '|' || c == '?' || c == '*';
}

static void MakeSafeFolderName(const char *gameName, char *folderName, int folderNameLen)
{
	if (!folderName || folderNameLen <= 0)
		return;

	folderName[0] = '\0';

	if (!gameName || !*gameName)
	{
		strncpy(folderName, "Unknown Game", folderNameLen - 1);
		folderName[folderNameLen - 1] = '\0';
		return;
	}

	int out = 0;
	bool lastWasSpace = true;

	for (int i = 0; gameName[i] != '\0' && out < folderNameLen - 1; ++i)
	{
		unsigned char c = (unsigned char)gameName[i];

		if (IsInvalidFolderChar((char)c) || c < 32)
			continue;

		if (isspace(c))
		{
			if (!lastWasSpace && out < folderNameLen - 1)
			{
				folderName[out++] = ' ';
				lastWasSpace = true;
			}
			continue;
		}

		folderName[out++] = (char)c;
		lastWasSpace = false;
	}

	while (out > 0 && (folderName[out - 1] == ' ' || folderName[out - 1] == '.'))
		out--;

	if (out == 0)
	{
		strncpy(folderName, "Unknown Game", folderNameLen - 1);
		folderName[folderNameLen - 1] = '\0';
		return;
	}

	folderName[out] = '\0';
}

struct Settings getSettings()
{
	struct Settings settings;

	strcpy(settings.outputPath, "game:"); // default back to where ever the xex file was launched from

	FILE *fd = fopen(SETTINGS_FILE, "r");

	if (fd == NULL)
	{
		dprintf("Failed to open settings file\n");
		return settings;
	}

	char buff[256];

	while (fgets(buff, sizeof(buff), fd) != NULL)
	{
		if (buff[0] == '#')
		{
			continue; // comment
		}

		if (strncmp(buff, "output-path: ", strlen("output-path: ")) == 0)
		{
			char *value = &buff[strlen("output-path: ")];

			value[strcspn(value, "\r\n")] = '\0';

			strcpy(settings.outputPath, value);
		}
	}

	fclose(fd);

	return settings;
}

int main()
{
	MakeConsole("embed:\\font", CONSOLE_COLOR_BLACK, CONSOLE_COLOR_WHITE);
	if (!CheckGameMounted())
		dprintf("Warning: Some paths may not be mounted\n");

	dprintf("free60 store 0.0.1 alpha\n");

	char selectedGameURL[512];
	char selectedGameName[256];
	char safeGameFolderName[256];
	char outputFolder[512];
	selectedGameURL[0] = '\0';
	selectedGameName[0] = '\0';

	if (showUI(selectedGameURL, sizeof(selectedGameURL), selectedGameName, sizeof(selectedGameName)) == EXIT_SUCCESS)
	{
		dprintf("Selected game URL: %s\n", selectedGameURL);
		dprintf("Selected game: %s\n", selectedGameName);
	}
	else
	{
		return EXIT_FAILURE;
	}

	struct Settings settings = getSettings();

	MakeSafeFolderName(selectedGameName, safeGameFolderName, sizeof(safeGameFolderName));
	_snprintf(outputFolder, sizeof(outputFolder), "%s\\%s", settings.outputPath, safeGameFolderName);
	outputFolder[sizeof(outputFolder) - 1] = '\0';
	dprintf("Extracting to: %s\n", outputFolder);

	return getGame(selectedGameURL, "game:\\tmp.7z.001", "game:\\tmp_output", outputFolder);
}
