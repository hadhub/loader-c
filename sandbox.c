#include "sandbox.h"

BOOL sandbox_check(void)
{
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(MEMORYSTATUSEX);

    if (!GlobalMemoryStatusEx(&mem))
        return FALSE;

    /* 1 GB = 1073741824 bytes */
    if (mem.ullTotalPhys < (1ULL * 1024 * 1024 * 1024))
        return FALSE;

    return TRUE;
}
