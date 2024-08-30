# This is the main makefile.  It stitches everything together. You should not need to call ur targets makefiles directly.

# You do NOT need to modify it in order to add project specific modifications 
# to your build pipeline, instead modify native.mk and wasm.mk

# Do modify this if you are adding a NEW target

MK = @make --no-print-directory

.PHONY: all server wasm
.PHONY: clean server_clean wasm_clean

all:
	$(MK) -s -f ./targets/server.mk
	$(MK) -s -f ./targets/wasm.mk

clean:
	$(MK) -f ./targets/server.mk clean
	$(MK) -f ./targets/wasm.mk clean

server:
	$(MK) -f ./targets/server.mk

server_clean:
	$(MK) -f ./targets/server.mk clean

wasm:
	$(MK) -f ./targets/wasm.mk

wasm_clean:
	$(MK) -f ./targets/wasm.mk clean