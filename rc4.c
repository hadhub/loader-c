#include "rc4.h"

void rc4_crypt(const uint8_t *key, size_t key_len, uint8_t *data, size_t data_len)
{
    uint8_t S[256];
    size_t i;
    uint8_t j = 0;
    uint8_t tmp;

    /* KSA */
    for (i = 0; i < 256; i++)
        S[i] = (uint8_t)i;

    for (i = 0; i < 256; i++) {
        j = j + S[i] + key[i % key_len];
        tmp = S[i];
        S[i] = S[j];
        S[j] = tmp;
    }

    /* PRGA */
    i = 0;
    j = 0;
    for (size_t n = 0; n < data_len; n++) {
        i = (i + 1) & 0xFF;
        j = j + S[i];
        tmp = S[i];
        S[i] = S[j];
        S[j] = tmp;
        data[n] ^= S[(uint8_t)(S[i] + S[j])];
    }
}
