###################################################
######## ADD PROJECT SPECIFIC DETAIL BELOW ########
###################################################

#add more directories containing .c files here
SRC_DIRS := src/shared \
	src/native

#add more include folders here (changes to the headers are tracked; can only path to using quotes)
LOCAL_INCLUDES := include

#add more include folders here (changes to the headers are NOT tracked; preferably path to using angle brackets)
EXTERNAL_INCLUDES := lib/uWebsockets \
	lib/uSockets/include

#add more libs
LIBS := lib/uSockets/bin/uSockets.a

#define macros
D_FLAGS := -DTARGET_NATIVE \
	-DUWS_NO_ZLIB -DUWS_MOCK_ZLIB

#additional linker flags
LD_FLAGS :=

#compiler warning level
WARN_LVL := -Wall

#additional compiler flags for c and c++
SHARED_FLAGS :=

#additional c compiler flags
CC_FLAGS :=

#additional c++ compiler flags
CXX_FLAGS :=

#set the root folder for intermediate build step output (.o and .d files)
BUILD_DIR := builds/native

#set the executable output
TARGET_DIR := bin
TARGET := $(TARGET_DIR)/program.exe

###################################################
### YOU DON'T NEED TO MODIFY ANYTHING PAST THIS ###
###################################################

$(info )
$(info === NATIVE TARGET ===)

-include env.mk

REQUIRED_DIRS := $(addprefix $(BUILD_DIR)/, $(SRC_DIRS)) $(TARGET_DIR)

.PHONY: native clean

native: $(REQUIRED_DIRS) $(TARGET)

#create any missing directories necessary for build output
$(REQUIRED_DIRS): %:
	$(MKDIR_P)

LD := clang++ -std=c++20
CC := clang
CXX := clang++ -std=c++20

PREFIXED_INCS := $(addprefix -iquote , $(LOCAL_INCLUDES)) $(addprefix -isystem , $(EXTERNAL_INCLUDES))

SRCS := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c) $(wildcard $(dir)/*.cpp))
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

LINKER_FLAGS := -O3 -flto -fuse-ld=gold

C_PREPROCESSOR_FLAGS := -MMD -MP $(D_FLAGS) $(PREFIXED_INCS)
CXX_PREPROCESSOR_FLAGS := -MMD -MP $(D_FLAGS) $(PREFIXED_INCS)

C_COMPILER_FLAGS := $(WARN_LVL) -O3 -flto $(SHARED_FLAGS) $(CC_FLAGS)
CXX_COMPILER_FLAGS := $(WARN_LVL) -O3 -flto $(SHARED_FLAGS) $(CXX_FLAGS)

LINK := $(LD) $(LINKER_FLAGS)
C_COMPILE := $(CC) $(C_PREPROCESSOR_FLAGS) $(C_COMPILER_FLAGS)
CXX_COMPILE := $(CXX) $(CXX_PREPROCESSOR_FLAGS) $(CXX_COMPILER_FLAGS)

#link
$(TARGET): $(OBJS)
	$(LINK) $^ -o $@ $(LIBS)

#compile c files
$(BUILD_DIR)/%.c.o: %.c
	$(C_COMPILE) -c $< -o $@

#compile cpp files
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(CXX_COMPILE) -c $< -o $@

#makefile-like dependency inclusion
-include $(OBJS:.o=.d)

clean:
	-$(RMDIR) $(BUILD_DIR)
	-$(RM) $(TARGET)