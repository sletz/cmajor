
## Libfaust with Cmajor backend 

Here is a special version of libfaust containing the Cmajor backend and compiled in JS/WebAssembly. The `libfaust-wasm.js`, `libfaust-wasm.wasm` and `libfaust-wasm.data` files are generated using Emscripten in the main Faust project. The `libfaust-wasm.data` contains all the standard Faust libraries in a single file.

The `cmajor.js` file loads the Emscripten generated files and defines the exported API needed to compile Faust DSP programs in Cmajor in a  **CmajorCompiler** class with the following functions: 

- `getLibFaustVersion()` returns the libfaust version
- `compile(dsp_name, dsp_content, argv)` compile Faust DSP as a sting in a Cmajor code as a string
- `getErrorMessage` returns error message in case of compilation error.

An HTML `cmajor.html` test file shows how to use the API.

