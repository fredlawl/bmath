# Install Steps

1. Download/install [WASM](https://webassembly.org/getting-started/developers-guide/)
2. Run: 
   ```
   PWASM_TOOLS=/path/to/emsdk/upstream/emscripten &&
   cmake \
   -DCMAKE_TOOLCHAIN_FILE=$PWASM_TOOLS/cmake/Modules/Platform/Emscripten.cmake \
   -DCMAKE_BUILD_TYPE=Release \
   -G "Unix Makefiles" \
   ./
   ```
3. Run `make`; files can not be found in the current dir