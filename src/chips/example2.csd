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

; =====================================================================
; MULTIPLE TABLE VARIATIONS (Double Dragon Style in A Minor)
; =====================================================================

; --- LEAD VARIATIONS ---
lead1@global:i = ftgen(0, 0, 32, -2, \
69,  0, 69, 72, 76,  0, 74, 72, \
74,  0, 74, 71, 74,  0, 69,  0, \
69,  0, 69, 72, 76,  0, 81, 79, \
76, 74, 72, 74, 76, 72, 69,  0)

lead2@global:i = ftgen(0, 0, 32, -2, \
81,  0, 79, 76, 74,  0, 72, 69, \
72,  0, 74, 76, 79,  0, 81,  0, \
76,  0, 72, 69, 67,  0, 69, 72, \
74, 76, 79, 76, 74, 72, 69,  0)


; --- BASS VARIATIONS ---
bass1@global:i = ftgen(0, 0, 32, -2, \
45, 45, 57, 45, 43, 43, 55, 43, \
41, 41, 53, 41, 40, 40, 52, 40, \
45, 45, 57, 45, 43, 43, 55, 43, \
41, 41, 53, 41, 40, 43, 45, 48)

bass2@global:i = ftgen(0, 0, 32, -2, \
40, 40, 52, 40, 41, 41, 53, 41, \
43, 43, 55, 43, 45, 45, 57, 45, \
40, 40, 52, 40, 41, 41, 53, 41, \
43, 45, 48, 50, 45, 43, 41, 40)


; --- ARPEGGIO VARIATIONS ---
arp1@global:i = ftgen(0, 0, 32, -2, \
57, 60, 64, 69, 57, 60, 64, 69, \
55, 58, 62, 67, 55, 58, 62, 67, \
53, 57, 60, 65, 53, 57, 60, 65, \
52, 55, 59, 64, 52, 55, 57, 60)

arp2@global:i = ftgen(0, 0, 32, -2, \
60, 64, 67, 72, 60, 64, 67, 72, \
58, 62, 65, 70, 58, 62, 65, 70, \
57, 60, 64, 69, 57, 60, 64, 69, \
55, 59, 62, 67, 55, 57, 60, 64)


; =====================================================================
; INSTRUMENT: Action Sequencer with Global Tables & Snare Multiplexing
; =====================================================================
instr 1
    tempo:k = 150
    rate:k  = (tempo / 60) * 4
    trig:k  = metro(rate)

    index:k      init 0
    loop_count:k init 0
    section:k    init 0

    if trig == 1 then
        prev_index:k = index
        index = (index + 1) % 16

        if index < prev_index then
            loop_count = loop_count + 1

            if loop_count >= 8 then
                loop_count = 0
                section = (section + 1) % 2
            endif
        endif
    endif

    midi1:k = 0
    midi2:k = 0

    if section == 0 then
        midi1 = table(index, lead1)
        midi2 = table(index, bass1)
    else
        midi1 = table(index, lead2)
        midi2 = table(index, bass2)
    endif

    freq1:k = (midi1 > 0 ? cpsmidinn(midi1) : 0)
    freq2:k = (midi2 > 0 ? cpsmidinn(midi2) : 0)

    wave1:k = 4  ; Lead (Pulse)
    wave2:k = 2  ; Bass (Sawtooth)

    pw1:k = 1500
    pw2:k = 2048
    pw3:k = 2048 ; <-- Declared pulse-width for voice 3

    gate1:k = (midi1 > 0 ? 1 : 0)
    gate2:k = (midi2 > 0 ? 1 : 0)

    ; --- VOICE 1 & 2 ENVELOPES ---
    env1:ChipADSR = chipadsr(0.1, 0.08, 0.7, 0.12)
    env2:ChipADSR = chipadsr(0.005, 0.06, 0.6, 0.08)

    ; --- VOICE 3 MULTIPLEXING: ARPEGGIO VS. NOISE SNARE ---
    is_snare:k = ((index % 4) == 2 ? 1 : 0)

    freq3:k = 0
    wave3:k = 0
    gate3:k = 0
    env3:ChipADSR = chipadsr(0.004, 0.12, 0.4, 0.06)

    if is_snare == 1 then
        freq3 = 220
        wave3 = 8      ; SID Noise waveform for snare
        gate3 = 1
        env3.att = 0.001
        env3.dec = 0.06
        env3.sus = 0.0
        env3.rel = 0.02
    else
        midi3:k = (section == 0 ? table(index, arp1) : table(index, arp2))
        freq3 = (midi3 > 0 ? cpsmidinn(midi3) : 0)
        wave3 = 1      ; Triangle for support arp
        gate3 = (midi3 > 0 ? 1 : 0)
        env3.att = 0.004
        env3.dec = 0.12
        env3.sus = 0.4
        env3.rel = 0.06
    endif

    ; Dynamic envelope modulation per trigger
    if trig == 1 then
        if section == 1 then
            env1.att = 0.02
            env1.rel = 0.05
        else
            env1.att = 0.1
            env1.rel = 0.12
        endif
        env2.dec = 0.04 + (0.04 * (index % 4) / 4)
    endif

    cutoff:k = linseg(800, p3 * 0.25, 1800, p3 * 0.5, 1200, p3 * 0.25, 2047)
    res:k    = 8
    filter_mode:i = 1
    route_mask:i  = 7

    out:a = sid6581(freq1, wave1, pw1, gate1, env1, \
    freq2, wave2, pw2, gate2, env2, \
    freq3, wave3, pw3, gate3, env3, \
    cutoff, res, filter_mode, route_mask)

    outs(out * 0.35, out * 0.35)
endin

</CsInstruments>
<CsScore>
i 1 0 32
e
</CsScore>
</CsoundSynthesizer>