TARGET ?= bin/bmath-ls
FMT_TARGET ?= bin/bmath-format
GO_FILES := $(shell find . -type f -name '*.go' -not -path './vendor/*')
ARCH := $(shell uname -m)
CGO_ENABLED ?= 1
CGO_CFLAGS ?= -I/usr/local/include/bmath
CGO_LDFLAGS ?= -L/usr/local/lib -L/usr/local/lib/$(ARCH)-linux-gnu

export CGO_ENABLED := $(CGO_ENABLED)
export CGO_CFLAGS := $(CGO_CFLAGS)
export CGO_LDFLAGS := $(CGO_LDFLAGS)

.PHONY: all
all: $(TARGET) $(FMT_TARGET)

.PHONY: test
test:
	go test -v ./...

.PHONY: compile-tests
compile-tests:
	go test -gcflags "-N -l" -o bin -c ./...
#	go test  -o bin -c ./...

.PHONY: clean
clean:
	go clean -a
	-rm -rf ./bin

.PHONY: generate
generate: export PATH := $(HOME)/go/bin:$(PATH)
generate: $(HOME)/go/bin/stringer
	go generate ./...

$(HOME)/go/bin/stringer:
	go install golang.org/x/tools/cmd/stringer@latest

$(TARGET): $(GO_FILES) pkg/lsp/types.go
	mkdir -p $(shell dirname $@)
	go build -work -v -o $@ cmd/ls/main.go

$(FMT_TARGET): $(GO_FILES) pkg/lsp/types.go
	mkdir -p $(shell dirname $@)
	go build -o $@ cmd/format/main.go

#node_modules:
#	npm install
#
#pkg/lsp/types.go: node_modules
#	npm run gen:lsp
