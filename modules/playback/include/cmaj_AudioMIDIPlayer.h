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

#include <mutex>
#include "../../compiler/include/cmaj_ErrorHandling.h"
#include "choc/audio/choc_AudioMIDIBlockDispatcher.h"

namespace cmaj::audio_utils
{

//==============================================================================
struct AudioDeviceOptions
{
    uint32_t sampleRate = 0;
    uint32_t blockSize = 0;
    uint32_t inputChannelCount = 2;
    uint32_t outputChannelCount = 2;
    std::string audioAPI, inputDeviceName, outputDeviceName;
};

//==============================================================================
struct AvailableAudioDevices
{
    std::vector<std::string> availableAudioAPIs,
                             availableInputDevices,
                             availableOutputDevices;

    std::vector<int32_t> sampleRates, blockSizes;
};

//==============================================================================
/**
 *  A callback which can be attached to an AudioMIDIPlayer, to receive callbacks
 *  which process chunks of synchronised audio and MIDI data.
 */
struct AudioMIDICallback
{
    virtual ~AudioMIDICallback() = default;

    /// This will be invoked (on an unspecified thread) if the sample rate
    /// of the device changes while this callback is attached.
    virtual void sampleRateChanged (double newRate) = 0;

    /// This will be called once before a set of calls to processSubBlock() are
    /// made, to allow the client to do any setup work that's needed.
    virtual void startBlock() = 0;

    /// After a call to startBlock(), one or more calls to processSubBlock() will be
    /// made for chunks of the main block, providing any MIDI messages that should be
    /// handled at the start of that particular subsection of the block.
    /// If `replaceOutput` is true, the caller must overwrite any data in the audio
    /// output buffer. If it is false, the caller must add its output to any existing
    /// data in that buffer.
    virtual void processSubBlock (const choc::audio::AudioMIDIBlockDispatcher::Block&,
                                  bool replaceOutput) = 0;

    /// After enough calls to processSubBlock() have been made to process the whole
    /// block, a call to endBlock() allows the client to do any clean-up work necessary.
    virtual void endBlock() = 0;
};

//==============================================================================
/**
 *  A multi-client device abstraction that provides unified callbacks for processing
 *  blocks of audio and MIDI input/output.
 *
*/
struct AudioMIDIPlayer
{
    AudioMIDIPlayer (const AudioDeviceOptions&);
    virtual ~AudioMIDIPlayer() = default;

    virtual AvailableAudioDevices getAvailableDevices() = 0;

    /// Attaches a callback to this device.
    void addCallback (AudioMIDICallback&);
    /// Removes a previously-attached callback to this device.
    void removeCallback (AudioMIDICallback&);

    /// The options that this device was created with.
    AudioDeviceOptions options;

    /// Provide this callback if you want to know when the options
    /// are changed (e.g. the sample rate). No guarantees about which
    /// thread may call it.
    std::function<void()> deviceOptionsChanged;


protected:
    //==============================================================================
    std::vector<AudioMIDICallback*> callbacks;
    std::mutex callbackLock;
    choc::audio::AudioMIDIBlockDispatcher dispatcher;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void handleOutgoingMidiMessage (const void* data, uint32_t length) = 0;

    void updateSampleRate (uint32_t);
    void addIncomingMIDIEvent (const void*, uint32_t);
    void process (choc::buffer::ChannelArrayView<const float> input,
                  choc::buffer::ChannelArrayView<float> output, bool replaceOutput);
};




//==============================================================================
//        _        _           _  _
//     __| |  ___ | |_   __ _ (_)| | ___
//    / _` | / _ \| __| / _` || || |/ __|
//   | (_| ||  __/| |_ | (_| || || |\__ \ _  _  _
//    \__,_| \___| \__| \__,_||_||_||___/(_)(_)(_)
//
//   Code beyond this point is implementation detail...
//
//==============================================================================

inline AudioMIDIPlayer::AudioMIDIPlayer (const AudioDeviceOptions& o) : options (o)
{
    dispatcher.setMidiOutputCallback ([this] (uint32_t, choc::midi::ShortMessage m)
    {
        if (auto len = m.length())
            handleOutgoingMidiMessage (m.data, len);
    });

    dispatcher.reset (options.sampleRate);
}

inline void AudioMIDIPlayer::addCallback (AudioMIDICallback& c)
{
    bool needToStart = false;

    {
        const std::scoped_lock lock (callbackLock);

        if (std::find (callbacks.begin(), callbacks.end(), std::addressof (c)) != callbacks.end())
            return;

        needToStart = callbacks.empty();
        callbacks.push_back (std::addressof (c));

        if (options.sampleRate != 0)
            c.sampleRateChanged (options.sampleRate);
    }

    if (needToStart)
        start();
}

inline void AudioMIDIPlayer::removeCallback (AudioMIDICallback& c)
{
    bool needToStop = false;

    {
        const std::scoped_lock lock (callbackLock);

        if (auto i = std::find (callbacks.begin(), callbacks.end(), std::addressof (c)); i != callbacks.end())
            callbacks.erase (i);

        needToStop = callbacks.empty();
    }

    if (needToStop)
        stop();
}

inline void AudioMIDIPlayer::updateSampleRate (uint32_t newRate)
{
    if (options.sampleRate != newRate)
    {
        options.sampleRate = newRate;

        if (newRate != 0)
        {
            const std::scoped_lock lock (callbackLock);

            for (auto c : callbacks)
                c->sampleRateChanged (newRate);

            dispatcher.reset (options.sampleRate);
        }

        if (deviceOptionsChanged)
            deviceOptionsChanged();
    }
}

inline void AudioMIDIPlayer::addIncomingMIDIEvent (const void* data, uint32_t size)
{
    const std::scoped_lock lock (callbackLock);
    dispatcher.addMIDIEvent (data, size);
}

inline void AudioMIDIPlayer::process (choc::buffer::ChannelArrayView<const float> input,
                                      choc::buffer::ChannelArrayView<float> output,
                                      bool replaceOutput)
{
    const std::scoped_lock lock (callbackLock);

    if (callbacks.empty())
    {
        if (replaceOutput)
            output.clear();

        return;
    }

    for (auto c : callbacks)
        c->startBlock();

    dispatcher.setAudioBuffers (input, output);

    dispatcher.processInChunks ([this, replaceOutput]
                                (const choc::audio::AudioMIDIBlockDispatcher::Block& block)
    {
        bool replace = replaceOutput;

        for (auto c : callbacks)
        {
            c->processSubBlock (block, replace);
            replace = false;
        }
    });

    for (auto c : callbacks)
        c->endBlock();
}

}
