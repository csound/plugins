<CsoundSynthesizer>
<CsLicense>

TEST JACKO OPCODES
Michael Gogins

This csd tests the Jacko opcodes.

There is a segmentation fault at the end of the performance. This does happen 
in the Jacko plugin, but it does not affect the performance.

Csound not only writes a soundfile using -o, but also sends realtime audio and 
MIDI to other Jack clients using Jack connections.

Before running this piece, in a terminal execute:

zynaddsubfx --input JACK --output JACK --auto-connect --sample-rate 48000 --buffer-size 128


</CsLicense>
<CsOptions>
;-o jacko.wav
</CsOptions>
<CsInstruments>
; Sampling rate must be the same as for Jack.
sr=48000
; ksmps must be the same as for Jack's frames/period.
ksmps=128
nchnls=2
0dbfs=40000

ga_audio_out init 0

; Initialize the Jack connections.
JackoInit "default", "csound6"                    
prints "Initial ports and connections:\n"
JackoInfo
JackoAudioOutConnect "audio_out_left", "system:playback_1"
JackoAudioOutConnect "audio_out_right", "system:playback_2"
JackoMidiInConnect "system:midi_capture_1", "midiin"
JackoMidiOutConnect "midiout", "zynaddsubfx:midi_input"
prints "Final ports and connections:\n"
JackoInfo
JackoOn                                                                        

instr 1
print p1, p2, p3, p4, p5
i_frequency = cpsmidinn(p4)
i_amplitude = ampdb(p5)
print i_frequency, i_amplitude
a_out oscil i_amplitude, i_frequency
ga_audio_out += a_out;
endin

; Sends notes to an external MIDI synthesizer.
instr 801
print p1, p2, p3, p4, p5
JackoNoteOut "midiout", 0, p4, p5		   
prints "Sent note to midiout.\n"
endin

instr 900 
print p1, p2, p3
outs ga_audio_out, ga_audio_out
JackoAudioOut "audio_out_left",  ga_audio_out
JackoAudioOut "audio_out_right", ga_audio_out
ga_audio_out = 0
endin

instr 1000
print p1, p2, p3
; Uncomment the following two lines to test if Jacko hangs Csound.
event "e", 0, 0, 0.1
prints "Ending Csound performance with 'e' event\n"
endin

</CsInstruments>
<CsScore>
f 0 60
i1     5  3 63 60
i801  10  3 69 60	
i1	  15  3 72 60	
i900   0 -1
i1000 20  1
e
</CsScore>

</CsoundSynthesizer>