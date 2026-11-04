<CsoundSynthesizer>
<CsOptions>
-+rtaudio=wasapi -odac4
--opcode-lib=../../build/src/chips/Debug/chips.dll
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1

; BYTE RUNNER - a two-minute SID-style arcade track in A minor.
; Sixteen-step patterns are arranged in eight-bar sections.

; Two-bar lead phrases. Zeroes are rests.
lead_a@global:i = ftgen(0, 0, 32, -2, \
69, 0, 69, 72, 76, 0, 74, 72, 74, 0, 74, 71, 74, 0, 69, 0, \
69, 0, 69, 72, 76, 0, 81, 79, 76, 74, 72, 74, 76, 72, 69, 0)

lead_b@global:i = ftgen(0, 0, 32, -2, \
76, 0, 74, 72, 69, 0, 72, 74, 76, 0, 79, 76, 74, 0, 72, 0, \
72, 0, 74, 76, 79, 0, 81, 79, 76, 74, 72, 69, 72, 74, 69, 0)

lead_hook@global:i = ftgen(0, 0, 32, -2, \
81, 0, 81, 79, 76, 0, 79, 81, 84, 0, 81, 79, 76, 74, 72, 0, \
81, 0, 81, 79, 76, 0, 79, 81, 86, 84, 81, 79, 76, 72, 69, 0)

; Eighth-note bass rhythm in semitones from each bar's root (-1 is a rest).
bass_pattern@global:i = ftgen(0, 0, 16, -2, \
0, -1, 0, -1, 7, -1, 0, -1, 0, -1, 12, -1, 7, -1, 12, -1)

; Arpeggio intervals from the current chord root.
arp_pattern@global:i = ftgen(0, 0, 16, -2, \
12, 19, 24, 19, 16, 19, 24, 19, 12, 19, 24, 28, 24, 19, 16, 19)

; Drum hits are one eighth-note or shorter; rests let each SID envelope fall.
kick_pattern@global:i = ftgen(0, 0, 16, -2, \
1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0)
snare_pattern@global:i = ftgen(0, 0, 16, -2, \
0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0)
hat_pattern@global:i = ftgen(0, 0, 16, -2, \
1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0)

; Am - F - C - G, one root per bar.
chord_roots@global:i = ftgen(0, 0, 4, -2, 45, 41, 48, 43)

instr 1

    tempo:k = 140
    trig:k = metro(tempo / 15)

    step:k init 15
    bar:k init -1
    section:k init 0
    lead_note:k init 0
    bass_note:k init 0
    arp_note:k init 0
    lead_gate:k init 0
    bass_gate:k init 0
    arp_gate:k init 0
    retrigger_lead:k init 0
    retrigger_bass:k init 0
    retrigger_arp:k init 0
    kick_gate:k init 0
    snare_gate:k init 0
    snare_remaining:k init 0
    hat_gate:k init 0

    if trig == 1 then
        step = (step + 1) % 16
        if step == 0 then
            bar = bar + 1
        endif
        section = int(bar / 8)

        phrase_index:k = ((bar % 2) * 16) + step
        chord:k = table(bar % 4, chord_roots)
        bass_offset:k = table(step, bass_pattern)
        bass_pitch:k = (bass_offset >= 0 ? chord + bass_offset : 0)
        arp_offset:k = table(step, arp_pattern)

        lead_note = 0
        bass_note = 0
        arp_note = 0

        if section == 1 then
            lead_note = table(phrase_index, lead_a)
            bass_note = bass_pitch
            arp_note = chord + arp_offset
        elseif section == 2 then
            lead_note = table(phrase_index, lead_hook)
            bass_note = bass_pitch
            arp_note = chord + arp_offset
        elseif section == 3 then
            lead_note = table(phrase_index, lead_b)
            bass_note = bass_pitch
            arp_note = chord + arp_offset
        elseif section == 4 then
            if bar >= 36 then
                bass_note = bass_pitch
            endif
            arp_note = chord + arp_offset
        elseif section == 5 then
            lead_note = table(phrase_index, lead_hook)
            bass_note = bass_pitch
            arp_note = chord + arp_offset
        elseif section == 6 then
            lead_note = table(phrase_index, lead_hook)
            lead_note = (lead_note > 0 ? lead_note + 12 : 0)
            bass_note = bass_pitch
            arp_note = chord + arp_offset
        elseif section == 7 then
            if bar < 60 then
                lead_note = table(phrase_index, lead_a)
                bass_note = bass_pitch
                arp_note = chord + arp_offset
            else
                arp_note = chord + arp_offset
            endif
        elseif section == 0 then
            arp_note = chord + arp_offset
            if bar >= 2 then
                bass_note = bass_pitch
            endif
        endif

        ; Force a brief gate low at every step so repeated notes re-trigger.
        lead_gate = 0
        bass_gate = 0
        arp_gate = 0
        retrigger_lead = 1
        kick_gate = table(step, kick_pattern)
        snare_gate = table(step, snare_pattern)
        hat_gate = table(step, hat_pattern)

        ; Intro and breakdown thin out the kit; the outro drops it entirely.
        if section == 0 then
            kick_gate = 0
            snare_gate = 0
            hat_gate = (bar >= 6 ? hat_gate : 0)
        elseif section == 4 then
            kick_gate = 0
            snare_gate = 0
            hat_gate = (bar >= 36 ? hat_gate : 0)
        elseif section == 7 then
            kick_gate = (bar < 60 ? kick_gate : 0)
            snare_gate = (bar < 60 ? snare_gate : 0)
            hat_gate = (bar < 60 ? hat_gate : 0)
        endif

        ; Shorten snare gate independently of the 107 ms sequencer step.
        if (bar % 4) == 3 && step == 14 && section >= 1 && section <= 6 then
            snare_gate = 1
        endif
        snare_remaining = (snare_gate > 0 ? 0.02 : 0)
    elseif retrigger_lead == 1 then
        lead_gate = (lead_note > 0 ? 1 : 0)
        bass_gate = (bass_note > 0 ? 1 : 0)
        arp_gate = (arp_note > 0 ? 1 : 0)
        retrigger_lead = 0
    else
        if snare_remaining > 0 then
            snare_remaining = snare_remaining - (ksmps / sr)
            snare_gate = (snare_remaining > 0 ? 1 : 0)
        endif
    endif

    lead_freq:k = (lead_note > 0 ? cpsmidinn(lead_note) : 0)
    bass_freq:k = (bass_note > 0 ? cpsmidinn(bass_note) : 0)
    arp_freq:k = (arp_note > 0 ? cpsmidinn(arp_note) : 0)

    ; Build the envelopes for each voice
    lead_env:Sid6581ADSR = sid6581adsr(0.004, 0.08, 0.35, 0.025)
    bass_env:Sid6581ADSR = sid6581adsr(0.003, 0.08, 0.55, 0.025)
    arp_env:Sid6581ADSR = sid6581adsr(0.002, 0.06, 0.22, 0.02)
    music_cutoff:k = (section == 5 || section == 6 ? 1500 : 850 + (section * 90))

    ; Build the voices for each instrument
    lead_voice:Sid6581Voice = sid6581voice(lead_freq, 4, 1250)
    bass_voice:Sid6581Voice = sid6581voice(bass_freq, 2, 2048)
    arp_voice:Sid6581Voice = sid6581voice(arp_freq, 1, 2048)

    ; Build the music signal
    music:a = sid6581(  lead_voice, lead_gate, lead_env, bass_voice, \
    bass_gate, bass_env, arp_voice, arp_gate, arp_env,\
    music_cutoff, 7, 1, 7)

    ; Build the drum envelopes and voices
    kick_env:Sid6581ADSR = sid6581adsr(0.001, 0.09, 0.0, 0.025)
    snare_env:Sid6581ADSR = sid6581adsr(0.001, 0.0002, 0.0, 0.001)
    hat_env:Sid6581ADSR = sid6581adsr(0.001, 0.035, 0.0, 0.012)

    kick_voice:Sid6581Voice = sid6581voice(72, 1, 2048)
    snare_voice:Sid6581Voice = sid6581voice(1200, 8, 2048)
    hat_voice:Sid6581Voice = sid6581voice(2200, 8, 2048)

    ; Build the drum signal
    drums:a = sid6581(kick_voice, kick_gate, kick_env, \
    snare_voice, snare_gate, snare_env, \
    hat_voice, hat_gate, hat_env, \
    2047, 0, 0, 0)

    ; Apply fade and mix the music and drum signals
    fade:k = linseg(0, 1, 1, p3 - 3, 1, 1, 0)
    mix:a = (music * 0.68) + (drums * 0.38)
    outs(mix * fade, mix * fade)
endin

</CsInstruments>
<CsScore>
; 64 bars at 140 BPM: eight sections, each eight bars long.
i 1 0 109.72
e
</CsScore>
</CsoundSynthesizer>
