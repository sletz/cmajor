
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
            const baseArgs = `-lang cmajor-hybrid -json -I libraries`;
            const auxArgs = argv !== "" ? `${argv} ` : ""; // Notice the space after `${argv}`
            const command = `${baseArgs} ${auxArgs}-cn ${dsp_name} -o ${dsp_name}.cmajor`;
            const res = this.fCompiler.generateAuxFiles(dsp_name, dsp_content, command);
            if (res) {
                return {
                    cmajor: this.fFS.readFile(`${dsp_name}.cmajor`, { encoding: "utf8" }),
                    json: this.fFS.readFile(`${dsp_name}.json`, { encoding: "utf8" })
                };
            } else {
                return null;
            }

        } catch (e) {
            // Enhanced error handling to provide more detailed feedback
            this.fErrorMessage = this.fCompiler.getErrorAfterException() || e.toString();
            this.fCompiler.cleanupAfterException();
            return null;
        }
    };
};

class SouceTransformer {
    constructor(cmajor) {
        window.currentView = this;
        this.cmajor = cmajor;
        this.sendMessageToServer({ type: "ready" });
    }

    sendMessageToServer(message) {
        cmaj_sendMessageToServer(message);
    }

    deliverMessageFromServer(msg) {
        // console.log ("SourceTransformer received msg of type " + JSON.stringify (msg));
        if (msg.type == "transformRequest") {
            this.sendMessageToServer({
                type: "transformResponse",
                message: { contents: this.transform(msg.message.filename, msg.message.contents) }
            });
        }
    }

    transform(filename, contents) {
        if (filename.endsWith(".dsp")) {
            let prefix = filename.substr(0, filename.length - 4);
            let { cmajor, json } = this.cmajor.compile(prefix, contents, "");
            console.log(json);
            return cmajor;
        }
        // console.log(contents);
        return contents;
    }
}

export default async function runWorker() {

    // Using the bundled version of the Faust library
    const { instantiateFaustModule } = await import("./index.js");
    let module = await instantiateFaustModule();

    let cmajor = new CmajorCompiler(module);
    console.log("Running source transformer");
    const connection = new SouceTransformer(cmajor);
}
