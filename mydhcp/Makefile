
# 情報工学科3年 学籍番号61610117 杉浦 圭祐
# Makefile

CC = gcc
CFLAGS = -Os -Wall -Wextra -Wpedantic
LDFLAGS = 

OBJDIR = ./obj
OUTDIR = .

TARGET_SERVER = $(OUTDIR)/mydhcps
OBJECTS_SERVER = $(addprefix $(OBJDIR)/, mydhcps.o config.o util.o)

server: $(TARGET_SERVER)

debugs: CFLAGS += -ggdb
debugs: $(TARGET_SERVER)

$(TARGET_SERVER): $(OBJECTS_SERVER)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(OBJECTS_SERVER)

.PHONY: clean

