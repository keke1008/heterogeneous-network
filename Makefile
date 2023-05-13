.PHONY: test
test:
	pio test -e native

# Generate Compilation Database for LSP and IDE
.PHONY: cmpdb
cmpdb:
	pio run -t compiledb -e megaatmega2560
