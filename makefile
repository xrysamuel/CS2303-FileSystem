CC := gcc
CFLAGS := -g -Wall
BUILD_DIR := build
UTILS_OBJS := $(BUILD_DIR)/socket.o $(BUILD_DIR)/buffer.o
INCLUDE_DIR := include

$(shell mkdir -p $(BUILD_DIR))

define compile
$(BUILD_DIR)/$2.o: $1 $(INCLUDE_DIR)/*.h
	$$(CC) $$(CFLAGS) -I$$(INCLUDE_DIR) -c $$< -o $$@
endef

define generate_target
$(BUILD_DIR)/$1: $(BUILD_DIR)/$1.o $(UTILS_OBJS)
	$(CC) $(CFLAGS) $$^ -o $$@
endef

all: build/BDC_command build/BDC_random build/BDS build/FC build/FS
	@echo "Done."

$(eval $(call compile,step1/BDC_random.c,BDC_random))
$(eval $(call compile,step1/BDC_command.c,BDC_command))
$(eval $(call compile,step1/BDS.c,BDS))
$(eval $(call compile,step2/FC.c,FC))
$(eval $(call compile,step2/FS.c,FS))
$(eval $(call compile,utils/socket.c,socket))
$(eval $(call compile,utils/buffer.c,buffer))

$(eval $(call generate_target,BDC_command))
$(eval $(call generate_target,BDC_random))
$(eval $(call generate_target,BDS))
$(eval $(call generate_target,FC))
$(eval $(call generate_target,FS))

clean:
	rm -rf $(BUILD_DIR)