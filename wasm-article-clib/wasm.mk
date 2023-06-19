###################################################
######## ADD PROJECT SPECIFIC DETAIL BELOW ########
###################################################

#add more directories containing .c files here
SRC_DIRS := lib/wlibc/src \
	src/shared \
	src/wasm

#add more include folders here (changes to the headers are tracked; can only path to using quotes)
LOCAL_INCLUDES := include

#add more include folders here (changes to the headers are NOT tracked; preferably path to using angle brackets)
EXTERNAL_INCLUDES := lib/wlibc/include

#add more libs (I doubt you'd have any for WASM)
LIBS :=

#define macros
D_FLAGS := -DTARGET_WASM

#additional linker flags
LD_FLAGS :=

#compiler warning level
WARN_LVL := -Wall

#additional compiler flags
CC_FLAGS :=

#machine-dependent options (aka wasm options) NOTE: building with -msimd128 breaks safari compatability as of my last test and -mexception-handling breaks most browsers
MDO_FLAGS := -mbulk-memory# -msimd128  -mexception-handling

#set the root folder for intermediate build step output (.o and .d files)
BUILD_DIR := builds/wasm

#set the executable output
TARGET_DIR := ../wasm-article-react/public
TARGET := $(TARGET_DIR)/rolling_total.wasm

###################################################
### YOU DON'T NEED TO MODIFY ANYTHING PAST THIS ###
###################################################

$(info )
$(info === WASM TARGET ===)

-include env.mk

REQUIRED_DIRS := $(addprefix $(BUILD_DIR)/, $(SRC_DIRS))

.PHONY: wasm clean

wasm: $(REQUIRED_DIRS) $(TARGET)

#create any missing directories necessary for build output
$(REQUIRED_DIRS): %:
	$(MKDIR_P)

LD := wasm-ld
CC := clang --target=wasm32

PREFIXED_INCS := $(addprefix -iquote , $(LOCAL_INCLUDES)) $(addprefix -isystem , $(EXTERNAL_INCLUDES))

SRCS := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

LINKER_FLAGS := --no-entry \
	--strip-all \
	--export-dynamic \
	--allow-undefined \
	--import-memory \
	--error-limit=0 \
	--lto-O3 \
	-O3 \
	--gc-sections

C_PREPROCESSOR_FLAGS := -MMD -MP $(D_FLAGS) $(PREFIXED_INCS)

C_COMPILER_FLAGS := $(WARN_LVL) \
	-flto \
	-O3 \
	-nostdlib \
	-fvisibility=hidden \
	-ffunction-sections \
	-fdata-sections \
	$(CC_FLAGS) \
	$(MDO_FLAGS)

LINK := $(LD) $(LINKER_FLAGS)
C_COMPILE := $(CC) $(C_PREPROCESSOR_FLAGS) $(C_COMPILER_FLAGS)

#link
$(TARGET): $(OBJS)
	$(LINK) $^ -o $@ $(LIBS)

#compile c files
$(BUILD_DIR)/%.c.o: %.c
	$(C_COMPILE) -c $< -o $@

#makefile-like dependency inclusion
-include $(OBJS:.o=.d)

clean:
	-$(RMDIR) $(BUILD_DIR)
	-$(RM) $(TARGET)