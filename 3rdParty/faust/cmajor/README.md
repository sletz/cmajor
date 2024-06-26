
## Libfaust with Cmajor backend 

Here is a special version of libfaust containing the Cmajor backend and compiled in JS/WebAssembly. The `libfaust-wasm.js`, `libfaust-wasm.wasm` and `libfaust-wasm.data` files are generated using Emscripten in the main Faust project. The `libfaust-wasm.data` contains all the standard Faust libraries in a single file.

The `cmajor.js` file loads the Emscripten generated files and defines the exported API needed to compile Faust DSP programs in Cmajor in a  **CmajorCompiler** class with the following functions: 

- `getLibFaustVersion()` returns the libfaust version
- `compile(dsp_name, dsp_content, argv)` compiles a Faust DSP program as a string in a pair containing the Cmajor code and the JSON file
- `getErrorMessage()` returns the error message in case of compilation error

An HTML `cmajor.html` test file shows how to use the API. The produced code is visible in the JavaScript console.

