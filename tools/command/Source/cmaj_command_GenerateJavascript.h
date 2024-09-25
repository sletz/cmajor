//
//     ,ad888ba,                              88
//    d8"'    "8b
//   d8            88,dba,,adba,   ,aPP8A.A8  88     The Cmajor Toolkit
//   Y8,           88    88    88  88     88  88
//    Y8a.   .a8P  88    88    88  88,   ,88  88     (C)2024 Cmajor Software Ltd
//     '"Y888Y"'   88    88    88  '"8bbP"Y8  88     https://cmajor.dev
//                                           ,88
//                                        888P"
//
//  The Cmajor project is subject to commercial or open-source licensing.
//  You may use it under the terms of the GPLv3 (see www.gnu.org/licenses), or
//  visit https://cmajor.dev to learn about our commercial licence options.
//
//  CMAJOR IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
//  EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
//  DISCLAIMED.

#pragma once

namespace generate_javascript
{

inline std::string generateJavascriptWorklet (cmaj::Patch& patch, const cmaj::Patch::LoadParams& loadParams, const choc::value::Value& engineOptions)
{
    static constexpr const auto generatedModuleSourceText = R"(
//==============================================================================
//
//  This file contains a Javascript/Webassembly/WebAudio export of the Cmajor
//  patch 'PATCH_FILE'.
//
//  This file was auto-generated by the Cmajor toolkit vCMAJ_VERSION
//
//  To use it, import this module into your HTML/Javascript code and call
//  `createAudioWorkletNodePatchConnection()`. The AudioWorkletPatchConnection
//  object that is returned is a PatchConnection with some extra functionality
//  to let you connect it to web audio/MIDI.
//
//  For more details about Cmajor, visit https://cmajor.dev
//
//==============================================================================

import * as helpers from "./cmaj_api/cmaj-audio-worklet-helper.js"


//==============================================================================
/** This exports the patch's manifest, in case a caller needs to find out about its properties.
 */
export const manifest =
MANIFEST_JSON;

/** Returns the patch's output endpoint list */
export function getOutputEndpoints() { return MAIN_CLASS_NAME.prototype.getOutputEndpoints(); }

/** Returns the patch's input endpoint list */
export function getInputEndpoints()  { return MAIN_CLASS_NAME.prototype.getInputEndpoints(); }

//==============================================================================
/**  Creates an audio worklet node for the patch with the given name, attaches it
 *   to the audio context provided, and returns an object containing the node
 *   and a PatchConnection class to control it.
 *
 *   @param {AudioContext} audioContext - a web audio AudioContext object
 *   @param {string} workletName - the name to give the new worklet that is created
 *   @returns {AudioWorkletPatchConnection} an AudioWorkletPatchConnection which has been initialised
 */
export async function createAudioWorkletNodePatchConnection (audioContext, workletName)
{
  const connection = new helpers.AudioWorkletPatchConnection (manifest);

  await connection.initialise ({ CmajorClass: MAIN_CLASS_NAME,
                                 audioContext,
                                 workletName,
                                 hostDescription: "WebAudio" });
  return connection;
}

CMAJOR_WRAPPER_CLASS

)";

    auto wrapper = generateCodeAndCheckResult (patch, loadParams, "javascript", choc::json::toString (engineOptions));
    auto manifest = choc::text::trim (choc::json::toString (loadParams.manifest.manifest, true));

    return choc::text::trim (choc::text::replace (generatedModuleSourceText,
                                                  "CMAJ_VERSION", cmaj::Library::getVersion(),
                                                  "PATCH_FILE", loadParams.manifest.manifestFile,
                                                  "CMAJOR_WRAPPER_CLASS", wrapper.generatedCode,
                                                  "MAIN_CLASS_NAME", wrapper.mainClassName,
                                                  "MANIFEST_JSON", manifest)) + "\n";
}

//==============================================================================
inline GeneratedFiles generateWebAudioHTML (cmaj::Patch& patch, const cmaj::Patch::LoadParams& loadParams, const choc::value::Value& engineOptions)
{
    auto& manifest = loadParams.manifest;

    // NB: had weird problems in some cases serving files that began with an underscore.
    auto cleanedName = choc::javascript::makeSafeIdentifier (loadParams.manifest.name);
    auto patchModuleFile = "cmaj_" + std::string (choc::text::trimCharacterAtStart (cleanedName, '_')) + ".js";

    std::string patchLink, patchDesc;

    if (auto url = manifest.manifest["URL"]; url.isString() && ! url.toString().empty())
        patchLink = choc::text::replace (R"(<a href="URL" class=cmaj-small-text>URL</a>)", "URL", url.toString());

    if (! manifest.description.empty() && manifest.description != manifest.name)
        patchDesc = "<span class=cmaj-small-text>" + manifest.description + "</span>";

    auto readme = choc::text::trimStart (choc::text::replace (R"(
### Auto-generated HTML & Javascript for Cmajor Patch "PATCH_NAME"

This folder contains some self-contained HTML/Javascript files that play and show a Cmajor
patch using WebAssembly and WebAudio.

For `index.html` to display correctly, this folder needs to be served as HTTP, so if you're
running it locally, you'll need to start a webserver that serves this folder, and then
point your browser at whatever URL your webserver provides. For example, you could run
`python3 -m http.server` in this folder, and then browse to the address it chooses.

The files have all been generated using the Cmajor command-line tool:
```
cmaj generate --target=webaudio --output=<location of this folder> <path to the .cmajorpatch file to convert>
```

- `index.html` is a minimal page that creates the javascript object that implements the patch,
   connects it to the default audio and MIDI devices, and displays its view.
- `CMAJOR_PATCH_JS` - this is the Javascript wrapper class for the patch, encapsulating its
   DSP as webassembly, and providing an API that is used to both render the audio and
   control its properties.
- `cmaj_api` - this folder contains javascript helper modules and resources.

To learn more about Cmajor, visit [cmajor.dev](cmajor.dev)
)",
                    "PATCH_NAME", manifest.name,
                    "CMAJOR_PATCH_JS", patchModuleFile));

    auto html = choc::text::trimStart (choc::text::replace (R"html(
<!DOCTYPE html><html lang="en">
<head>
  <meta charset="utf-8" />
  <title>Cmajor Patch</title>
</head>

<body>
  <div id="cmaj-main">
    <div id="cmaj-view-container"></div>
    <div id="cmaj-overlay">
      <div id="cmaj-info">
        <span>PATCH_NAME</span>
        PATCH_DESC
        PATCH_LINK
        <span id="cmaj-click-to-start">- Click to Start -</span>
      </div>
    </div>
    <button id="cmaj-reset-button">Stop Audio</button>
  </div>
</body>

<style>
    * { box-sizing: border-box; padding: 0; margin: 0; border: 0; }
    html { background: black; overflow: hidden; }
    body { padding: 0.5rem; display: block; position: absolute; width: 100%; height: 100%; }

    #cmaj-main {
      display: flex;
      flex-direction: column;
      color: white;
      font-family: Monaco, Consolas, monospace;
      width: 100%;
      height: 100%;
    }

    #cmaj-view-container {
      display: block;
      position: relative;
      width: 100%;
      height: 100%;
      overflow: auto;
    }

    .cmaj-piano-keyboard {
      width: 100%;
      height: 5rem;
      align-self: center;
      margin: 0.1rem;
      overflow: hidden;
    }

    #cmaj-header-bar { position: relative; height: 1.5rem; min-height: 1.5rem; overflow: hidden; display: flex; align-items: center; margin-bottom: 2px; user-select: none; }
    #cmaj-header-content { height: 100%; overflow: hidden; display: flex; align-items: center; flex-grow: 1; }
    #cmaj-header-logo { height: 100%; fill-opacity: 0.8; }
    #cmaj-header-title { height: 100%; flex-grow: 1; text-align: center; padding-right: 1rem; }

    #cmaj-click-to-start { margin-top: 3rem; opacity: 0; }

    #cmaj-reset-button { display: none; position: absolute; left: 0; bottom: 0; height: 2rem; margin: 0.1rem; padding: 0.3rem; opacity: 0.4; }
    #cmaj-reset-button:hover  { opacity: 0.5; }

    #cmaj-overlay { border: none;
                    position: absolute;
                    top: 0;
                    left: 0;
                    width: 100%;
                    height: 100%;
                    display: flex;
                    flex-direction: column;
                    justify-content: center;
                    align-items: center;
                    cursor: pointer;
                    background: rgb(0,0,0);
                    background: radial-gradient(circle, #333333ee 0%, #333333cc 100%);
                    z-index: 10;
                    text-shadow: 0 0 0.2rem black, 0 0 0.5rem black, 0 0 1rem black, 0 0 3rem black;

    }

    #cmaj-overlay * { padding: 0.5rem; }
    #cmaj-overlay a { color: rgb(120, 120, 184); }

    #cmaj-info { display: flex; flex-direction: column; justify-content: center; align-items: center; flex-grow: 1; }

    .cmaj-small-text { font-size: 75%; }
</style>

<script type="module">

import * as patch from "./PATCH_MODULE_FILE"
import { createPatchViewHolder } from "./cmaj_api/cmaj-patch-view.js"
import PianoKeyboard from "./cmaj_api/cmaj-piano-keyboard.js"

customElements.define ("cmaj-panel-piano-keyboard", PianoKeyboard);

let keyboard = null;

function removePianoKeyboard (connection)
{
    if (keyboard)
    {
        const main = document.getElementById ("cmaj-main");
        main.removeChild (keyboard);

        keyboard.detachPatchConnection (connection);
        keyboard = null;
    }
}

function getMIDIInputEndpointID (connection)
{
    for (const i of connection.inputEndpoints)
        if (i.purpose === "midi in")
            return i.endpointID;
}

function createPianoKeyboard (connection)
{
    removePianoKeyboard (connection);

    const midiInputEndpointID = getMIDIInputEndpointID (connection);

    if (midiInputEndpointID)
    {
        const main = document.getElementById ("cmaj-main");
        keyboard = new PianoKeyboard();
        keyboard.classList.add ("cmaj-piano-keyboard");
        main.appendChild (keyboard);

        keyboard.attachToPatchConnection (connection, midiInputEndpointID);
    }
}

//==============================================================================
async function loadPatch()
{
    const audioContext = new AudioContext();
    audioContext.suspend();
    const connection = await patch.createAudioWorkletNodePatchConnection (audioContext, "cmaj-worklet-processor");

    const viewContainer = document.getElementById ("cmaj-view-container");
    const startOverlay = document.getElementById ("cmaj-overlay");
    const resetButton = document.getElementById ("cmaj-reset-button");

    viewContainer.innerHTML = "";

    const view = await createPatchViewHolder (connection);

    if (view)
        viewContainer.appendChild (view)

    resetButton.onclick = () =>
    {
        removePianoKeyboard (connection);
        audioContext?.close();
        connection?.close?.();

        viewContainer.innerHTML = "";
        viewContainer.style.transform = "none";
        startOverlay.style.display = "flex";
        resetButton.style.display = "none";

        loadPatch();
    }

    startOverlay.onclick = () =>
    {
        startOverlay.style.display = "none";
        resetButton.style.display = "block";
        connection.connectDefaultAudioAndMIDI (audioContext);
        audioContext.resume();

        if (connection?.manifest?.isInstrument
             && ! view.hasOnscreenKeyboard)
            createPianoKeyboard (connection);
    };

    document.getElementById ("cmaj-click-to-start").style.opacity = 1.0;
}

loadPatch();

</script>
</html>
)html",
                    "PATCH_MODULE_FILE", patchModuleFile,
                    "PATCH_NAME", manifest.name,
                    "PATCH_DESC", patchDesc,
                    "PATCH_LINK", patchLink));

    GeneratedFiles generatedFiles;

    generatedFiles.addFile (patchModuleFile, generateJavascriptWorklet (patch, loadParams, engineOptions));
    generatedFiles.addFile ("index.html", html);
    generatedFiles.addFile ("README.md", readme);

    generatedFiles.addPatchResources (manifest);
    generatedFiles.sort();

    return generatedFiles;
}


//==============================================================================
inline GeneratedFiles generateWebAudioModule (cmaj::Patch& patch, const cmaj::Patch::LoadParams& loadParams, const choc::value::Value& engineOptions)
{
    (void) patch;
    (void) engineOptions;

    auto& manifest = loadParams.manifest;

    // NB: had weird problems in some cases serving files that began with an underscore.
    auto cleanedName = choc::javascript::makeSafeIdentifier (loadParams.manifest.name);
    auto patchModuleFile = "cmaj_" + std::string (choc::text::trimCharacterAtStart (cleanedName, '_')) + ".js";

    std::string patchLink, patchDesc;

    if (auto url = manifest.manifest["URL"]; url.isString() && ! url.toString().empty())
        patchLink = choc::text::replace (R"(<a href="URL" class=cmaj-small-text>URL</a>)", "URL", url.toString());

    if (! manifest.description.empty() && manifest.description != manifest.name)
        patchDesc = "<span class=cmaj-small-text>" + manifest.description + "</span>";


    auto index_js = choc::text::trimStart (choc::text::replace (R"(
import { WebAudioModule } from './sdk/index.js';
import { CompositeAudioNode, ParamMgrFactory } from './sdk/parammgr.js';
import * as patch from "./PATCH_MODULE_FILE";
import { createPatchViewHolder } from "./cmaj_api/cmaj-patch-view.js"

const getBaseUrl = (relativeURL) => {
  const baseURL = relativeURL.href.substring(0, relativeURL.href.lastIndexOf('/'));
  return baseURL;
};

class CmajNode extends CompositeAudioNode
{
  constructor (context, options)
  {
    super (context, options);
  }

  setup (patchConnection, paramManagerNode)
  {
    this.patchConnection = patchConnection;

    const getInputWithPurpose = (purpose) =>
    {
      for (const i of this.patchConnection.inputEndpoints)
        if (i.purpose === purpose)
          return i.endpointID;
    }

    if (getInputWithPurpose ("audio in"))
      this.connect (this.patchConnection.audioNode, 0, 0);

    this._wamNode = paramManagerNode;
    this._output = this.patchConnection.audioNode;

    const midiEndpointID = getInputWithPurpose ("midi in");

    if (midiEndpointID)
    {
      this._wamNode.addEventListener('wam-midi', ({ detail }) =>
      {
//        console.log(detail);
        this.patchConnection.sendMIDIInputEvent (midiEndpointID, detail.data.bytes[2] | (detail.data.bytes[1] << 8) | (detail.data.bytes[0] << 16));
      });
    }
  }
}

export default class CmajModule extends WebAudioModule
{
  async createAudioNode (options)
  {
    const node = new CmajNode(this.audioContext);

    this.patchConnection = await patch.createAudioWorkletNodePatchConnection (this.audioContext, "cmaj-processor");
        const paramMgrNode = await ParamMgrFactory.create(this, {});

    node.setup (this.patchConnection, paramMgrNode);

    return node;
  }

  async initialize (state)
  {
    const hasPurpose = (endpoints, purpose) =>
    {
      for (const i of endpoints)
        if (i.purpose === purpose)
          return true;

      return false;
    }

    const descriptor =
    {
      identifier:     patch.manifest.ID,
      name:           patch.manifest.name,
      description:    patch.manifest.description,
      version:        patch.manifest.version,
      vendor:         patch.manifest.manufacturer,
      isInstrument:   patch.manifest.isInstrument,
      thumbnail:      patch.manifest.icon,
      website:        patch.manifest.URL,
      hasMidiInput:   hasPurpose (patch.getInputEndpoints(), "midi in"),
      hasAudioInput:  hasPurpose (patch.getInputEndpoints(), "audio in"),
      hasMidiOutput:  hasPurpose (patch.getOutputEndpoints(), "midi out"),
      hasAudioOutput: hasPurpose (patch.getOutputEndpoints(), "audio out"),
    };

    Object.assign (this.descriptor, descriptor);
    return super.initialize(state);
  }

  createGui()
  {
    return createPatchViewHolder (this.patchConnection);
  }

  destroyGui()
  {
  }
}
)",
    "PATCH_MODULE_FILE", patchModuleFile,
    "PATCH_NAME", manifest.name,
    "PATCH_DESC", patchDesc,
    "PATCH_LINK", patchLink));

auto index_html = choc::text::trimStart (choc::text::replace (R"(
<html>
<head>
  <script type="module" src="https://mainline.i3s.unice.fr/wam_wc/wam-host/wamHost.js"> </script>
  <script type="module" src="https://mainline.i3s.unice.fr/wam_wc/wam-plugin/wamPlugin.js"></script>
</head>

<wam-host id="host">
</wam-host>

<body>
  <script>
    const plugin = document.createElement ("wam-plugin");
    plugin.id = "wamRemote";
    plugin.src = window.location.href + "/../index.js";

    document.getElementById ("host").appendChild (plugin);
  </script>
</body>
</html>
)",
    "PATCH_MODULE_FILE", patchModuleFile,
    "PATCH_NAME", manifest.name,
    "PATCH_DESC", patchDesc,
    "PATCH_LINK", patchLink));


    GeneratedFiles generatedFiles;

    generatedFiles.addFile (patchModuleFile, generateJavascriptWorklet (patch, loadParams, engineOptions));
    generatedFiles.addFile ("index.html", index_html);
    generatedFiles.addFile ("index.js", index_js);

    generatedFiles.addPatchResources (manifest);
    generatedFiles.addWamResources();
    generatedFiles.sort();

    return generatedFiles;
}


}
