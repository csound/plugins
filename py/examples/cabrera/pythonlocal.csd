<CsoundSynthesizer>
<CsOptions>
  -odac
</CsOptions>
<CsInstruments>

gkprint init 0

instr 1
ivalue init p4
ktrig changed gkprint
if (ktrig == 1) then
  printk 0.5, ivalue
endif
endin

instr 2
gkprint init 1
endin

</CsInstruments>

<CsScore>
;          p4
i 1 0 5   100
i 1 1 5   200
i 1 2 5   300
i 1 3 5   400

i 2 3 1
</CsScore>
</CsoundSynthesizer>
