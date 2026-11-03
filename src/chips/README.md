# chips — `sid6581`

A Csound 7 plugin wrapping chip emulators from
[floooh/chips](https://github.com/floooh/chips). Its first opcodes, `sid6581`
and `chipadsr`, wrap the MOS Technology 6581 "SID" sound chip
(`chips/m6581.h`). The emulator is header-only and is **vendored in this
directory** (`m6581.h`), so the plugin has no external dependencies.

The SID was the sound chip of the Commodore 64: three independent voices,
each with a 16-bit frequency register, a waveform generator and an ADSR
envelope, mixed through a single multi-mode filter.

## Opcode signatures

```
iEnv:ChipADSR  chipadsr  kA, kD, kS, kR

aOut  sid6581  kFreq1, kWave1, kPw1, kGate1, iEnv1, \
                kFreq2, kWave2, kPw2, kGate2, iEnv2, \
                kFreq3, kWave3, kPw3, kGate3, iEnv3, \
                kCutoff, kRes, iFilterMode, iRouteMask
```

`chipadsr` builds a `ChipADSR` value from A (attack seconds), D (decay
seconds), S (sustain 0..1) and R (release seconds). `sid6581` has one a-rate
output (mono), three k-rate voice parameter sets, one `ChipADSR` per voice,
two k-rate filter parameters and two i-rate filter parameters. All k-rate
parameters are updated once per control cycle.

## Parameters

### `chipadsr`

| Arg | Rate | Meaning |
| --- | --- | --- |
| `kA` | k | attack time in seconds (rise 0 -> full); sets `.att` |
| `kD` | k | decay time in seconds (fall full -> sustain); sets `.dec` |
| `kS` | k | sustain level 0..1; sets `.sus` |
| `kR` | k | release time in seconds (fall sustain -> 0); sets `.rel` |

### `sid6581`

| Arg | Rate | Meaning | Range |
| --- | --- | --- | --- |
| `kFreq1..3` | k | voice frequency in Hz | 0 .. ~3850 Hz |
| `kWave1..3` | k | waveform selection (bitmask, see below) | 0 .. 15 |
| `kPw1..3` | k | pulse-width register | 0 .. 4095 |
| `kGate1..3` | k | envelope gate | >= 0.5 on, else off |
| `iEnv1..3` | ChipADSR | envelope for each voice (from `chipadsr`) | |
| `kCutoff` | k | filter cutoff register | 0 .. 2047 |
| `kRes` | k | filter resonance | 0 .. 15 |
| `iFilterMode` | i | filter mode (bitmask, see below) | 0 .. 15 |
| `iRouteMask` | i | voices routed through the filter (bitmask) | 0 .. 7 |

### Waveform bitmask (`kWave`)

The waveform is selected by the same bit encoding as the SID control
register's waveform nibble. Values combine to give the classic SID mixed
waveforms:

| Bit | Value | Waveform |
| --- | --- | --- |
| 0 | 1 | triangle |
| 1 | 2 | sawtooth |
| 2 | 4 | pulse |
| 3 | 8 | noise |

Examples: `1` = triangle, `2` = saw, `4` = pulse, `8` = noise,
`3` = triangle + saw, `6` = saw + pulse, `7` = tri + saw + pulse.

`kPw` only affects the output when the pulse bit is set.

### Envelope (`ChipADSR`)

Each voice has one ADSR envelope. `chipadsr` builds a `ChipADSR` from four
values, which are also available as **named members** you can read and write
with dot notation:

| Member | Meaning |
| --- | --- |
| `.att` | attack time in seconds |
| `.dec` | decay time in seconds |
| `.sus` | sustain level 0..1 |
| `.rel` | release time in seconds |

```csound
env:ChipADSR = chipadsr(0.01, 0.1, 0.8, 0.2)  ; build one
env.sus = 0.5                                 ; tweak a member
kLevel = env.sus                              ; read a member
```

The SID's envelope registers are written only when a voice's gate changes:

- **note-on** (gate rises): `.att`, `.dec`, `.sus` and `.rel` are latched;
- **note-off** (gate falls): only `.rel` is refreshed.

So k-rate changes to the `ChipADSR` values take effect on the *next* note,
never mid-note. Each voice is monophonic: a gate rising before a previous
release has finished re-attacks and cuts the tail (authentic SID behaviour).
To hear the full attack -> decay -> sustain -> release, the gate must rise
and later fall.

The SID stores ADSR as 4-bit register nibbles, so the seconds / 0..1 values
are quantised to the nearest of 16 attack, decay, sustain and release values.
The SID envelope timing is also non-linear (a fast linear attack and an
exponential decay/release), so the values are musical approximations.

### Filter mode (`iFilterMode`)

| Bit | Value | Meaning |
| --- | --- | --- |
| 0 | 1 | low-pass |
| 1 | 2 | band-pass |
| 2 | 4 | high-pass |
| 3 | 8 | voice 3 off (3OFF) |

Modes combine (e.g. `3` = LP + BP). With `8`, voice 3 is silenced.

### Filter routing (`iRouteMask`)

Selects which voices pass through the filter (voices not selected are sent
directly to the output, unfiltered):

| Bit | Value | Voice |
| --- | --- | --- |
| 0 | 1 | voice 1 |
| 1 | 2 | voice 2 |
| 2 | 4 | voice 3 |

`7` routes all three voices through the filter; `0` bypasses the filter
entirely.

## Fixed settings

A few SID parameters are not part of the signature and are fixed:

- **master volume**: 15 (maximum)
- **SID clock**: 985248 Hz (PAL), which the envelopes and filter are tuned to

The output is single-channel and roughly in the range [-1, 1]; duplicate it
to both channels for stereo.

## Building

The plugin builds with the rest of this repository. It requires a Csound 7
installation and is enabled by default:

```
cmake -DBUILD_CHIPS_OPCODES=ON ..
```

Disable it with `-DBUILD_CHIPS_OPCODES=OFF`.

## Examples

The directory ships two examples, both in the modern (functional) orchestra
syntax: `example.csd` (a chord and a gated note, shown below) and
`example2.csd` (a complete three-voice "Double Dragon" style sequencer).

```csound
<CsoundSynthesizer>
<CsOptions>
-odac
--opcode-lib=../../build/src/chips/Release/chips.dll
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1

; A sustained three-voice chord, all voices low-pass filtered.
instr 1
  env1:ChipADSR = chipadsr(0.01, 0.1, 0.8, 0.2)
  env2:ChipADSR = chipadsr(0.02, 0.2, 0.6, 0.4)
  env3:ChipADSR = chipadsr(0.005, 0.3, 0.7, 0.3)

  a1:a = sid6581(261.63, 1, 2048, 1, env1, \   ; C4  triangle
                 329.63, 2, 1024, 1, env2, \   ; E4  sawtooth
                 392.00, 4,  512, 1, env3, \   ; G4  pulse
                 1200, 6, 1, 7)                ; cutoff, res, low-pass, route all
  outs(a1, a1)
endin

; A single gated voice, envelope triggered by a k-rate gate.
instr 2
  kGate:k = linseg(0, 0.05, 1, p3 - 0.1, 1, 0.05, 0)
  env:ChipADSR = chipadsr(0.01, 0.1, 0.7, 0.3)
  a1:a = sid6581(220, 4, 2048, kGate, env, \
                 0,   0, 0,    0,     env, \
                 0,   0, 0,    0,     env, \
                 0, 0, 0, 0)
  outs(a1 * 0.5, a1 * 0.5)
endin
</CsInstruments>
<CsScore>
i 1 0 4
i 2 0 0.5
i 2 0.5 0.5
e
</CsScore>
</CsoundSynthesizer>
```

The `--opcode-lib` option loads the plugin library. The examples use a
relative path (`../../build/src/chips/Release/`) to the build output, so
run them from this `src/chips` directory:

```
csound example.csd
csound example2.csd
```

On Windows the library is `chips.dll`; on Linux/macOS it is
`chips.so`/`chips.dylib` (adjust the path to your build directory if it
differs).

## License

The plugin source is licensed under the LGPL-2.1-or-later, like the rest of
this repository. The vendored `m6581.h` is (c) 2018 Andre Weissflog,
provided under the zlib/libpng license; its SID implementation is based on
tedplay (c) 2012 Attila Grosz, used under the Unlicense. See the header
comment in `m6581.h` for the full notices.
