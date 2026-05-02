#ifndef PARSING_H
#define PARSING_H

struct ChunkedDecodeState
{
    unsigned long remaining;
    char sizeLine[32];
    int sizeLineLen;
    int state;
};

void InitChunkedDecodeState(ChunkedDecodeState *state);
bool ParseChunkSize(const char *line, int len, unsigned long *size);
int HexValue(char c);
bool HeaderContainsToken(const char *headers, int headerLen, const char *name, const char *token);
int FindHeaderEnd(const char *data, int len);
bool IncrementSplitFilename(char *filename);
bool ContainsNoCase(const char *data, int dataLen, const char *needle);
bool EqualsNoCase(const char *a, int aLen, const char *b);
char LowerAscii(char c);
int parseURL(const char *URL, char *domain, char *path);

#endif
