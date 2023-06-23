.PHONY: run
run:
	pio run -e megaatmega2560

.PHONY: test
test:
	pio test -e native

.PHONY: check
check:
	pio check -e native --fail-on-defect low --fail-on-defect medium --fail-on-defect high

.PHONY: doc
doc:
	doxygen Doxyfile

# Generate Compilation Database for LSP and IDE
.PHONY: cmpdb
cmpdb:
	pio run -t compiledb -e megaatmega2560

.PHONY: native-cmpdb
native-cmpdb:
	pio run -t compiledb -e native
