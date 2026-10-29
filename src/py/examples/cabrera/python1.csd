<CsoundSynthesizer>
<CsOptions>
  -odac -m0
</CsOptions>
<CsInstruments>

sr=44100
ksmps=128
nchnls=2

pyinit ;Start python interpreter

pyruni {{
a = 2
b = 3
print("a + b = " + str(a+b))
}} ;Execute a python script on the header

instr 1
pyruni {{a = 6
b = 5
print("a + b = " + str(a+b))}} 
endin


instr 2
pyruni {{print("a + b = " + str(a+b))}}
endin


</CsInstruments>
<CsScore>

i 1 0 0.1
i 2 1 0.1
</CsScore>
</CsoundSynthesizer> 