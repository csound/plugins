<CsoundSynthesizer>
<CsOptions>
--opcode-lib=./libclconv.dylib
</CsOptions>
<CsInstruments>

ksmps = 1
0dbfs = 1

gift ftgen 0, 0, 0, 1, "pianoc2.wav", 0,0,1
;tablew 1, 0, gift

instr 1
ipsize = 512
idev =  1; /* device number */
ain mpulse 1, 7
asig clconv ain, gift, ipsize, idev
  out(asig)

endin


</CsInstruments>
<CsScore>
i1 0 10
</CsScore>
</CsoundSynthesizer>

