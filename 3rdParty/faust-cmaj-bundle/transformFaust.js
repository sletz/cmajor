/**
 * Class for parsing JSON UI and creating input events for the Faust program.
 */
class JSONParser {

    /**
     * Replaces specified characters in a label with an underscore.
     * @param {string} label - The input label string.
     * @returns {string} The modified label string with specified characters replaced.
     */
    static buildLabel(label) {
        return this.replaceCharList(label, [' ', '(', ')', '\\', '/', '.', '-'], '_');
    }

    /**
     * Replaces a list of characters in a string with a replacement character.
     * @param {string} str - The input string.
     * @param {string[]} chars - The list of characters to be replaced.
     * @param {string} replacement - The replacement character.
     * @returns {string} The modified string with specified characters replaced.
     */
    static replaceCharList(str, chars, replacement) {
        let regex = new RegExp(`[${chars.join('\\')}]`, 'g');
        return str.replace(regex, replacement);
    }

    /**
     * Parses the UI descriptor and applies a callback to each item.
     * @param {Object[]} ui - The UI descriptor array.
     * @param {function} callback - The callback function to apply to each item.
     */
    static parseUI(ui, callback) {
        ui.forEach(group => this.parseGroup(group, callback));
    }

    /**
     * Parses a UI group and applies a callback to each item in the group.
     * @param {Object} group - The UI group object.
     * @param {function} callback - The callback function to apply to each item.
     */
    static parseGroup(group, callback) {
        if (group.items) {
            this.parseItems(group.items, callback);
        }
    }

    /**
     * Parses an array of UI items and applies a callback to each item.
     * @param {Object[]} items - The array of UI items.
     * @param {function} callback - The callback function to apply to each item.
     */
    static parseItems(items, callback) {
        items.forEach(item => this.parseItem(item, callback));
    }

    /**
     * Parses a UI item and applies a callback to the item or its children.
     * @param {Object} item - The UI item object.
     * @param {function} callback - The callback function to apply to the item.
     */
    static parseItem(item, callback) {
        if (item.type === "vgroup" || item.type === "hgroup" || item.type === "tgroup") {
            this.parseItems(item.items, callback);
        } else {
            callback(item);
        }
    }

    /**
     * Creates the input events for the Faust program based on the JSON UI.
     * @param {string} json_str - The JSON UI string.
     * @returns {string[]} The input events as an array of strings.
     */
    static printInputs(json_str) {
        let cmajor_events = [];
        let json = JSON.parse(json_str);
        console.log(`// Input events for "${json.name}"`);
        this.parseUI(json.ui, item => {
            let event;
            if (['vslider', 'hslider', 'nentry'].includes(item.type)) {
                event = `input event float32 ${this.buildLabel(item.label)} [[ name: "${item.label}", min: ${item.min}, max: ${item.max}, init: ${item.init}, step: ${item.step} ]];`;
            } else if (['button', 'checkbox'].includes(item.type)) {
                event = `input event float32 ${this.buildLabel(item.label)} [[ name: "${item.label}", text: "off|on", boolean ]];`;
            }
            if (event) {
                cmajor_events.push(event);
                console.log(event);
            }
        });
        return cmajor_events;
    }
}

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
    * @param {string} dsp_name - The name of the DSP program to compile.
    * @param {string} dsp_content - The content of the DSP program.
    * @param {string} argv - Additional arguments for the compilation process.
    * @returns {Object|null} An object containing the contents of the compiled .cmajor and .json files,
    *                        or null if the compilation fails or an error occurs.
    * 
    * The return object has the following structure if successful:
    * {
    *      cmajor: string, // The content of the compiled .cmajor file
    *      json: string    // The content of the compiled .json file
    * }
    */
    compile(dsp_name, dsp_content, argv) {
        try {
            // Customize the compilation arguments
            const baseArgs = `-lang cmajor-hybrid -json`;
            const auxArgs = argv !== "" ? `${argv} ` : ""; // Notice the space after `${argv}`
            const command = `${baseArgs} ${auxArgs}-cn ${dsp_name} -o ${dsp_name}.cmajor`;
            this.fCompiler.generateAuxFiles(dsp_name, dsp_content, command);
            return {
                cmajor: this.fFS.readFile(`${dsp_name}.cmajor`, { encoding: "utf8" }),
                json: this.fFS.readFile(`${dsp_name}.json`, { encoding: "utf8" })
            };
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
            console.log("Faust library version: " + this.cmajor.getLibFaustVersion());
            let res = this.cmajor.compile(prefix, contents, "");
            // Parse the JSON UI to generate the input events
            if (res) {
                JSONParser.printInputs(res.json);
                return res.cmajor;
            } else {
                console.error(this.cmajor.getErrorMessage());
                return contents;
            }
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
