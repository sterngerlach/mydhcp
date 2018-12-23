
# 情報工学科3年 学籍番号61610117 杉浦 圭祐
# Makefile

# gcc 4.4.7 does not support -Wpedantic option

CC = gcc
CFLAGS = -Os -Wall -Wextra
LDFLAGS = 

OBJDIR = ./obj
OUTDIR = .

TARGET_SERVER = $(OUTDIR)/mydhcps
OBJECTS_SERVER = $(addprefix $(OBJDIR)/, \
	mydhcps.o mydhcps_signal.o mydhcps_config.o \
	mydhcps_ip_addr_list.o mydhcps_client_list.o mydhcps_state_machine.o \
	mydhcp.o util.o)

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

