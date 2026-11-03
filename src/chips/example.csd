<CsoundSynthesizer>
<CsOptions>
-+rtaudio=wasapi -odac --opcode-lib=../../build/src/chips/Release/chips.dll
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1


; A single gated voice, envelope triggered by a k-rate gate.
instr 2
  kGate oscili 1, 10
  aL sid6581 randh:k(1000, 10), 4, 2048, kGate, \
               0,   0, 0,    0,     \
               0,   0, 0,    0,     \
               0, 0, 0, 0
  outs aL * 0.5, aL * 0.5
endin
</CsInstruments>
<CsScore>
i 2 0 10
e
</CsScore>
</CsoundSynthesizer>
