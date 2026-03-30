CC = clang
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -O2 -D_DEFAULT_SOURCE
LDFLAGS = -lncurses

SRCDIR = src
OBJDIR = obj
TARGET = fml

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
CFLAGS += -D_DARWIN_C_SOURCE
endif
ifeq ($(UNAME_S),Linux)
CFLAGS += -D_DEFAULT_SOURCE
endif

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

.PHONY: all clean install static release

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

static: CFLAGS += -static
static: LDFLAGS = -static -lncurses -ltinfo
static: clean all

release: CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -Os -DNDEBUG
release: clean $(TARGET)
	strip $(TARGET)