sr=44100
kr=4410
ksmps=10
nchnls=2

giSinusoid      ftgen           0,      0, 8192, 10,    1

pyinit

pyruni  {{
import ctcsound
import sys
from random import random
p = _CSOUND_
print("\\n  --> Python version number: {}\\n".format(sys.version_info[0]))
cs = ctcsound.Csound(pointer_=p)
for i in range(800):
        cs.scoreEvent('i', [1, i * .2,     0.05, 6.8 + random() * 3,   70.0])
        cs.scoreEvent('i', [1, i * .2,     0.05, 8.8 + random() * 3,   70.0])
}}

instr 1

        iDuration       =       p3
        iFrequency      =       cpsoct(p4)
        iAmplitude      =       ampdb(p5)

        aAmplitude      linseg  iAmplitude, iDuration - 0.01, iAmplitude, 0.002 ,0, 1,0
        aOutput         oscili  aAmplitude, iFrequency, giSinusoid

                        outs    aOutput, aOutput

endin
