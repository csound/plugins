<CsoundSynthesizer>
<CsOptions>
  -odac -m0
</CsOptions>
<CsInstruments>

sr=44100
kr=100
nchnls=2

pyinit

pyruni "a = 0"

instr 1
    pyrun "a = a + 1" 
endin


instr 2
    pyruni {{print("a = " + str(a))}}
endin


</CsInstruments>
<CsScore>

i 1 0 1    ;Adds to a for 1 second
i 2 1 0.1  ;Prints a
i 1 2 1    ;Adds to a for another second
i 2 3 0.1  ;Prints a

</CsScore>
</CsoundSynthesizer> 