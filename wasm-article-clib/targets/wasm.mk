###################################################
######## ADD PROJECT SPECIFIC DETAIL BELOW ########
###################################################

#add more directories containing .c files here
SRC_DIRS := src/shared src/wasm

#add more include folders here (changes to the headers are tracked; can only path to using quotes)
LOCAL_INCLUDES := include

#add more include folders here (changes to the headers are NOT tracked; preferably path to using angle brackets)
EXTERNAL_INCLUDES := clib/wlibc/include

#add more libs (I doubt you'd have any for WASM)
LIBS := clib/wlibc/bin/wlibc.a

#compiler warning level
WARN_LVL := -Wall

#define macros
D_FLAGS := -DENV_PTR32 -DENV_WASM -DENV_RENDER -D_DEBUG

#additional linker flags
LD_FLAGS := -O3 --shared-memory --max-memory=1073741824 -zstack-size=8192

#additional compiler flags
CC_FLAGS := -g -O3

#asm flags
ASM_FLAGS := -g -O3

#wat flags
WAT_FLAGS := -r --enable-all --disable-multi-value

#machine-dependent options (aka wasm options) NOTE: building with -msimd128 breaks safari compatability as of my last test and -mexception-handling breaks most browsers
MDO_FLAGS := -mbulk-memory -msimd128 -matomics -mmutable-globals #-mexception-handling

#set the root folder for intermediate build step output (.o and .d files)
BUILD_DIR := builds/wasm

#set the executable output
TARGET_DIR := ./bin
TARGET := $(TARGET_DIR)/rolling_total.wasm

#auto-copy into website location
EXPORT := ../wasm-article-react/public/rolling_total.wasm

###################################################
### YOU DON'T NEED TO MODIFY ANYTHING PAST THIS ###
###################################################

$(info [1;35m=== WASM TARGET ===[0m)

.PHONY: all hot clean


ifeq ($(OS),Windows_NT)
RMDIR := powershell rm -ErrorAction Ignore -r -force
RM := powershell del -ErrorAction Ignore
MKDIR_P := mkdir
CP := powershell cp
else
RMDIR := rm -R -f
RM := rm -f
MKDIR_P = mkdir -p "$(dir $@)"
CP := cp
endif

REQUIRED_DIRS := $(TARGET_DIR) $(addprefix $(BUILD_DIR)/, $(foreach dir, $(SRC_DIRS), $(wildcard $(dir) $(dir)/*/)))

SRCS := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c $(dir)/*/*.c $(dir)/*.s $(dir)/*/*.s $(dir)/*.wat $(dir)/*/*.wat))
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

all: $(REQUIRED_DIRS) $(TARGET)

clean:
	$(RMDIR) $(BUILD_DIR)
	$(RM) $(TARGET)

#create any missing directories necessary for build output
$(REQUIRED_DIRS):
	$(MKDIR_P) "$@"

LD := wasm-ld-18
CC := clang-18 -std=c17 -target wasm32-unknown-unknown -flto -MMD -MP

PREFIXED_INCS := $(addprefix -iquote , $(LOCAL_INCLUDES)) $(addprefix -isystem , $(EXTERNAL_INCLUDES))

LINKER_FLAGS := --no-entry \
	--unresolved-symbols=report-all \
	--import-memory \
	--error-limit=0 \
	--stack-first \
	--gc-sections \
	$(LD_FLAGS)

C_PREPROCESSOR_FLAGS := $(D_FLAGS) \
	-D'NULL=((void*)0)' \
	-D'EXPORT(name)=__attribute__((export_name(name)))' \
	-D'IMPORT(name)=extern __attribute__((import_module("app"),import_name(name)))' \
	$(PREFIXED_INCS)

C_COMPILER_FLAGS := $(WARN_LVL) \
	$(MDO_FLAGS) \
	$(CC_FLAGS) \
	-nostdlib
	
LINK := $(LD) $(LINKER_FLAGS)
C_COMPILE := $(CC) $(C_PREPROCESSOR_FLAGS) $(C_COMPILER_FLAGS)

#link and export
$(TARGET): $(LIBS) $(OBJS)
	echo [36mLinking $@[0m
	$(LINK) $(LIBS) $(OBJS) -o $@ && echo [32mBuilt successfully![0m
	wasm2wat --enable-all $(TARGET) -o $(TARGET).wat
	wasm-opt -g -O3 --disable-multimemory $(TARGET) -o $(EXPORT) 

#compile c files
$(BUILD_DIR)/%.c.o: %.c
	echo [36mCompiling $*.c[0m
	$(C_COMPILE) -c $*.c -o $@

#compile asm files
$(BUILD_DIR)/%.s.o: %.s
	echo [36mCompiling $*.s[0m
	$(CC) -nostdlib $(ASM_FLAGS) $(MDO_FLAGS) -c $*.s -o $@

#compile wat files
$(BUILD_DIR)/%.wat.o: %.wat
	echo [36mCompiling $*.wat[0m
	wat2wasm $(WAT_FLAGS) $*.wat -o $@

#makefile-like dependency inclusion
-include $(OBJS:.o=.d)
