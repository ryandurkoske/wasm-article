
# This is the main makefile.  It stitches everything together.  You do NOT need to modify it in order to add project specific modifications to your build pipeline, instead modify native.mk and wasm.mk

MK = @make --no-print-directory

.PHONY: all native wasm
.PHONY: clean clean_native clean_wasm

all:
	$(MK) -f native.mk
	$(MK) -f wasm.mk

clean:
	$(MK) -f native.mk clean
	$(MK) -f wasm.mk clean

native:
	$(MK) -f native.mk

clean_native:
	$(MK) -f native.mk clean

wasm:
	$(MK) -f wasm.mk

clean_wasm:
	$(MK) -f wasm.mk clean