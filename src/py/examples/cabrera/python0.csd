<CsoundSynthesizer>
<CsOptions>
  -odac -m0
</CsOptions>
<CsInstruments>

sr=44100
ksmps=128
nchnls=2

pyinit ;Start python interpreter

pyruni "print(44100)"

instr 1
endin


</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer> 