#ifndef UI_H
#define UI_H

#include <string>
#include <xtl.h>

DWORD OpenKeyboardToString(
    DWORD userIndex,
    std::string *outputString,
    LPCWSTR title,
    LPCWSTR description,
    LPCWSTR defaultText
);

int showUI(char *gameURL, int len, char *gameName, int gameNameLen);

#endif
