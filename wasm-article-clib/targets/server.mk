###################################################
######## ADD PROJECT SPECIFIC DETAIL BELOW ########
###################################################

#add more directories containing .c files here
SRC_DIRS := src/server src/shared

#add more include folders here (changes to the headers are tracked; can only path to using quotes)
LOCAL_INCLUDES := include

#add more include folders here (changes to the headers are NOT tracked; preferably path to using angle brackets)
EXTERNAL_INCLUDES := clib/uWebsockets \
	clib/uSockets/include

#add more libs
LIBS := clib/uSockets/bin/uSockets.a

#define macros
D_FLAGS := -DTARGET_SERVER \
	-DUWS_NO_ZLIB -DUWS_MOCK_ZLIB \
	-D'NULL=((void*)0)' \
	-D'EXPORT(name)=' \
	-D'IMPORT(name)='

#additional linker flags
LD_FLAGS := -O3

#compiler warning level
WARNING_FLAGS := -Wall

#additional c compiler flags
CC_FLAGS := -O3

#additional c++ compiler flags
CXX_FLAGS := -O3

#set the root folder for intermediate build step output (.o and .d files)
BUILD_DIR := builds/server

#set the executable output
TARGET_DIR := bin
TARGET := $(TARGET_DIR)/program.exe

###################################################
### YOU SHOULDN'T NEED TO MODIFY ANYTHING PAST THIS ###
###################################################

$(info [1;35m=== SERVER TARGET ===[0m)

ifeq ($(OS),Windows_NT)
RMDIR := powershell rm -ErrorAction Ignore -r -force
RM := powershell del -ErrorAction Ignore
MKDIR_P = mkdir "$@" 
else
SHELL = /bin/bash
RMDIR := rm -R -f
RM := rm -f
MKDIR_P = mkdir -p "$@"
endif

REQUIRED_DIRS := $(TARGET_DIR) $(dir $(addprefix $(BUILD_DIR)/, $(foreach dir, $(SRC_DIRS), $(wildcard $(dir) $(dir)/*))))

.PHONY: all clean

all: $(REQUIRED_DIRS) $(TARGET)

#create any missing directories necessary for build output
$(REQUIRED_DIRS):
	$(MKDIR_P)

LD := clang++ -std=c17 -flto -fuse-ld=gold
CC := clang -std=c17 -MMD -MP
CXX := clang++ -std=c++20 -MMD -MP

INCS := $(addprefix -iquote , $(LOCAL_INCLUDES)) $(addprefix -isystem , $(EXTERNAL_INCLUDES))

SRCS := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c $(dir)/*/*.c $(dir)/*.cpp $(dir)/*/*.cpp))
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

#link (libs as deps for better error handling)
$(TARGET): $(LIBS) $(OBJS)
	$(info [32mLinking $@ ...[0m)
	$(LD) $(LD_FLAGS) $^ -o $@ $(LIBS)

#compile c files
$(BUILD_DIR)/%.c.o: %.c
	$(info [36mCompiling $*.c ...[0m)
	$(CC) $(D_FLAGS) $(WLIBC_MACROS) $(WARNING_FLAGS) $(CC_FLAGS) $(INCS) -c $< -o $@

#compile cpp files
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(info [36mCompiling $*.cpp ...[0m)
	$(CXX) $(D_FLAGS) $(WARNING_FLAGS) $(CXX_FLAGS) $(INCS) -c $< -o $@

#makefile-like dependency inclusion
-include $(OBJS:.o=.d)

clean:
	-$(RMDIR) $(BUILD_DIR)
	-$(RM) $(TARGET)