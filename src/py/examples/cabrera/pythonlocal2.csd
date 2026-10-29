<CsoundSynthesizer>
<CsOptions>
  -odac -m0
</CsOptions>
<CsInstruments>

pyinit

gkprint init 0

instr 1
;assign 4th p-field to local python variable "value"
pylassigni "value", p4
ktrig changed gkprint

; If gkprint has changed (i.e. instr 2 was triggered)
; print the value of the local python variable "value"
if (ktrig == 1) then
  kvalue pyleval "value"
  printk 0.5, kvalue
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
