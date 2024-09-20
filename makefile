CC := gcc
CFLAGS := -g -Wall
BUILD_DIR := build
UTILS_OBJS := $(BUILD_DIR)/socket.o $(BUILD_DIR)/buffer.o $(BUILD_DIR)/server.o $(BUILD_DIR)/client.o
INCLUDE_DIR := include

$(shell mkdir -p $(BUILD_DIR))

define compile
$(BUILD_DIR)/$2: $1 $(INCLUDE_DIR)/*.h
	$$(CC) $$(CFLAGS) -I$$(INCLUDE_DIR) -c $1 -o $$@
endef

define link
$(BUILD_DIR)/$2: $$(addprefix build/, $1) $(UTILS_OBJS)
	$(CC) $(CFLAGS) $$^ -o $$@
endef

all: build/BDC_command build/BDC_random build/BDS build/FC build/FS
	@echo "Done."

$(eval $(call compile,disk/BDC_random.c,BDC_random.o))
$(eval $(call compile,disk/BDC_command.c,BDC_command.o))
$(eval $(call compile,disk/BDS.c,BDS.o))
$(eval $(call compile,fs/FC.c,FC.o))
$(eval $(call compile,fs/FS.c,FS.o))
$(eval $(call compile,fs/blocks.c,blocks.o))
$(eval $(call compile,fs/disk.c,disk.o))
$(eval $(call compile,fs/inodes.c,inodes.o))
$(eval $(call compile,fs/fs.c,fs.o))
$(eval $(call compile,utils/socket.c,socket.o))
$(eval $(call compile,utils/buffer.c,buffer.o))
$(eval $(call compile,utils/server.c,server.o))
$(eval $(call compile,utils/client.c,client.o))

$(eval $(call link,BDC_command.o,BDC_command))
$(eval $(call link,BDC_random.o,BDC_random))
$(eval $(call link,BDS.o,BDS))
$(eval $(call link,FC.o,FC))
$(eval $(call link,FS.o blocks.o disk.o inodes.o fs.o,FS))

clean:
	rm -rf $(BUILD_DIR)