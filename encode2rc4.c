#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rc4.h"

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "[*] Usage : %s <key> <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    const char *key        = argv[1];
    const char *input_file = argv[2];
    const char *output_file = argv[3];

    printf("[+] Reading input file\n");
    FILE *fp = fopen(input_file, "rb");
    if (!fp) {
        perror("[-] fopen input");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0) {
        fprintf(stderr, "[-] Empty or invalid input file\n");
        fclose(fp);
        return 1;
    }

    uint8_t *data = (uint8_t *)malloc(file_size);
    if (!data) {
        perror("[-] malloc");
        fclose(fp);
        return 1;
    }

    fread(data, 1, file_size, fp);
    fclose(fp);

    printf("[+] To RC4\n");
    printf("[*] Key : %s\n", key);
    printf("[*] Output file : %s\n", output_file);

    rc4_crypt((const uint8_t *)key, strlen(key), data, (size_t)file_size);

    fp = fopen(output_file, "wb");
    if (!fp) {
        perror("[-] fopen output");
        free(data);
        return 1;
    }

    fwrite(data, 1, file_size, fp);
    fclose(fp);

    printf("[+] Shellcode size : %ld\n", file_size);

    free(data);
    return 0;
}
