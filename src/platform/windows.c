#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "../platform.h"

// @TODO: what happens if FormatMessage fails?
internal void print_last_error(u8* label)
{
    DWORD code = GetLastError();
    char *msg = NULL;
    FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM    |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            code,
            0, // default language
            (LPSTR)&msg,
            0,
            NULL
            );

    fprintf(stderr, "%s failed with error %lu: %s\n", label, code, msg);
    LocalFree(msg);
}

// @TODO: harden this function
File os_read_file(u8* path)
{
    File result = {0};

    HANDLE h = CreateFileA(
            (LPSTR)path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

    if (h == INVALID_HANDLE_VALUE) {
        print_last_error((u8*)"CreateFileA");
        return result;
    }

    // Get file size (assuming <4GB)
    DWORD size = GetFileSize(h, NULL);
    if (size == INVALID_FILE_SIZE) {
        print_last_error((u8*)"GetFileSize");
        CloseHandle(h);
        return result;
    }

    // Read entire file into heap buffer
    u8* buf = (u8*)malloc(size + 1);
    if (!buf) {
        fprintf(stderr, "malloc failed\n");
        CloseHandle(h);
        return result;
    }

    DWORD bytesRead = 0;
    if (!ReadFile(h, buf, size, &bytesRead, NULL)) {
        print_last_error((u8*)"ReadFile");
        free(buf);
        CloseHandle(h);
        return result;
    }
    buf[bytesRead] = '\0';

    result.length = size;
    result.data = buf;

    CloseHandle(h);

    return result;
}
