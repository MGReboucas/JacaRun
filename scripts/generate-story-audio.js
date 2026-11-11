// Original synthesized cinematic score. No recordings, samples or external libraries.
// Cues follow StoryIntro.h: 0, 2.4, 5.0, 6.9, 9.5, 12.1; end at 15.2 seconds.
const fs = require('fs');
const path = require('path');
const rate = 22050, duration = 15.2;
const mix = new Float64Array(Math.round(rate * duration));
let seed = 9417;
const random = () => ((seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0) / 4294967296 * 2 - 1);
function event(start, length, sample) {
    const offset = Math.round(start * rate), count = Math.round(length * rate);
    for (let i = 0; i < count && offset + i < mix.length; i++) {
        const t = i / rate, edge = Math.min(1, t / .008, (count-i) / (rate*.025));
        mix[offset+i] += sample(t, t/length) * edge;
    }
}
function note(at, midi, length=.24, gain=.09, decay=9) {
    const hz = 440 * 2 ** ((midi-69)/12);
    event(at,length,(t) => gain * (Math.sin(2*Math.PI*hz*t) + .22*Math.sin(4*Math.PI*hz*t)) * Math.exp(-decay*t));
}
function splash(at, gain=.10, length=.42) {
    let low=0;
    event(at,length,(t) => { low=low*.78+random()*.22; return gain*low*Math.exp(-5*t); });
}
function drum(at, gain=.12) {
    event(at,.18,(t) => gain*Math.sin(2*Math.PI*(60*t+2*(1-Math.exp(-t*18))))*Math.exp(-t*22));
}
// Quiet creek, bubbles and a warm three-note motif for Bento and Dourado.
let water=0;
event(0,duration,(t) => { water=water*.97+random()*.03; return water*.055; });
for(const [at,midi] of [[.12,60],[.48,64],[.85,67],[1.25,72],[1.65,67]]) note(at,midi,.55,.08,5);
for(let at=.25;at<4.8;at+=.73) {
    event(at,.10,(t,u) => .035*Math.sin(2*Math.PI*(650*t+90*t*t))*Math.sin(Math.PI*u));
}
// Sneaky pizzicato, with a deliberate breath before the fish strikes back.
for(const [at,midi] of [[2.5,57],[2.85,60],[3.2,63],[3.55,64],[4,67],[4.35,71]]) note(at,midi,.24,.10,13);
event(4.62,.30,(t,u) => .06*Math.sin(2*Math.PI*(400*t+250*t*t))*Math.sin(Math.PI*u));
// A dry comic snap and falling whistle, not a violent impact.
event(5.15,.065,(t,u) => random()*.18*(1-u));
event(5.20,.48,(t,u) => .10*Math.sin(2*Math.PI*(700*t-390*t*t))*Math.sin(Math.PI*u));
splash(5.22,.16);
// Bento's dramatic low musical bellow and short orchestral chord.
for(const midi of [36,43,48,51]) note(7.0,midi,1.1,.08,3);
event(7.05,.7,(t,u) => .10*(Math.sin(2*Math.PI*75*t)+.24*Math.sin(2*Math.PI*151*t)) * Math.sin(Math.PI*u));
splash(7.05,.22,.65);
// Awkward silence under the excuse, punctuated by two soft cricket chirps.
for(const at of [10.1,10.42]) event(at,.13,(t,u) => .025*Math.sin(2*Math.PI*2700*t)*Math.pow(Math.sin(Math.PI*u),2));
note(11.2,55,.28,.06,12); note(11.48,54,.3,.06,12);
// Wind-up, splash, then a quick chase flourish.
event(12.3,.42,(t,u) => .08*Math.sin(2*Math.PI*(220*t+500*t*t))*Math.sin(Math.PI*u));
splash(12.65,.25,.55);
for(let i=0;i<10;i++) {
    const at=12.7+i*.21;
    drum(at,i%2?.07:.12);
    note(at,[69,72,76,79,81,79,76,74,72,69][i],.25,.10,9);
}
let peak=0; for(const v of mix) peak=Math.max(peak,Math.abs(v));
const gain=.72/Math.max(.1,peak);
const out=Buffer.alloc(44+mix.length*2);
out.write('RIFF',0); out.writeUInt32LE(out.length-8,4); out.write('WAVEfmt ',8);
out.writeUInt32LE(16,16); out.writeUInt16LE(1,20); out.writeUInt16LE(1,22);
out.writeUInt32LE(rate,24); out.writeUInt32LE(rate*2,28); out.writeUInt16LE(2,32);
out.writeUInt16LE(16,34); out.write('data',36); out.writeUInt32LE(mix.length*2,40);
for(let i=0;i<mix.length;i++) {
    const tail=Math.min(1,(mix.length-i)/(rate*.18));
    out.writeInt16LE(Math.round(Math.max(-1,Math.min(1,mix[i]*gain*tail))*32767),44+i*2);
}
const destination=path.join(__dirname,'../visual/Content/audio/story-score.wav');
fs.writeFileSync(destination,out);
console.log(`story-score.wav: ${duration}s, PCM mono ${rate}Hz, peak ${(peak*gain).toFixed(3)}`);
