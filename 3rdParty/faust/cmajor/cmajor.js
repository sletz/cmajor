/************************************************************************
 FAUST Architecture File
 Copyright (C) 2024 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This Architecture section is free software; you can redistribute it
 and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of
 the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; If not, see <http://www.gnu.org/licenses/>.
 
 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 
 ************************************************************************
 ************************************************************************/

'use strict';

// Using libfaust compiled with MODULARIZE=1

class CmajorCompiler {
    fModule;
    fCompiler;
    fFS;
    fErrorMessage;

    // Initializes the class with necessary WebAssembly module components
    constructor(module) {
        this.fModule = module;
        this.fCompiler = new module.libFaustWasm();
        this.fFS = module.FS;
        this.fErrorMessage = "";
    };

    // Returns the version of the FAUST library being used
    getLibFaustVersion() {
        return this.fCompiler.version();
    };

    // Retrieves the last compilation error message
    getErrorMessage() {
        return this.fErrorMessage;
    };

    /**
     * Compiles a FAUST program with specified parameters.
     * 
     * @param {string} dsp_name - The name of the DSP program.
     * @param {string} dsp_content - The FAUST program code.
     * @param {string} argv - Additional arguments for the compiler as a string.
     * @returns {string|null} The compiled program, or null if compilation fails.
     */
    compile(dsp_name, dsp_content, argv) {
        try {
            // Customize the compilation arguments
            argv = argv + "-lang cmajor-hybrid -I libraries -cn " + dsp_name + " -o foo.cmajor";
            const res = this.fCompiler.generateAuxFiles(dsp_name, dsp_content, argv);
            return (res) ? this.fFS.readFile("foo.cmajor", { encoding: "utf8" }) : null;
        } catch (e) {
            // Enhanced error handling to provide more detailed feedback
            this.fErrorMessage = this.fCompiler.getErrorAfterException() || e.toString();
            this.fCompiler.cleanupAfterException();
            return null;
        }
    };
};
