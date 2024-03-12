# The Cmajor Language

Cmajor is a programming language for writing fast, portable audio software.

You've heard of C, C++, C#, objective-C... well, C*major* is a C-family language designed specifically for writing DSP signal processing code.

## Project goals

We're aiming to improve on the current status-quo for audio development in quite a few ways:

- To match (and often beat) the performance of traditional C/C++
- To make the same code portable across diverse processor architectures (CPU, DSP, GPU, TPU etc)
- To offer enough power and flexibility to satisfy professional audio tech industry users
- To speed-up commercial product cycles by enabling sound-designers to be more independent from the instrument platforms
- To attract students and beginners by being vastly easier to learn than C/C++

Our main documentation site is at [cmajor.dev](https://cmajor.dev).

The [Cmajor VScode extension](https://marketplace.visualstudio.com/items?itemName=CmajorSoftware.cmajor-tools) offers a one-click install process to get you up-and-running, or see the [Quick Start Guide](https://cmajor.dev/docs/GettingStarted) for other options, like how to use the command-line tools.

If you want to learn about the nitty-gritty of the Cmajor language, the [language guide](https://cmajor.dev/docs/LanguageReference) offers a deep dive. To see some examples of the code, try the [examples](./examples/patches) folder.

## How to build

To build Cmajor requires a host with suitable compilers, and with support for Cmake. For MacOS, a recent XCode is required, whilst Windows requires VS2019. Linux builds have been successfully built using both clang and gcc v8 and above.

When cloning the Cmajor repository, you need to ensure you have also pulled the submodules, as there are a number for third party libraries and some pre-built LLVM libraries.

### Building on MacOS

To build on MacOS, from the top level directory in a shell execute:

```
> cmake -Bbuild -GXcode .
```

You can then open the XCode project in the build directory to build the tooling

### Building on Windows

On Windows, use cmake to build a suitable project to be opened with visual studio. From the top level directory in a shell execute:

```
> cmake -Bbuild -G"Visual Studio 16 2019" .
```

The resulting visual studio project can be opened and the tooling built. On Windows/arm64, building the plugin is not supported for now, and this can be disabled by specifying `-DBUILD_PLUGIN=OFF` when running cmake

Only `Release` and `RelWithDebugInfo` builds of the windows project are currently supported, as the pre-built LLVM libraries use the release runtime. If you find you need a Debug build, the simplest way is to re-build the LLVM library as a debug build. Be warned, this will take some time and the resulting LLVM libraries are rather large. The LLVM build script can be run to achieve this specifying `--build-type=Debug`

### Building on Linux

On linux platforms, simply running cmake with your generator of choice will produce a suitable build. We find ninja works well for this project:

```
> cmake -Bbuild -GNinja -DBUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release .
> cd build
> ninja
```

### Faust => Cmajor

The Faust => Cmajor hackish integration allows to use Faust .dsp files in a Cmajor patch, to be compiled on the fly in .cmajor files using the Faust to Cmajor backend. Look at `examples/patches/FaustCmajor` for an example.

Added files:

- `/3rdParty/faust/libfaust.a` (statically compiled Faust 2.72.7 only containing the Cmajor backend)
- `include/cmajor/helpers/export.h` and `include/cmajor/helpers/libfaust-box-c.h` header files
- `examples/patches/FaustCmajor` with a Faust/Cmajor hybrid project containing `addSynth.dsp` and `stereoEcho.dsp` Faust files and `test.cmajor` Cmajor file
- `examples/patches/FaustCmajorPoly` with a Faust/Cmajor hybrid project containing the `voice.dsp` Faust file that describes the voice code to be compiled in Cmajor, then wrapped with Cmajor written polyphony + MIDI code in the `poly-dsp.cmajor` Cmajor file.
- `examples/patches/FaustCmajorPolyEffect` with a Faust/Cmajor hybrid project containing the `voice.dsp` Faust file that describes the voice code, `effect.dsp` for the global effect, to be compiled in Cmajor then wrapped with Cmajor written polyphony + MIDI code in `poly-dsp-effect.cmajor` Cmajor file.

The compiled `cmaj` binary can be copied in `~/.vscode/extensions`, in a sub-directory called `cmajorsoftware.cmajor-tools-xxxxx`, to be used with VSCode.

#### Using Faust code in Cmajor

Coding conventions have to be known when using Faust code in a Cmajor program:

- each Faust DSP file will be compiled as a `processor` inside the `namespace faust {...}`, which name will be the DSP filename itself. So a `foo.dsp` file will be compiled as a `processor foo {...}` block of code.
- labels used in button, sliders, nentries... will be compiled as input events, to be used in the graph. So a `hslider("freq", 200, 200, 1000, 0.1)` will be converted as a `event freq ....` line of Cmajor code. 
- audio inputs/outputs in the Faust processor are generated as `input0/input1...inputN` and `output0/output1...outputN`.

Here is a block of connection code combining an `addSynth` and `stereoEcho` processors compiled from Faust DSP `addSynth.dsp` and `stereoEcho.dsp` files with a Cmajor coded `ClassicRingtone` processor.

File addSynth.dsp:

```
import("stdfaust.lib");

vol = hslider("volume [unit:dB]", -20, -96, 0, 0.1) : ba.db2linear : si.smoo;
freq = hslider("freq [unit:Hz]", 500, 100, 2000, 1);

process = vgroup("addSynth", (os.osc(freq) + 0.5*os.osc(2.*freq) + 0.25*os.osc(3.*freq)) * vol);
```

File stereoEcho.dsp:

```
import("stdfaust.lib");
    
gain = hslider("gain", 0.5, 0, 1, 0.01);
feedback = hslider("feedback", 0.8, 0, 1, 0.01);
 
echo(del_sec, fb, g) = + ~ de.delay(50000, del_samples) * fb * g
with {
    del_samples = del_sec*ma.SR;
};

process = echo(1.6, 0.6, 0.7), echo(0.7, feedback, gain);
```

Cmajor connection graph:

```
connection 
{
    // Connect to Faust addSynth
    volume -> faust::addSynth.volume;
    freq -> faust::addSynth.freq;

    // Connect to Faust stereoEcho
    feedback -> faust::stereoEcho.feedback;
    gain -> faust::stereoEcho.gain;

    faust::addSynth.output0 -> faust::stereoEcho.input0;
    ClassicRingtone.out -> faust::stereoEcho.input1;

    faust::stereoEcho.output0 -> audioOut0;
    faust::stereoEcho.output1 -> audioOut1;
}
```
----

All content is copyright 2023 [Cmajor Software Ltd](https://cmajor.dev) unless marked otherwise.
