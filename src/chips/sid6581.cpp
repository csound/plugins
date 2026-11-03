/*
    sid6581.cpp

    Csound opcode wrapping the floooh/chips m6581 (MOS 6581 "SID") emulator.

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA

    The m6581 emulator (m6581.h) is vendored in this directory. It is
    (c) 2018 Andre Weissflog and provided under the zlib/libpng license;
    the SID implementation itself is based on tedplay (c) 2012 Attila
    Grosz, used under the Unlicense. See m6581.h for the full notices.

    OPCODE

    aOut sid6581 kFreq1, kWave1, kPw1, kGate1, \
                   kFreq2, kWave2, kPw2, kGate2, \
                   kFreq3, kWave3, kPw3, kGate3, \
                   kCutoff, kRes, iFilterMode, iRouteMask

    Parameters
      kFreq1..3   voice frequency in Hz (0..~4 kHz); converted to the
                  SID 16-bit frequency register.
      kWave1..3   waveform bitmask: 1 = triangle, 2 = sawtooth,
                  4 = pulse, 8 = noise; values combine (e.g. 3 = tri+saw,
                  6 = saw+pulse).
      kPw1..3     pulse width register value (0..4095, 12-bit). Ignored
                  unless the pulse waveform is selected.
      kGate1..3   envelope gate: >= 0.5 gates on (attack/sustain),
                  otherwise off (release).
      kCutoff     filter cutoff register value (0..2047, 11-bit).
      kRes        filter resonance (0..15, 4-bit).
      iFilterMode filter mode bitmask: 1 = low-pass, 2 = band-pass,
                  4 = high-pass, 8 = voice 3 off (3OFF).
      iRouteMask  filter routing bitmask: 1 = voice 1, 2 = voice 2,
                  4 = voice 3 (which voices pass through the filter).

    The output volume is fixed at maximum (15), the SID clock is the PAL
    rate (985248 Hz) and the voice envelopes are a fixed gate-friendly ADSR
    (fast attack, full sustain, quick release); none of these are exposed as
    arguments yet.
*/

/* m6581.h must be included before any Csound header so that its <math.h>
   include (with _USE_MATH_DEFINES on MSVC) is the first one in the
   translation unit. Wrapping it in extern "C" keeps the C-linkage
   declarations and the CHIPS_IMPL definitions in agreement when compiled
   as C++. */
#define CHIPS_IMPL
extern "C" {
#include "m6581.h"
}

#include <plugin.h>

/* SID clock frequency (PAL). */
#define SID_TICK_HZ 985248

/* SID master volume (0..15), fixed since it is not part of the signature. */
#define SID_VOLUME 15

/* Convert a frequency in Hz to the SID 16-bit frequency register value. */
static uint16_t freq_to_reg(cs_float hz) {
    double value;
    if (hz <= FL(0.0))
        return 0;
    value = (double)hz * 16777216.0 / (double)SID_TICK_HZ;
    if (value > 65535.0)
        value = 65535.0;
    return (uint16_t)(value + 0.5);
}

/* Build a voice control register from a waveform bitmask and a gate. */
static uint8_t voice_ctrl(cs_float wave, cs_float gate) {
    int w = (wave >= FL(0.0)) ? (int)wave : 0;
    uint8_t ctrl = (uint8_t)((w & 0x0F) << 4);
    if (gate >= FL(0.5))
        ctrl |= M6581_CTRL_GATE;
    return ctrl;
}

struct Sid6581 : csnd::Plugin<1, 16> {
    /* state */
    m6581_t sid;
    uint8_t filter_mode;
    uint8_t route_mask;

    /* Write one SID register (one bus cycle, i.e. one m6581_tick). */
    void sid_write(uint8_t reg, uint8_t data) {
        uint64_t pins = M6581_CS | ((uint64_t)(reg & M6581_ADDR_MASK)) |
                        ((uint64_t)data << 16);
        m6581_tick(&sid, pins); /* RW pin low => write */
    }

    int init() {
        m6581_desc_t desc;
        desc.tick_hz = SID_TICK_HZ;
        desc.sound_hz = (int)sr();
        desc.magnitude = 1.0f;
        m6581_init(&sid, &desc);
        m6581_reset(&sid);

        /* The ADSR parameters are not part of the opcode signature, so give
           every voice a fixed, gate-friendly envelope: fast attack, full
           sustain and a quick release. Without this the SID's zeroed sustain
           level would silence each note right after its attack/decay
           transient (ATKDEC: attack 2, decay 0; SUSREL: sustain 15,
           release 2). */
        sid_write(M6581_V1_ATKDEC, 0x20);
        sid_write(M6581_V1_SUSREL, 0xF2);
        sid_write(M6581_V2_ATKDEC, 0x20);
        sid_write(M6581_V2_SUSREL, 0xF2);
        sid_write(M6581_V3_ATKDEC, 0x20);
        sid_write(M6581_V3_SUSREL, 0xF2);

        /* i-rate inputs */
        filter_mode = (uint8_t)((int)inargs[14] & 0x0F);
        route_mask = (uint8_t)((int)inargs[15] & 0x07);
        return OK;
    }

    int kperf() {
        cs_float *aout = outargs.data(0);
        uint32_t ksmps = (uint32_t)insdshead->ksmps;
        uint32_t n;

        /* Write the voice registers from the k-rate inputs. */
        write_voice(M6581_V1_FREQ_LO, M6581_V1_PW_LO, M6581_V1_CTRL,
                    inargs[0], inargs[1], inargs[2], inargs[3]);
        write_voice(M6581_V2_FREQ_LO, M6581_V2_PW_LO, M6581_V2_CTRL,
                    inargs[4], inargs[5], inargs[6], inargs[7]);
        write_voice(M6581_V3_FREQ_LO, M6581_V3_PW_LO, M6581_V3_CTRL,
                    inargs[8], inargs[9], inargs[10], inargs[11]);

        /* Filter registers. */
        {
            uint16_t cutoff = 0;
            uint8_t res = 0;
            cs_float c = inargs[12];
            cs_float r = inargs[13];
            if (c > FL(0.0))
                cutoff = (c > FL(2047.0)) ? 2047 : (uint16_t)(c + FL(0.5));
            if (r > FL(0.0))
                res = (r > FL(15.0)) ? 15 : (uint8_t)(r + FL(0.5));
            sid_write(M6581_FC_LO, (uint8_t)(cutoff & 0x07));
            sid_write(M6581_FC_HI, (uint8_t)((cutoff >> 3) & 0xFF));
            sid_write(M6581_RES_FILT, (uint8_t)((res << 4) | (route_mask & 0x07)));
            sid_write(M6581_MODE_VOL, (uint8_t)((filter_mode << 4) | SID_VOLUME));
        }

        /* Generate the audio vector, honouring the sample-accurate offset
           and early-exit regions. */
        for (n = 0; n < offset; n++)
            aout[n] = FL(0.0);
        for (n = offset; n < nsmps; n++) {
            uint64_t pins;
            do {
                pins = m6581_tick(&sid, 0);
            } while ((pins & M6581_SAMPLE) == 0);
            aout[n] = (cs_float)sid.sample;
        }
        for (n = nsmps; n < ksmps; n++)
            aout[n] = FL(0.0);

        return OK;
    }

    /* Write the frequency, pulse-width and control registers for one voice. */
    void write_voice(uint8_t freq_lo, uint8_t pw_lo, uint8_t ctrl_reg,
                     cs_float freq, cs_float wave, cs_float pw,
                     cs_float gate) {
        uint16_t f = freq_to_reg(freq);
        sid_write(freq_lo, (uint8_t)(f & 0xFF));
        sid_write((uint8_t)(freq_lo + 1), (uint8_t)((f >> 8) & 0xFF));

        uint16_t p = 0;
        if (pw > FL(0.0))
            p = (pw > FL(4095.0)) ? 4095 : (uint16_t)(pw + FL(0.5));
        sid_write(pw_lo, (uint8_t)(p & 0xFF));
        sid_write((uint8_t)(pw_lo + 1), (uint8_t)((p >> 8) & 0x0F));

        sid_write(ctrl_reg, voice_ctrl(wave, gate));
    }
};

static void onload(csnd::Csound *csound) {
    csnd::plugin<Sid6581>(csound, "sid6581", "a",
                            "kkkkkkkkkkkkkkii", csnd::thread::ik);
}

#ifdef BUILD_PLUGINS
#include <modload.h>
void csnd::on_load(csnd::Csound *csound) { onload(csound); }
#else
extern "C" int32_t sid6581_init_modules(CSOUND *csound) {
    onload((csnd::Csound *)csound);
    return OK;
}
#endif
