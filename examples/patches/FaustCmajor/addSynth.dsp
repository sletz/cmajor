import("stdfaust.lib");

vol = hslider("volume [unit:dB]", -20, -96, 0, 0.1) : ba.db2linear : si.smoo;
freq = hslider("freq [unit:Hz]", 500, 100, 2000, 1);

process = vgroup("addSynth", (os.osc(freq) + 0.5*os.osc(2.*freq) + 0.25*os.osc(3.*freq)) * vol);
