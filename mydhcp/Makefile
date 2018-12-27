
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
TARGET_CLIENT = $(OUTDIR)/mydhcpc
OBJECTS_CLIENT = $(addprefix $(OBJDIR)/, \
	mydhcpc.o mydhcpc_context.o mydhcpc_signal.o mydhcpc_state_machine.o \
	mydhcp.o util.o)

server: $(TARGET_SERVER)
client: $(TARGET_CLIENT)

debugs: CFLAGS += -ggdb
debugs: $(TARGET_SERVER)
debugc: CFLAGS += -ggdb
debugc: $(TARGET_CLIENT)

$(TARGET_SERVER): $(OBJECTS_SERVER)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
$(TARGET_CLIENT): $(OBJECTS_CLIENT)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(OBJECTS_SERVER)
	rm -f $(OBJECTS_CLIENT)

.PHONY: clean

