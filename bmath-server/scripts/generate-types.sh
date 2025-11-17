#!/bin/bash -e
mkdir -p .cache
ts-json-schema-generator \
  --path node_modules/vscode-languageserver-protocol/lib/common/protocol.d.ts \
  --type "*" \
  --expose all \
  --additional-properties \
  --out .cache/lsp.schema.json
#quicktype --help
# pass --debug all to for debugging
quicktype \
  --telemetry disable \
  --package lsp \
  --no-multi-file-output \
  --src .cache/lsp.schema.json --src-lang json \
  --lang go \
  --omit-empty \
  --out pkg/lsp/types.go
