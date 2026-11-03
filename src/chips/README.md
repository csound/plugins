# chips — `sid6581`

A Csound 7 plugin wrapping chip emulators from
[floooh/chips](https://github.com/floooh/chips). Its first opcode, `sid6581`,
wraps the MOS Technology 6581 "SID" sound chip (`chips/m6581.h`). The
emulator is header-only and is **vendored in this directory** (`m6581.h`),
so the plugin has no external dependencies.

The SID was the sound chip of the Commodore 64: three independent voices,
each with a 16-bit frequency register, a waveform generator and an ADSR
envelope, mixed through a single multi-mode filter.

## Opcode signature

```
aOut  sid6581  kFreq1, kWave1, kPw1, kGate1, \
                 kFreq2, kWave2, kPw2, kGate2, \
                 kFreq3, kWave3, kPw3, kGate3, \
                 kCutoff, kRes, iFilterMode, iRouteMask
```

The opcode has one **a-rate** output (mono) and 16 inputs: twelve k-rate
voice parameters, two k-rate filter parameters and two i-rate filter
parameters. All parameter values are updated once per control cycle.

## Parameters

| Arg | Rate | Meaning | Range |
| --- | --- | --- | --- |
| `kFreq1..3` | k | voice frequency in Hz | 0 .. ~3850 Hz |
| `kWave1..3` | k | waveform selection (bitmask, see below) | 0 .. 15 |
| `kPw1..3` | k | pulse-width register | 0 .. 4095 |
| `kGate1..3` | k | envelope gate | >= 0.5 on, else off |
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
- **envelope (ADSR)**: fast attack, full sustain, quick release, so that a
  gated note sustains until its gate drops below 0.5 (the raw SID registers
  otherwise default to sustain = 0, silencing every note after its attack)

The output is single-channel and roughly in the range [-1, 1]; duplicate it
to both channels for stereo.

## Building

The plugin builds with the rest of this repository. It requires a Csound 7
installation and is enabled by default:

```
cmake -DBUILD_CHIPS_OPCODES=ON ..
```

Disable it with `-DBUILD_CHIPS_OPCODES=OFF`.

## Example

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
  aL sid6581 261.63, 1, 2048, 1,   ; C4  triangle
               329.63, 2, 1024, 1,   ; E4  sawtooth
               392.00, 4,  512, 1,   ; G4  pulse
               1200, 6, 1, 7         ; cutoff, res, low-pass, route all
  outs aL, aL
endin

; A single gated voice, envelope triggered by a k-rate gate.
instr 2
  kGate linseg 0, 0.05, 1, p3 - 0.1, 1, 0.05, 0
  aL sid6581 220, 4, 2048, kGate, \
               0,   0, 0,    0,     \
               0,   0, 0,    0,     \
               0, 0, 0, 0
  outs aL * 0.5, aL * 0.5
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
run it from this `src/chips` directory:

```
csound example.csd
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
