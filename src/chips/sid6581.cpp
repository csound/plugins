/*
    sid6581.cpp

    Csound opcode wrapping the floooh/chips m6581 (MOS 6581 "SID") emulator.

    This opcodes is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    The opcode is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this opcode; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA

    The m6581 emulator (m6581.h) is vendored in this directory. It is
    (c) 2018 Andre Weissflog and provided under the zlib/libpng license;
    the SID implementation itself is based on tedplay (c) 2012 Attila
    Grosz, used under the Unlicense. See m6581.h for the full notices.

    OPCODE

    iEnv:Sid6581ADSR    sid6581adsr   kA, kD, kS, kR
    iVoice:Sid6581Voice sid6581voice  kFreq, kWave, kPw

    out = sid6581(iVoice1, kGate1, iEnv1, \
                  iVoice2, kGate2, iEnv2, \
                  iVoice3, kGate3, iEnv3, \
                  kCutoff, kRes, iFilterMode, iRouteMask)

    Parameters
      iVoice1..3  Sid6581Voice for each voice, built with sid6581voice. It
                  has three k-rate members accessible with dot notation:
                  .freq, .wave and .pw (e.g. v1.freq = 440).
        .freq     voice frequency in Hz (0..~4 kHz); converted to the
                  SID 16-bit frequency register.
        .wave     waveform bitmask: 1 = triangle, 2 = sawtooth,
                  4 = pulse, 8 = noise; values combine (e.g. 3 = tri+saw,
                  6 = saw+pulse).
        .pw       pulse width register value (0..4095, 12-bit). Ignored
                  unless the pulse waveform is selected.
      kGate1..3   envelope gate: >= 0.5 gates on (attack/sustain),
                  otherwise off (release).
      iEnv1..3    Sid6581ADSR envelope for each voice. A Sid6581ADSR value
                  is built with the sid6581adsr opcode and has four k-rate
                  members accessible with dot notation - .att (attack
                  seconds), .dec (decay seconds), .sus (sustain 0..1) and
                  .rel (release seconds) - which may be read or written
                  individually (e.g. env1.sus = 0.5). The SID envelope
                  registers are only written on a gate transition: A/D/S/R
                  are latched on note-on and R is refreshed on note-off,
                  so k-rate changes take effect on the next note.
      kCutoff     filter cutoff register value (0..2047, 11-bit).
      kRes        filter resonance (0..15, 4-bit).
      iFilterMode filter mode bitmask: 1 = low-pass, 2 = band-pass,
                  4 = high-pass, 8 = voice 3 off (3OFF). The mute applies
                  only to voices outside the filter route, so if voice 3
                  is in iRouteMask (e.g. iRouteMask = 7) it is still heard.
      iRouteMask  filter routing bitmask: 1 = voice 1, 2 = voice 2,
                  4 = voice 3 (which voices pass through the filter).

    The output volume is fixed at maximum (15) and the SID clock is the PAL
    rate (985248 Hz); neither is exposed as an argument.
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
#include <csound_structs.h>

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

/* Register base addresses for each of the three voices, as
   { freq_lo, pw_lo, ctrl }. */
static const uint8_t VOICE_REGS[3][3] = {
    { M6581_V1_FREQ_LO, M6581_V1_PW_LO, M6581_V1_CTRL },
    { M6581_V2_FREQ_LO, M6581_V2_PW_LO, M6581_V2_CTRL },
    { M6581_V3_FREQ_LO, M6581_V3_PW_LO, M6581_V3_CTRL },
};
static const uint8_t VOICE_ATKDEC[3] = {
    M6581_V1_ATKDEC, M6581_V2_ATKDEC, M6581_V3_ATKDEC,
};
static const uint8_t VOICE_SUSREL[3] = {
    M6581_V1_SUSREL, M6581_V2_SUSREL, M6581_V3_SUSREL,
};

/* -------------------------------------------------------------------------
   Sid6581ADSR user-defined struct (named members A/D/S/R)
   ------------------------------------------------------------------------- */

/* The Sid6581ADSR orchestra type holds four k-rate members: att (attack
   seconds), dec (decay seconds), sus (sustain 0..1) and rel (release
   seconds). It is registered with csound->RegisterStruct() so the members
   can be read and written with dot notation in the orchestra, e.g.
   env1.sus = 0.5. Csound stores each member in its own CS_VAR_MEM block, so
   the members are NOT contiguous; opcode code reads/writes them through
   CS_STRUCT_VAR::members[i]->value. */
static const CSOUND_STRUCT_MEMBER SID6581ADSR_MEMBERS[4] = {
    { "att", "k" },
    { "dec", "k" },
    { "sus", "k" },
    { "rel", "k" },
};

/* The Sid6581Voice orchestra type holds the three k-rate members that
   describe a voice's tone: freq (Hz), wave (waveform bitmask) and pw
   (pulse-width register). */
static const CSOUND_STRUCT_MEMBER SID6581VOICE_MEMBERS[3] = {
    { "freq", "k" },
    { "wave", "k" },
    { "pw",   "k" },
};

/* Read the i-th numeric member of a struct value (0 if absent). */
static cs_float struct_member(const CS_STRUCT_VAR *s, int i) {
    if (s == NULL || s->members == NULL || s->members[i] == NULL)
        return FL(0.0);
    return s->members[i]->value;
}

/* Read the i-th member (0=att, 1=dec, 2=sus, 3=rel) of a Sid6581ADSR value. */
static cs_float adsr_member(const CS_STRUCT_VAR *env, int i) {
    return struct_member(env, i);
}

/* -------------------------------------------------------------------------
   Envelope conversion: "friendly" seconds / 0..1 values -> SID nibbles
   ------------------------------------------------------------------------- */

/* Number of SID ticks per envelope step, one entry per rate nibble (0..15).
   The SID envelope counter is a 15-bit LFSR loaded with 0x7FFF and clocked
   until it matches _m6581_rate_count_period[nibble]; simulate it here to
   recover the exact per-step tick count. */
static double env_step_ticks[16];

/* Cumulative sum of _m6581_env_gen_dr_divisors:
   dr_prefix[k] = sum(0..k). Used to count envelope steps for decay/release,
   which advance through the exponential divisor table. */
static int dr_prefix[256];

/* Fill env_step_ticks and dr_prefix. Must run once before any envelope
   conversion. */
static void build_env_tables(void) {
    int n, i;
    for (n = 0; n < 16; n++) {
        uint32_t lfsr = 0x7FFF;
        uint32_t target = _m6581_rate_count_period[n];
        uint32_t steps = 0;
        while (lfsr != target && steps < 0x8000) {
            uint32_t fb = ((lfsr >> 14) ^ (lfsr >> 13)) & 1u;
            lfsr = ((lfsr << 1) | fb) & 0x7FFFu;
            steps++;
        }
        env_step_ticks[n] = (double)steps;
    }
    {
        int acc = 0;
        for (i = 0; i < 256; i++) {
            acc += (int)_m6581_env_gen_dr_divisors[i];
            dr_prefix[i] = acc;
        }
    }
}

/* Clamp a sustain value to a 4-bit nibble. */
static int sustain_nibble(cs_float s) {
    int n = (int)(s * 15.0 + 0.5);
    if (n < 0)
        n = 0;
    if (n > 15)
        n = 15;
    return n;
}

/* Sustain level (0..255) for a sustain nibble. */
static int sustain_level(int nib) { return nib * 0x11; }

/* Nearest rate nibble whose per-step tick count matches `target`. */
static uint8_t nearest_rate_nibble(double target) {
    double best = env_step_ticks[0] - target;
    uint8_t best_n = 0;
    int n;
    if (best < 0.0)
        best = -best;
    for (n = 1; n < 16; n++) {
        double d = env_step_ticks[n] - target;
        if (d < 0.0)
            d = -d;
        if (d < best) {
            best = d;
            best_n = (uint8_t)n;
        }
    }
    return best_n;
}

/* Attack: 255 linear steps from level 0 to 255. */
static uint8_t attack_nibble(cs_float sec) {
    return nearest_rate_nibble((double)sec * (double)SID_TICK_HZ / 255.0);
}

/* Decay: exponential drop from level 255 down to the sustain level. */
static uint8_t decay_nibble(cs_float sec, int s_nib) {
    int steps = dr_prefix[255] - dr_prefix[sustain_level(s_nib)];
    if (steps < 1)
        steps = 1;
    return nearest_rate_nibble((double)sec * (double)SID_TICK_HZ / (double)steps);
}

/* Release: exponential drop from the sustain level down to 0. */
static uint8_t release_nibble(cs_float sec, int s_nib) {
    int steps = dr_prefix[sustain_level(s_nib)] - dr_prefix[0];
    if (steps < 1)
        steps = 1;
    return nearest_rate_nibble((double)sec * (double)SID_TICK_HZ / (double)steps);
}

/* Convert a Sid6581ADSR value into SID ATKDEC/SUSREL register bytes. */
static void adsr_to_regs(const CS_STRUCT_VAR *env, uint8_t &atkdec,
                         uint8_t &susrel) {
    int s = sustain_nibble(adsr_member(env, 2));
    uint8_t a = attack_nibble(adsr_member(env, 0));
    uint8_t d = decay_nibble(adsr_member(env, 1), s);
    uint8_t r = release_nibble(adsr_member(env, 3), s);
    atkdec = (uint8_t)((a << 4) | d);
    susrel = (uint8_t)((s << 4) | r);
}

/* -------------------------------------------------------------------------
   sid6581adsr: build a Sid6581ADSR value from k-rate A/D/S/R inputs
   ------------------------------------------------------------------------- */
struct Sid6581Adsr : csnd::Plugin<1, 4> {
    /* Copy the four k-rate inputs into the output struct's members. */
    int kperf() {
        CS_STRUCT_VAR *out = (CS_STRUCT_VAR *)outargs.data(0);
        if (out == NULL || out->members == NULL)
            return OK;
        out->members[0]->value = inargs[0]; /* A */
        out->members[1]->value = inargs[1]; /* D */
        out->members[2]->value = inargs[2]; /* S */
        out->members[3]->value = inargs[3]; /* R */
        return OK;
    }
};

/* -------------------------------------------------------------------------
   sid6581voice: build a Sid6581Voice value from k-rate freq/wave/pw inputs
   ------------------------------------------------------------------------- */
struct Sid6581VoiceOp : csnd::Plugin<1, 3> {
    /* Copy the three k-rate inputs into the output struct's members. */
    int kperf() {
        CS_STRUCT_VAR *out = (CS_STRUCT_VAR *)outargs.data(0);
        if (out == NULL || out->members == NULL)
            return OK;
        out->members[0]->value = inargs[0]; /* freq */
        out->members[1]->value = inargs[1]; /* wave */
        out->members[2]->value = inargs[2]; /* pw */
        return OK;
    }
};

/* -------------------------------------------------------------------------
   sid6581: one full SID chip (three voices + filter)
   Inputs: 3 x (Sid6581Voice, kGate, Sid6581ADSR), kCutoff, kRes, iMode, iRoute
   ------------------------------------------------------------------------- */
struct Sid6581 : csnd::Plugin<1, 13> {
    /* state */
    m6581_t sid;
    uint8_t filter_mode;
    uint8_t route_mask;
    bool prev_gate[3];
    uint8_t latched_sustain[3];

    /* Write one SID register without advancing the emulated clock, so
       register updates don't alter the chip's timing relative to Csound. */
    void sid_write(uint8_t reg, uint8_t data) {
        m6581_write_reg(&sid, reg, data);
    }

    /* Initialise and reset the emulator, set a default envelope on each
       voice and read the i-rate filter mode and route mask. */
    int init() {
        m6581_desc_t desc;
        desc.tick_hz = SID_TICK_HZ;
        desc.sound_hz = (int)sr();
        desc.magnitude = 1.0f;
        m6581_init(&sid, &desc);
        m6581_reset(&sid);

        /* A default gate-friendly envelope (fast attack, full sustain, quick
           release); it is overwritten on the first gate transition. */
        for (int v = 0; v < 3; v++) {
            sid_write(VOICE_ATKDEC[v], 0x20);
            sid_write(VOICE_SUSREL[v], 0xF2);
            prev_gate[v] = false;
            latched_sustain[v] = 15;
        }

        /* i-rate inputs */
        filter_mode = (uint8_t)((int)inargs[11] & 0x0F);
        route_mask = (uint8_t)((int)inargs[12] & 0x07);
        return OK;
    }

    /* Per k-cycle: update voice, envelope and filter registers, then render
       ksmps samples of audio from the emulator. */
    int kperf() {
        cs_float *aout = outargs.data(0);
        uint32_t ksmps = (uint32_t)insdshead->ksmps;
        uint32_t n;
        int v;

        /* Write the voice registers from the k-rate inputs, latching each
           voice's envelope on a gate transition. */
        for (v = 0; v < 3; v++) {
            int base = v * 3;
            const CS_STRUCT_VAR *voice =
                (const CS_STRUCT_VAR *)inargs.data(base);
            cs_float gate = inargs[base + 1];
            bool gate_on = gate >= FL(0.5);
            const CS_STRUCT_VAR *env =
                (const CS_STRUCT_VAR *)inargs.data(base + 2);

            if (gate_on && !prev_gate[v]) {
                /* note-on: latch A/D/S/R */
                uint8_t atkdec, susrel;
                adsr_to_regs(env, atkdec, susrel);
                sid_write(VOICE_ATKDEC[v], atkdec);
                sid_write(VOICE_SUSREL[v], susrel);
                latched_sustain[v] = susrel >> 4;
            }
            else if (!gate_on && prev_gate[v]) {
                /* note-off: refresh the release rate only */
                uint8_t r = release_nibble(adsr_member(env, 3),
                                           latched_sustain[v]);
                sid_write(VOICE_SUSREL[v],
                          (uint8_t)((latched_sustain[v] << 4) | r));
            }
            prev_gate[v] = gate_on;

            write_voice(VOICE_REGS[v][0], VOICE_REGS[v][1], VOICE_REGS[v][2],
                        struct_member(voice, 0), struct_member(voice, 1),
                        struct_member(voice, 2), gate);
        }

        /* Filter registers. */
        {
            uint16_t cutoff = 0;
            uint8_t res = 0;
            cs_float c = inargs[9];
            cs_float r = inargs[10];
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

/* Build the envelope tables, then register the Sid6581ADSR and Sid6581Voice
   types and the sid6581adsr, sid6581voice and sid6581 opcodes. */
static void onload(csnd::Csound *csound) {
    CSOUND *cs = csound->get_csound();

    /* Build the envelope conversion tables once, before any performance. */
    build_env_tables();

    /* Register the Sid6581ADSR struct type (named members A/D/S/R). */
    if (cs->RegisterStruct != NULL)
        cs->RegisterStruct(cs, "Sid6581ADSR", SID6581ADSR_MEMBERS, 4);

    if (cs->RegisterStruct != NULL)
        cs->RegisterStruct(cs, "Sid6581Voice", SID6581VOICE_MEMBERS, 3);

    csnd::plugin<Sid6581Adsr>(csound, "sid6581adsr", ":Sid6581ADSR;", "kkkk",
                              csnd::thread::k);
    csnd::plugin<Sid6581VoiceOp>(csound, "sid6581voice", ":Sid6581Voice;",
                                 "kkk", csnd::thread::k);
    csnd::plugin<Sid6581>(
        csound, "sid6581", "a",
        ":Sid6581Voice;k:Sid6581ADSR;:Sid6581Voice;k:Sid6581ADSR;"
        ":Sid6581Voice;k:Sid6581ADSR;kkii",
        csnd::thread::ik);
}

#ifdef BUILD_PLUGINS
#include <modload.h>
/* Entry point used when the plugin is built as part of the plugins tree. */
void csnd::on_load(csnd::Csound *csound) { onload(csound); }
#else
/* Entry point used when Csound loads the library as a standalone module. */
extern "C" int32_t sid6581_init_modules(CSOUND *csound) {
    onload((csnd::Csound *)csound);
    return OK;
}
#endif
