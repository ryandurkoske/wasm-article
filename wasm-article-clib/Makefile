ifeq ($(OS),Windows_NT)
RMDIR := powershell rm -ErrorAction Ignore -r -force
RM := powershell del -ErrorAction Ignore
MKDIR_BUILD = if not exist "$(dir $@)" mkdir "$(dir $@)"
else
RMDIR := rm -R
RM := rm
MKDIR_BUILD = mkdir -p "$(dir $@)"
endif

SRC_DIR := src
NATIVE_SRC_DIR := $(SRC_DIR)/native
SHARED_SRC_DIR := $(SRC_DIR)/shared
WASM_SRC_DIR := $(SRC_DIR)/wasm

#add more native folders here
NATIVE_SRC := $(wildcard $(NATIVE_SRC_DIR)/*.c) \
	$(wildcard $(NATIVE_SRC_DIR)/*.cpp) \

#add more shared folders here
SHARED_SRC := $(wildcard $(SHARED_SRC_DIR)/*.c)

#add more wasm folders here
WASM_SRC := $(wildcard lib/wlibc/src/*.c) \
	$(wildcard $(WASM_SRC_DIR)/*.c)

OBJ_DIR := builds
NATIVE_OBJ_DIR := $(OBJ_DIR)/native
WASM_OBJ_DIR := $(OBJ_DIR)/wasm

NATIVE_BIN_DIR := bin
NATIVE_TARGET := $(NATIVE_BIN_DIR)/program.exe

WASM_BIN_DIR := ../wasm-article-react/public
WASM_TARGET := $(WASM_BIN_DIR)/rolling_total.wasm

.PHONY: all native wasm clean clean_native clean_wasm

all: $(NATIVE_TARGET) $(WASM_TARGET)

native: $(NATIVE_TARGET)

SRCS := $(SHARED_SRC) $(NATIVE_SRC)
OBJS := $(SRCS:%=$(NATIVE_OBJ_DIR)/%.o)

D_FLAGS := -DTARGET_NATIVE -DUWS_NO_ZLIB -DUWS_MOCK_ZLIB


$(NATIVE_TARGET): LDFLAGS := -O3 -flto
$(NATIVE_TARGET): LDLIBS := lib/uSockets/lib/uSockets.a

$(NATIVE_TARGET): LD := clang++ -std=c++20


$(NATIVE_TARGET): $(OBJS)
	$(LD) -o $@ $(LDFLAGS) $^ $(LDLIBS)

$(NATIVE_OBJ_DIR)/%.c.o: C_PREPROCESSOR_FLAGS := -Iinclude -MMD -MP -Ilib/uSockets/src -Ilib/uWebsockets
$(NATIVE_OBJ_DIR)/%.c.o: COMPILER_FLAGS := -O3 -flto -Wall $(D_FLAGS)
$(NATIVE_OBJ_DIR)/%.c.o: CC := clang

$(NATIVE_OBJ_DIR)/%.c.o: %.c
	@$(MKDIR_BUILD)
	$(CC) $(C_PREPROCESSOR_FLAGS) $(COMPILER_FLAGS) -c $< -o $@

$(NATIVE_OBJ_DIR)/%.cpp.o: C_PREPROCESSOR_FLAGS := -Iinclude -MMD -MP -Ilib/uSockets/src -Ilib/uWebsockets
$(NATIVE_OBJ_DIR)/%.cpp.o: COMPILER_FLAGS := -O3 -flto -Wall $(D_FLAGS)
$(NATIVE_OBJ_DIR)/%.cpp.o: CXX := clang++ -std=c++20

$(NATIVE_OBJ_DIR)/%.cpp.o: %.cpp
	@$(MKDIR_BUILD)
	$(CXX) $(C_PREPROCESSOR_FLAGS) $(COMPILER_FLAGS) -c $< -o $@


wasm: $(WASM_TARGET)

SRCS := $(WASM_SRC) $(SHARED_SRC)
OBJS := $(SRCS:%=$(WASM_OBJ_DIR)/%.o)

D_FLAGS := -DTARGET_WASM

$(WASM_OBJ_DIR)/%.c.o: C_PREPROCESSOR_FLAGS := -Iinclude -isystem lib/wlibc/include -MMD -MP
$(WASM_OBJ_DIR)/%.c.o: COMPILER_FLAGS := -Wall \
	-Ofast \
	-nostdlib \
	-fvisibility=hidden \
	-ffunction-sections \
	-fdata-sections \
	$(D_FLAGS)

$(WASM_TARGET): LDFLAGS := -Llib \
	--no-entry \
	--strip-all \
	--export-dynamic \
	--allow-undefined \
	--import-memory \
	--error-limit=0 \
	--lto-O3 \
	-O3 \
	--gc-sections
$(WASM_TARGET): LDLIBS := 

$(WASM_TARGET): LD := wasm-ld
$(WASM_OBJ_DIR)/%.c.o: CC := clang --target=wasm32 -mbulk-memory #-msimd128  -mexception-handling

$(WASM_TARGET): $(OBJS)
	$(LD) -o $@ $(LDFLAGS) $(LDLIBS) $^

$(WASM_OBJ_DIR)/%.c.o: %.c
	@$(MKDIR_BUILD)
	$(CC) $(C_PREPROCESSOR_FLAGS) $(COMPILER_FLAGS) -c $< -o $@


clean: clean_native clean_wasm

clean_native:
	-$(RMDIR) $(NATIVE_OBJ_DIR)
	-$(RM) $(NATIVE_TARGET)

clean_wasm:
	-$(RMDIR) $(WASM_OBJ_DIR)
	-$(RM) $(WASM_TARGET)