import("stdfaust.lib");
    
gain = hslider("gain", 0.5, 0, 1, 0.01);
feedback = hslider("feedback", 0.8, 0, 1, 0.01);
 
echo(del_sec, fb, g) = + ~ de.delay(50000, del_samples) * fb * g
with {
    del_samples = del_sec*ma.SR;
};

process = echo(1.6, 0.6, 0.7), echo(0.7, feedback, gain);
