#ifndef RC4_H
#define RC4_H

#include <stdint.h>
#include <stddef.h>

void rc4_crypt(const uint8_t *key, size_t key_len, uint8_t *data, size_t data_len);

#endif
