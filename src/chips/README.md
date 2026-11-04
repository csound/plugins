# chips — `sid6581`

A Csound 7 plugin wrapping chip emulators from
[floooh/chips](https://github.com/floooh/chips). Its first opcodes, `sid6581`
and `sid6581adsr`, wrap the MOS Technology 6581 "SID" sound chip
(`chips/m6581.h`). The emulator is header-only and is **vendored in this
directory** (`m6581.h`), so the plugin has no external dependencies.

The SID was the sound chip of the Commodore 64: three independent voices,
each with a 16-bit frequency register, a waveform generator and an ADSR
envelope, mixed through a single multi-mode filter.

## Opcode signatures

```
iEnv:Sid6581ADSR = sid6581adsr(kA, kD, kS, kR)
iVoice:Sid6581Voice = sid6581voice(kFreq, kWave, kPw)

aOut:a = sid6581(iVoice1, kGate1, iEnv1, \
                 iVoice2, kGate2, iEnv2, \
                 iVoice3, kGate3, iEnv3, \
                 kCutoff, kRes, iFilterMode, iRouteMask)
```

`sid6581adsr` builds a `Sid6581ADSR` value from A (attack seconds), D (decay
seconds), S (sustain 0..1) and R (release seconds). `sid6581voice` builds a
`Sid6581Voice` value from a frequency, a waveform bitmask and a pulse width.
`sid6581` has one a-rate output (mono), and for each of its three voices a
`Sid6581Voice`, a k-rate gate and a `Sid6581ADSR`, followed by two k-rate filter
parameters and two i-rate filter parameters. All k-rate parameters are
updated once per control cycle.

## Parameters

### `sid6581adsr`

| Arg | Rate | Meaning |
| --- | --- | --- |
| `kA` | k | attack time in seconds (rise 0 -> full); sets `.att` |
| `kD` | k | decay time in seconds (fall full -> sustain); sets `.dec` |
| `kS` | k | sustain level 0..1; sets `.sus` |
| `kR` | k | release time in seconds (fall sustain -> 0); sets `.rel` |

### `sid6581voice`

| Arg | Rate | Meaning | Range |
| --- | --- | --- | --- |
| `kFreq` | k | voice frequency in Hz; sets `.freq` | 0 .. ~3850 Hz |
| `kWave` | k | waveform selection (bitmask, see below); sets `.wave` | 0 .. 15 |
| `kPw` | k | pulse-width register; sets `.pw` | 0 .. 4095 |

The members can be read and written with dot notation, e.g.
`v.freq = 440`.

### `sid6581`

| Arg | Rate | Meaning | Range |
| --- | --- | --- | --- |
| `iVoice1..3` | Sid6581Voice | tone of each voice (from `sid6581voice`) | |
| `kGate1..3` | k | envelope gate | >= 0.5 on, else off |
| `iEnv1..3` | Sid6581ADSR | envelope for each voice (from `sid6581adsr`) | |
| `kCutoff` | k | filter cutoff register | 0 .. 2047 |
| `kRes` | k | filter resonance | 0 .. 15 |
| `iFilterMode` | i | filter mode (bitmask, see below) | 0 .. 15 |
| `iRouteMask` | i | voices routed through the filter (bitmask) | 0 .. 7 |

### Waveform bitmask (`.wave`)

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

`.pw` only affects the output when the pulse bit is set.

### Envelope (`Sid6581ADSR`)

Each voice has one ADSR envelope. `sid6581adsr` builds a `Sid6581ADSR` from four
values, which are also available as **named members** you can read and write
with dot notation:

| Member | Meaning |
| --- | --- |
| `.att` | attack time in seconds |
| `.dec` | decay time in seconds |
| `.sus` | sustain level 0..1 |
| `.rel` | release time in seconds |

```csound
env:Sid6581ADSR = sid6581adsr(0.01, 0.1, 0.8, 0.2)  ; build one
env.sus = 0.5                                       ; tweak a member
kLevel:k = env.sus                                  ; read a member
```

The SID's envelope registers are written only when a voice's gate changes:

- **note-on** (gate rises): `.att`, `.dec`, `.sus` and `.rel` are latched;
- **note-off** (gate falls): only `.rel` is refreshed.

So k-rate changes to the `Sid6581ADSR` values take effect on the *next* note,
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

Modes combine (e.g. `3` = LP + BP). With `8`, voice 3 is silenced, except when voice 3 is included in
`iRouteMask`: the emulator applies the mute only to voices outside the filter
route, so with `iRouteMask = 7` (or any mask containing `4`) voice 3 is still
heard.

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

The directory ships `example.csd`, a longer arranged track with lead, bass,
arpeggio and drums, along with `chiptune.ogg`, a rendering of it. Both use
the modern (functional) orchestra syntax with explicit type declarations.
The listing below is a simplified chord and gated note.

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
  env1:Sid6581ADSR = sid6581adsr(0.01, 0.1, 0.8, 0.2)
  env2:Sid6581ADSR = sid6581adsr(0.02, 0.2, 0.6, 0.4)
  env3:Sid6581ADSR = sid6581adsr(0.005, 0.3, 0.7, 0.3)

  v1:Sid6581Voice = sid6581voice(261.63, 1, 2048)   ; C4  triangle
  v2:Sid6581Voice = sid6581voice(329.63, 2, 1024)   ; E4  sawtooth
  v3:Sid6581Voice = sid6581voice(392.00, 4,  512)   ; G4  pulse

  out:a = sid6581(v1, 1, env1, \
                 v2, 1, env2, \
                 v3, 1, env3, \
                 1200, 6, 1, 7)             ; cutoff, res, low-pass, route all
  outs(out, out)
endin

; A single gated voice, envelope triggered by a k-rate gate.
instr 2
  kGate:k = linseg(0, 0.05, 1, p3 - 0.1, 1, 0.05, 0)
  env:Sid6581ADSR = sid6581adsr(0.01, 0.1, 0.7, 0.3)
  voice:Sid6581Voice = sid6581voice(220, 4, 2048)
  off:Sid6581Voice = sid6581voice(0, 0, 0)
  a1:a = sid6581(voice, kGate, env, \
                 off, 0, env, \
                 off, 0, env, \
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

The `--opcode-lib` option loads the plugin library. The example uses a
relative path (`../../build/src/chips/Release/`) to the build output, so
run it from this `src/chips` directory.

On Windows the library is `chips.dll`; on Linux/macOS it is
`chips.so`/`chips.dylib` (adjust the path to your build directory if it
differs).

## License

The plugin source is licensed under the LGPL-2.1-or-later, like the rest of
this repository. The vendored `m6581.h` is (c) 2018 Andre Weissflog,
provided under the zlib/libpng license; its SID implementation is based on
tedplay (c) 2012 Attila Grosz, used under the Unlicense. See the header
comment in `m6581.h` for the full notices.
