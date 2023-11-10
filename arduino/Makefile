.PHONY: build
build:
	pio run -e debug

.PHONY: upload
upload:
	pio run -e debug -t upload

.PHONY: release
release:
	pio run -e release -t upload

.PHONY: release-build
release-build:
	pio run -e release


.PHONY: monitor
monitor:
	pio device monitor

.PHONY: test
test:
	pio test -e test -v

.PHONY: check
check:
	pio check -e test --fail-on-defect low --fail-on-defect medium --fail-on-defect high

.PHONY: doc
doc:
	doxygen Doxyfile

# Generate Compilation Database for LSP and IDE
.PHONY: cmpdb
cmpdb:
	pio run -t compiledb -e debug

.PHONY: test-cmpdb
test-cmpdb:
	pio run -t compiledb -e test
