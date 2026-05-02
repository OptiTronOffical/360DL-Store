#include "OutputConsole.h"
#include "AtgConsole.h"
#include "AtgInput.h"
#include "AtgUtil.h"
#include "Corona4G.h"
#include <stdio.h>
#include <string.h>
#include <iostream>

#include "downloadFile.h"
#include "decompress7z.h"
#include <extract-xiso.h>
#include <ui.h>

#include <win32/dirent.h>

#include <file-stuff.h>

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
			dprintf("Error: USB0 not mounted\n");
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
			dprintf("Error: USB1 not mounted\n");
			return false;
		}
	}
	else
	{
		fclose(fd1);
		remove("Usb1:\\test.tmp");
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

int getGame(const char *URL, const char *sevenZipFile, const char *isoFolder, const char *outputFolder)
{
	if ( downloadFileHTTPS(URL, sevenZipFile, NULL, NULL, dprintf) == false) {
		return EXIT_FAILURE;
	}

	customForceMkdir(isoFolder);

	if (decompressSevenZipFile(sevenZipFile, isoFolder) == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
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

	dprintf("Processing files Success! Your game has been downloaded and processed!\n");

	return EXIT_SUCCESS;
}

int main()
{
	MakeConsole("embed:\\font", CONSOLE_COLOR_BLACK, CONSOLE_COLOR_WHITE);
	if (!CheckGameMounted())
		return -1;

	dprintf("Hello World!\n");

	char selectedGameURL[512];
	selectedGameURL[0] = '\0';

	if (showUI(selectedGameURL, sizeof(selectedGameURL)) == EXIT_SUCCESS)
	{
		dprintf("Selected game URL: %s\n", selectedGameURL);
	}

	getGame(selectedGameURL, "game:\\tmp.7z.001", "game:\\tmp_output", "USB1:\\Apps and Games\\test");
}
