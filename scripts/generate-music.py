"""Original synthesized JacaRun music and bite. Standard library; no samples."""
from pathlib import Path
import math
import random
import struct
import wave

RATE = 22050
ROOT = Path(__file__).resolve().parents[1] / 'visual/Content/audio'
ROOT.mkdir(parents=True, exist_ok=True)
rng = random.Random(8425)

def save(name, samples):
    peak = max(abs(x) for x in samples) or 1
    gain = min(1, .85 / peak)
    with wave.open(str(ROOT / name), 'wb') as out:
        out.setparams((1, 2, RATE, 0, 'NONE', 'not compressed'))
        out.writeframes(b''.join(struct.pack('<h', round(x * gain * 32767)) for x in samples))
    print(name, 'seconds:', round(len(samples)/RATE, 3), 'peak:', round(peak*gain, 3))

beat = round(RATE * 60 / 112)
music = [0.] * (beat * 32)

def note(start, midi, duration, volume, kind='pluck'):
    frequency = 440 * 2 ** ((midi - 69) / 12)
    count = int(duration * beat)
    for i in range(count):
        t = i / RATE
        end = min(1., (count-i)/(RATE*.02))
        attack = min(1., i/(RATE*.004))
        if kind == 'pluck':
            value = (math.sin(2*math.pi*frequency*t) + .32*math.sin(4*math.pi*frequency*t) + .12*math.sin(6*math.pi*frequency*t)) * math.exp(-t*8)
        else:
            value = math.sin(2*math.pi*frequency*t) * math.exp(-t*5)
        music[(round(start*beat)+i) % len(music)] += value * volume * attack * end

def drum(start, kind, volume):
    for i in range(int(RATE*.16)):
        t = i/RATE
        if kind == 'low':
            value = math.sin(2*math.pi*(65*t+2*(1-math.exp(-t*22)))) * math.exp(-t*24)
        elif kind == 'triangle':
            value = sum(math.sin(2*math.pi*f*t) for f in (2300,3217,4871))/3 * math.exp(-t*34)
        else:
            value = (rng.random()*2-1) * math.exp(-t*65)
        # End every event smoothly; the buffer wraps for a seamless musical loop.
        music[(round(start*beat)+i) % len(music)] += value*volume*min(1,i/35)*min(1,(RATE*.16-i)/120)

# Eight bars, original melody over Am / G / F / E, with a light syncopated rhythm.
melodies = [
    [76,79,81,79,76,74], [72,76,79,76,74,72],
    [74,79,81,79,74,71], [71,74,79,74,72,71],
    [72,77,79,77,76,72], [69,72,77,76,72,69],
    [71,76,80,76,74,71], [71,74,76,74,71,68],
]
for bar in range(8):
    base = bar*4
    root = [45,45,43,43,41,41,40,40][bar]
    note(base,root,1.2,.20,'bass')
    note(base+1.5,root+7,.8,.13,'bass')
    note(base+2.5,root,1,.17,'bass')
    for offset, pitch in zip((0,.75,1.5,2,2.75,3.5),melodies[bar]):
        note(base+offset,pitch,.48,.13)
    for offset in (0,1.5,2,3.5): drum(base+offset,'low',.16)
    for offset in (.5,1.5,2.5,3.5): drum(base+offset,'triangle',.07)
    for step in range(8): drum(base+step*.5,'shaker',.035 if step%2 else .022)
save('mangue-loop.wav', music)

chomp = []
for i in range(int(RATE*.27)):
    t=i/RATE
    crunch=(rng.random()*2-1)*math.exp(-t*32)
    jaw=math.sin(2*math.pi*(220*t-180*t*t))*math.exp(-t*13)
    second=max(0,t-.10)
    crunch2=(rng.random()*2-1)*math.exp(-second*45) if t>.10 else 0
    fade=min(1,i/80)*min(1,(RATE*.27-i)/300)
    chomp.append((.28*crunch+.28*jaw+.13*crunch2)*fade)
save('chomp.wav',chomp)
