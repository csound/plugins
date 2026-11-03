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
; The ChipADSR value is k-rate and latched on the gate's rising edge.
instr 2
  kGate oscili 1, 10
  env:ChipADSR chipadsr 0.01, 0.1, 0.7, 0.3
  aL sid6581 randh:k(1000, 10), 4, 2048, kGate, env, \
               0,   0, 0,    0,     env, \
               0,   0, 0,    0,     env, \
               0, 0, 0, 0
  outs aL * 0.5, aL * 0.5
endin
</CsInstruments>
<CsScore>
i 2 0 10
e
</CsScore>
</CsoundSynthesizer>
