#include <xtl.h>

#include <ctype.h>
#include <iostream>
#include <string>

#include <OutputConsole.h>
#include <downloadFile.h>

#include "vimms_lair.h"

static std::string UrlEncodeQuery(const std::string &value)
{
    static const char hex[] = "0123456789ABCDEF";
    std::string encoded;

    for (size_t i = 0; i < value.length(); ++i)
    {
        unsigned char c = (unsigned char)value[i];

        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            encoded += (char)c;
        }
        else if (c == ' ')
        {
            encoded += '+';
        }
        else
        {
            encoded += '%';
            encoded += hex[c >> 4];
            encoded += hex[c & 0x0F];
        }
    }

    return encoded;
}

static void RenderSearchResults(const GameList *list, int selected, int scroll)
{
    const int visibleRows = 10;

    ClearConsole();
    dprintf("Vimm's Lair search results\n\n");

    if (!list || list->count == 0)
    {
        dprintf("No games found.\n\n");
        dprintf("B: Back\n");
        return;
    }

    for (int row = 0; row < visibleRows; ++row)
    {
        int index = scroll + row;
        if (index >= (int)list->count)
            break;

        dprintf("%c %2d. %s\n",
                index == selected ? '>' : ' ',
                index + 1,
                list->items[index].name);
    }

    dprintf("\nA: Select   B: Back   D-Pad: Move\n");
}

static int ShowSearchResultsUI(const GameList *list)
{
    if (!list || list->count == 0)
    {
        RenderSearchResults(list, 0, 0);

        while (true)
        {
            XINPUT_STATE state;
            ZeroMemory(&state, sizeof(state));

            if (XInputGetState(0, &state) == ERROR_SUCCESS &&
                (state.Gamepad.wButtons & XINPUT_GAMEPAD_B))
            {
                return -1;
            }

            Sleep(50);
        }
    }

    const int visibleRows = 10;
    int selected = 0;
    int scroll = 0;
    WORD previousButtons = 0;

    RenderSearchResults(list, selected, scroll);

    while (true)
    {
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(state));

        if (XInputGetState(0, &state) != ERROR_SUCCESS)
        {
            Sleep(50);
            continue;
        }

        WORD buttons = state.Gamepad.wButtons;
        WORD pressed = buttons & ~previousButtons;
        previousButtons = buttons;

        if (pressed & XINPUT_GAMEPAD_A)
            return selected;

        if (pressed & XINPUT_GAMEPAD_B)
            return -1;

        if (pressed & XINPUT_GAMEPAD_DPAD_UP)
            selected--;

        if (pressed & XINPUT_GAMEPAD_DPAD_DOWN)
            selected++;

        if (selected < 0)
            selected = 0;

        if (selected >= (int)list->count)
            selected = (int)list->count - 1;

        if (selected < scroll)
            scroll = selected;

        if (selected >= scroll + visibleRows)
            scroll = selected - visibleRows + 1;

        if (pressed & (XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_DOWN))
            RenderSearchResults(list, selected, scroll);

        Sleep(50);
    }
}

static void RenderMediaResults(const MediaList *list, const char *gameName, int selected)
{
    ClearConsole();
    dprintf("Choose download media\n\n");

    if (!list || list->count == 0)
    {
        dprintf("No media IDs found.\n\n");
        dprintf("B: Back\n");
        return;
    }

    for (int i = 0; i < (int)list->count; ++i)
    {
        dprintf("%c %2d. %s Disc %s version %s\n",
                i == selected ? '>' : ' ',
                i + 1,
                gameName ? gameName : "Selected game",
                list->items[i].disc,
                list->items[i].version);
    }

    dprintf("\nA: Select   B: Back   D-Pad: Move\n");
}

static int ShowMediaResultsUI(const MediaList *list, const char *gameName)
{
    if (!list || list->count == 0)
    {
        RenderMediaResults(list, gameName, 0);
        while (true)
        {
            XINPUT_STATE state;
            ZeroMemory(&state, sizeof(state));

            if (XInputGetState(0, &state) == ERROR_SUCCESS &&
                (state.Gamepad.wButtons & XINPUT_GAMEPAD_B))
            {
                return -1;
            }

            Sleep(50);
        }
    }

    if (list->count == 1)
        return 0;

    int selected = 0;
    WORD previousButtons = 0;

    RenderMediaResults(list, gameName, selected);

    while (true)
    {
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(state));

        if (XInputGetState(0, &state) != ERROR_SUCCESS)
        {
            Sleep(50);
            continue;
        }

        WORD buttons = state.Gamepad.wButtons;
        WORD pressed = buttons & ~previousButtons;
        previousButtons = buttons;

        if (pressed & XINPUT_GAMEPAD_A)
            return selected;

        if (pressed & XINPUT_GAMEPAD_B)
            return -1;

        if (pressed & XINPUT_GAMEPAD_DPAD_UP)
            selected--;

        if (pressed & XINPUT_GAMEPAD_DPAD_DOWN)
            selected++;

        if (selected < 0)
            selected = 0;

        if (selected >= (int)list->count)
            selected = (int)list->count - 1;

        if (pressed & (XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_DOWN))
            RenderMediaResults(list, gameName, selected);

        Sleep(50);
    }
}

static void WideToCharSimple(const WCHAR *src, char *dst, DWORD dstSize)
{
    if (!dst || dstSize == 0)
        return;

    dst[0] = '\0';

    if (!src)
        return;

    DWORD i = 0;

    for (; i < dstSize - 1 && src[i] != L'\0'; ++i)
    {
        // Simple narrow conversion.
        // Characters outside 8-bit range become '?'.
        dst[i] = (src[i] <= 0xFF) ? (char)src[i] : '?';
    }

    dst[i] = '\0';
}

DWORD OpenKeyboardToString(
    DWORD userIndex,
    std::string *outputString,
    LPCWSTR title,
    LPCWSTR description,
    LPCWSTR defaultText)
{

    // XShowKeyboardUI writes WCHARs, so use a wide temp buffer.
    const DWORD MAX_KEYBOARD_CHARS = 256;
    WCHAR wideResult[MAX_KEYBOARD_CHARS];
    char finalResult[MAX_KEYBOARD_CHARS];
    ZeroMemory(wideResult, sizeof(wideResult));
    finalResult[0] = '\0';

    XOVERLAPPED overlapped;
    ZeroMemory(&overlapped, sizeof(overlapped));

    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!overlapped.hEvent)
        return GetLastError();

    DWORD result = XShowKeyboardUI(
        userIndex,
        VKBD_LATIN_FULL | VKBD_HIGHLIGHT_TEXT,
        defaultText,
        title,
        description,
        wideResult,
        MAX_KEYBOARD_CHARS,
        &overlapped);

    if (result != ERROR_IO_PENDING)
    {
        CloseHandle(overlapped.hEvent);
        return result;
    }

    // Blocking wait until the user presses Done or Cancel.
    result = XGetOverlappedResult(&overlapped, NULL, TRUE);

    DWORD extended = XGetOverlappedExtendedError(&overlapped);

    CloseHandle(overlapped.hEvent);

    if (result != ERROR_SUCCESS)
        return result;

    // ERROR_CANCELLED means the user backed out.
    if (extended != ERROR_SUCCESS)
        return extended;

    WideToCharSimple(wideResult, finalResult, MAX_KEYBOARD_CHARS);

    outputString->assign(finalResult);

    return ERROR_SUCCESS;
}

int showUI(char *gameURL, int len, char *gameName, int gameNameLen)
{
    const unsigned long long HTML_BUFFER_CAPACITY = 1024 * 1024 * 4;
    unsigned long long OUTPUT_BUFFER_SIZE = HTML_BUFFER_CAPACITY;

    char *buffer = (char *)malloc(OUTPUT_BUFFER_SIZE); // 4MB buffer just to be safe

    if (buffer == NULL)
    {
        dprintf("Malloc failed\n");
        return EXIT_FAILURE;
    }

    std::string gameSearchString = "";
    DWORD keyboardResult = OpenKeyboardToString(0, &gameSearchString, L"Search for a game", L"Search for a game here", L"GTA VI");
    if (keyboardResult != ERROR_SUCCESS || gameSearchString.empty())
    {
        free(buffer);
        return EXIT_FAILURE;
    }

    std::cout << "Text output: " << gameSearchString << "\n";

    std::string searchURL = "https://vimm.net/vault/?p=list&system=Xbox360&q=";

    searchURL.append(UrlEncodeQuery(gameSearchString));

    if (downloadFileHTTPS(searchURL.c_str(), NULL, buffer, &OUTPUT_BUFFER_SIZE, dprintf) == false)
    { // downloads into a null terminated buffer
        std::cout << "Download failed\n";
        free(buffer);
        return EXIT_FAILURE;
    }

    GameList list = {0};

    if (!parse_vimm_search_results(buffer, &list)) // this strlen operation assumes that there were no NULL character before the end, which is fine for HTML, but not for other files
    {
        fprintf(stderr, "Failed to parse HTML.\n");
        free(buffer);
        free_game_list(&list);
        return 1;
    }

    int selected = ShowSearchResultsUI(&list);

    if (selected < 0)
    {
        free_game_list(&list);
        free(buffer);
        return EXIT_FAILURE;
    }

    std::string selectedURL = "https://vimm.net";
    selectedURL.append(list.items[selected].link);

    if (gameName && gameNameLen > 0)
    {
        strncpy(gameName, list.items[selected].name, gameNameLen - 1);
        gameName[gameNameLen - 1] = '\0';
    }

    ClearConsole();
    dprintf("Selected:\n%s\n\n%s\n", list.items[selected].name, selectedURL.c_str());


	// Now download the selected game

    OUTPUT_BUFFER_SIZE = HTML_BUFFER_CAPACITY;
    if (downloadFileHTTPS(selectedURL.c_str(), NULL, buffer, &OUTPUT_BUFFER_SIZE, dprintf) == false)
    { // downloads into a null terminated buffer
        std::cout << "Download failed\n";
        free_game_list(&list);
        free(buffer);
        return EXIT_FAILURE;
    }

    MediaList mediaList = {0};
    if (!parse_vimm_media_ids(buffer, &mediaList) || mediaList.count == 0)
    {
        dprintf("Failed to parse media ID from selected game page.\n");
        free_media_list(&mediaList);
        free_game_list(&list);
        free(buffer);
        return EXIT_FAILURE;
    }

    int selectedMedia = ShowMediaResultsUI(&mediaList, list.items[selected].name);
    if (selectedMedia < 0)
    {
        free_media_list(&mediaList);
        free_game_list(&list);
        free(buffer);
        return EXIT_FAILURE;
    }

    std::string finalDownloadURL = "https://dl3.vimm.net/?mediaId=";
    finalDownloadURL.append(mediaList.items[selectedMedia].id);

    if (gameURL && len > 0)
    {
        strncpy(gameURL, finalDownloadURL.c_str(), len - 1);
        gameURL[len - 1] = '\0';
    }

    ClearConsole();
    dprintf("Download URL:\n%s\n\n%s Disc %s version %s\n",
            finalDownloadURL.c_str(),
            list.items[selected].name,
            mediaList.items[selectedMedia].disc,
            mediaList.items[selectedMedia].version);

    free_media_list(&mediaList);
    free_game_list(&list);
    free(buffer);
    return EXIT_SUCCESS;
}
