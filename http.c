#include "http.h"
#include <winhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static wchar_t *to_wide(const char *str)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if (len == 0)
        return NULL;
    wchar_t *wide = (wchar_t *)malloc(len * sizeof(wchar_t));
    if (!wide)
        return NULL;
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wide, len);
    return wide;
}

BOOL http_download(const char *url, uint8_t **out_data, size_t *out_len)
{
    BOOL result = FALSE;
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    URL_COMPONENTSW uc;
    wchar_t hostName[256] = {0};
    wchar_t urlPath[1024] = {0};

    *out_data = NULL;
    *out_len  = 0;

    wchar_t *wurl = to_wide(url);
    if (!wurl)
        return FALSE;

    ZeroMemory(&uc, sizeof(uc));
    uc.dwStructSize    = sizeof(uc);
    uc.lpszHostName    = hostName;
    uc.dwHostNameLength = sizeof(hostName) / sizeof(wchar_t);
    uc.lpszUrlPath     = urlPath;
    uc.dwUrlPathLength = sizeof(urlPath) / sizeof(wchar_t);

    if (!WinHttpCrackUrl(wurl, 0, 0, &uc)) {
        free(wurl);
        return FALSE;
    }
    free(wurl);

    hSession = WinHttpOpen(L"Mozilla/5.0",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME,
                           WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
        return FALSE;

    hConnect = WinHttpConnect(hSession, hostName, uc.nPort, 0);
    if (!hConnect)
        goto cleanup;

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath,
                                  NULL, WINHTTP_NO_REFERER,
                                  WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest)
        goto cleanup;

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
        goto cleanup;

    if (!WinHttpReceiveResponse(hRequest, NULL))
        goto cleanup;

    /* Read response body in chunks */
    {
        uint8_t *buf = NULL;
        size_t total = 0;
        DWORD bytes_read = 0;
        uint8_t chunk[4096];

        for (;;) {
            if (!WinHttpReadData(hRequest, chunk, sizeof(chunk), &bytes_read))
                goto read_fail;
            if (bytes_read == 0)
                break;

            uint8_t *tmp = (uint8_t *)realloc(buf, total + bytes_read);
            if (!tmp)
                goto read_fail;
            buf = tmp;
            memcpy(buf + total, chunk, bytes_read);
            total += bytes_read;
        }

        *out_data = buf;
        *out_len  = total;
        result = TRUE;
        goto cleanup;

    read_fail:
        free(buf);
    }

cleanup:
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    return result;
}
