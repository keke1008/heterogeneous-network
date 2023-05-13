.PHONY: test
test:
	pio test -e native

.PHONY: check
check:
	pio check -e native --fail-on-defect low --fail-on-defect medium --fail-on-defect high

# Generate Compilation Database for LSP and IDE
.PHONY: cmpdb
cmpdb:
	pio run -t compiledb -e megaatmega2560
