###################################################
########## OPTIONAL STATIC LIB GENERATOR ##########
###################################################

#choose an allocator (more options soon)
ALLOCATOR := walloc.c

#additional compiler flags
CC_FLAGS := -g -Wall -O3
D_FLAGS := 

#wast flags
WAT_FLAGS := -r --enable-all --disable-multi-value

# (make sure these match with your project requirements)
MDO_FLAGS := -mbulk-memory -msimd128 -matomics -mmutable-globals

###################################################
### YOU DON'T NEED TO MODIFY ANYTHING PAST THIS ###
###################################################

ifeq ($(OS),Windows_NT)
RMDIR := powershell rm -ErrorAction Ignore -r -force
RM := powershell del -ErrorAction Ignore
MKDIR_P = mkdir "$@" 
else
RMDIR := rm -R
RM := rm
MKDIR_P = mkdir -p "$@"
endif

SRC_DIR := src

SRCS := stdio.c \
	threads32.wat \
	$(ALLOCATOR)

#set the root folder for intermediate build step output (.o and .d files)
BUILD_DIR := build

#set the executable output
TARGET_DIR := bin
TARGET := $(TARGET_DIR)/wlibc.a

REQUIRED_DIRS := $(TARGET_DIR) $(BUILD_DIR)

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)


.PHONY: all clean

all: $(REQUIRED_DIRS) $(TARGET)

clean:
	-$(RMDIR) $(BUILD_DIR)
	-$(RMDIR) $(TARGET_DIR)

#create any missing directories necessary for build output
$(REQUIRED_DIRS): %:
	$(MKDIR_P)

C_PREPROCESSOR_FLAGS := -MMD \
	$(D_FLAGS) \
	$(PREFIXED_INCS)

C_COMPILER_FLAGS := -flto \
	-nostdlib \
	$(WARN_LVL) \
	$(CC_FLAGS) \
	$(MDO_FLAGS)

C_COMPILE := clang --target=wasm32 \
	-iquote include \
	 $(C_PREPROCESSOR_FLAGS) \
	 $(C_COMPILER_FLAGS)

#archive
$(TARGET): $(OBJS)
	llvm-ar rcsu $@ $^

#compile c files
$(BUILD_DIR)/%.c.o: $(SRC_DIR)/%.c
	$(C_COMPILE) -c $< -o $@

#compile WAST files
$(BUILD_DIR)/%.wat.o: $(SRC_DIR)/%.wat
	echo [36mCompiling $*.wat[0m
	wat2wasm $(WAT_FLAGS) $< -o $@

#makefile-like dependency inclusion
-include $(OBJS:.o=.d)