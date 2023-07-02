<CsoundSynthesizer>
<CsOptions>
  -odac
</CsOptions>
<CsInstruments>

sr=44100
kr=100
nchnls=2

pyinit

pyruni {{
def average(a,b):
    ave = (a + b)/2
    return ave
}} ;Define function "average"

instr 1
iave   pycall1i "average", p4, p5
prints "a = %i\\n", iave
endin


</CsInstruments>
<CsScore>

i 1 0 1  100  200
i 1 1 1  1000 2000

</CsScore>
</CsoundSynthesizer> 