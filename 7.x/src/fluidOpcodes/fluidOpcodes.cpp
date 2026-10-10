/**
 * FLUID SYNTH OPCODES
 *
 * Adapts Fluidsynth to use global engines, soundFonts, and outputs
 *
 * Based on work by Michael Gogins and Steven Yi.  License is identical to
 * SOUNDFONTS VST License (listed below)
 *
 * Copyright (c) 2003 by Steven Yi. All rights reserved.
 *
 * [ORIGINAL INFORMATION BELOW]
 *
 * S O U N D F O N T S   V S T
 *
 * Adapts Fluidsynth to be both a VST plugin instrument
 * and a Csound plugin opcode.
 * Copyright (c) 2001-2003 by Michael Gogins. All rights reserved.
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <cmath>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <sstream>
#include "OpcodeBase.hpp"
#include "csdl.h"
#include <fluidsynth.h>
#include <fluidsynth/version.h>

static std::vector<fluid_synth_t *> &fluidsynths_for_ids() {
    static std::vector<fluid_synth_t *> fluidsynths_for_ids_;
    return fluidsynths_for_ids_;
}

static std::mutex &fluidsynths_for_ids_mutex() {
    // When both locks are needed, acquire fluid_synths_mutex first.
    static std::mutex mutex;
    return mutex;
}

static void tof(fluid_synth_t *f, cs_float *a) {
    std::lock_guard<std::mutex> lock(fluidsynths_for_ids_mutex());
    auto &instances = fluidsynths_for_ids();
    auto instance = std::find(instances.begin(), instances.end(), f);
    if (instance == instances.end()) {
        instances.push_back(f);
        instance = instances.end() - 1;
    }
    *a = instance - instances.begin();
};

static void toa(cs_float *f, fluid_synth_t *&a) {
    std::lock_guard<std::mutex> lock(fluidsynths_for_ids_mutex());
    auto &instances = fluidsynths_for_ids();
    int index = *f;
    a = index >= 0 && index < (int)instances.size() ? instances[index] : nullptr;
};

struct FluidEngineConfig {
  int32_t chorusEnabled;
  int32_t reverbEnabled;
  int32_t channelCount;
  int32_t voiceCount;
  double sampleRate;
};

struct NamedFluidEngine {
  fluid_synth_t *synth;
  FluidEngineConfig config;
};

using NamedFluidEngines = std::map<std::string, NamedFluidEngine>;
using FluidSoundFonts = std::map<fluid_synth_t *,
                                 std::map<std::string, int32_t>>;

static int32_t createFluidEngine(CSOUND *csound, cs_float *engineId,
                                 int32_t chorusEnabled, int32_t reverbEnabled,
                                 int32_t channelCount, int32_t voiceCount,
                                 const char *reuseName) {
  if (channelCount <= 0) {
    channelCount = 256;
  } else if (channelCount < 16) {
    channelCount = 16;
  } else if (channelCount > 256) {
    channelCount = 256;
  }
  if (voiceCount <= 0) {
    voiceCount = 4096;
  } else if (voiceCount < 16) {
    voiceCount = 16;
  } else if (voiceCount > 4096) {
    voiceCount = 4096;
  }

  FluidEngineConfig config = {chorusEnabled, reverbEnabled, channelCount,
                              voiceCount, (double)csound->GetEngineSr(csound)};
  void *fluid_synths_mutex = 0;
  std::vector<fluid_synth_t *> *fluid_synths = 0;
  NamedFluidEngines *named_engines = 0;
  FluidSoundFonts *soundfonts = 0;
  csound::QueryGlobalPointer(csound, "fluid_synths_mutex", fluid_synths_mutex);
  csound::QueryGlobalPointer(csound, "fluid_synths", fluid_synths);
  csound::QueryGlobalPointer(csound, "fluid_named_engines", named_engines);
  csound::QueryGlobalPointer(csound, "fluid_soundfonts", soundfonts);
  if (!fluid_synths_mutex || !fluid_synths || !named_engines || !soundfonts) {
    return csound->InitError(csound, "%s",
                             Str("error accessing fluid engine registry\n"));
  }

  csound::LockGuard synthsGuard(csound, fluid_synths_mutex);
  if (reuseName && *reuseName) {
    auto namedEngine = named_engines->find(reuseName);
    if (namedEngine != named_engines->end()) {
      const FluidEngineConfig &existing = namedEngine->second.config;
      if (existing.chorusEnabled != config.chorusEnabled ||
          existing.reverbEnabled != config.reverbEnabled ||
          existing.channelCount != config.channelCount ||
          existing.voiceCount != config.voiceCount ||
          existing.sampleRate != config.sampleRate) {
        return csound->InitError(
            csound, Str("fluid engine name %s was reused with different settings"),
            reuseName);
      }
      tof(namedEngine->second.synth, engineId);
      log(csound, "Reusing fluidEngine 0x%p as '%s'.\n",
          namedEngine->second.synth, reuseName);
      return OK;
    }
  }

  fluid_settings_t *fluidSettings = new_fluid_settings();
  fluid_synth_t *fluidSynth = 0;
  if (fluidSettings != NULL) {
    fluid_settings_setnum(fluidSettings, (char *)"synth.sample-rate",
                          config.sampleRate);
    fluid_settings_setint(fluidSettings, (char *)"synth.midi-channels",
                          channelCount);
    fluid_settings_setint(fluidSettings, (char *)"synth.polyphony", voiceCount);
    fluidSynth = new_fluid_synth(fluidSettings);
  }
  if (!fluidSynth) {
    if (fluidSettings) {
      delete_fluid_settings(fluidSettings);
    }
    return csound->InitError(csound, "%s", Str("error allocating fluid engine\n"));
  }

#if (FLUIDSYNTH_VERSION_MAJOR >= 2 && FLUIDSYNTH_VERSION_MINOR >= 2)
  fluid_synth_chorus_on(fluidSynth, -1, chorusEnabled);
  fluid_synth_reverb_on(fluidSynth, -1, reverbEnabled);
#else
  fluid_synth_set_chorus_on(fluidSynth, chorusEnabled);
  fluid_synth_set_reverb_on(fluidSynth, reverbEnabled);
#endif
  log(csound, "Created fluidEngine 0x%p with sampling rate = %f, "
              "chorus %s, reverb %s, channels %d, voices %d.\n",
      fluidSynth, config.sampleRate, chorusEnabled ? "on" : "off",
      reverbEnabled ? "on" : "off", channelCount, voiceCount);
  tof(fluidSynth, engineId);
  fluid_synths->push_back(fluidSynth);
  if (reuseName && *reuseName) {
    named_engines->emplace(reuseName, NamedFluidEngine{fluidSynth, config});
  }
  (*soundfonts)[fluidSynth];
  return OK;
}

class FluidEngine : public csound::OpcodeBase<FluidEngine> {
  // Outputs.
  cs_float *iFluidSynth;
  // Inputs.
  cs_float *iChorusEnabled;
  cs_float *iReverbEnabled;
  cs_float *iChannelCount;
  cs_float *iVoiceCount;
  // State.
  // fluid_synth_t *fluidSynth;
  // fluid_settings_t *fluidSettings;
  int32_t chorusEnabled;
  int32_t reverbEnabled;
  int32_t channelCount;
  int32_t voiceCount;
public:
  int32_t init(CSOUND *csound) {
    chorusEnabled = (int32_t)*iChorusEnabled;
    reverbEnabled = (int32_t)*iReverbEnabled;
    channelCount = (int32_t)*iChannelCount;
    voiceCount = (int32_t)*iVoiceCount;
    return createFluidEngine(csound, iFluidSynth, chorusEnabled, reverbEnabled,
                             channelCount, voiceCount, nullptr);
  }
};

class FluidEngineReuse : public csound::OpcodeBase<FluidEngineReuse> {
  cs_float *iFluidSynth;
  STRINGDAT *iName;
  cs_float *iChorusEnabled;
  cs_float *iReverbEnabled;
  cs_float *iChannelCount;
  cs_float *iVoiceCount;

public:
  int32_t init(CSOUND *csound) {
    if (!iName || !iName->data || !iName->data[0]) {
      return csound->InitError(csound, "%s",
                               Str("fluidEngineReuse requires a non-empty name"));
    }
    return createFluidEngine(
        csound, iFluidSynth, (int32_t)*iChorusEnabled,
        (int32_t)*iReverbEnabled, (int32_t)*iChannelCount,
        (int32_t)*iVoiceCount, iName->data);
  }
};

class FluidLoad : public csound::OpcodeBase<FluidLoad> {
  // Outputs.
  cs_float *iInstrumentNumber;
  // Inputs.
  cs_float *iFilename;
  cs_float *iFluidSynth;
  cs_float *iListPresets;
  // State.
  char *filename;
  char *filepath;
  fluid_synth_t *fluidSynth;
  int32_t soundFontId;
  int32_t listPresets;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
    mutex = csound->Create_Mutex(0);
    csound::LockGuard guard(csound, mutex);
    int32_t result = OK;
    soundFontId = -1;
    toa(iFluidSynth, fluidSynth);
    listPresets = (int32_t)*iListPresets;
    CS_TYPE *argType = GetTypeForArg(iFilename);
    if (strcmp("S", argType->varTypeName) == 0) {
      filename = csound->Strdup(csound, ((STRINGDAT *)iFilename)->data);
    } else {
      char legacy_name[MAXNAME];
      csound->StringArg2Name(
          csound, legacy_name,
          (IsStringCode(*iFilename) ? csound->GetArgString(csound, *iFilename)
                                    : (char *)iFilename),
          (char *)"fluid.sf2.", IsStringCode(*iFilename));
      filename = legacy_name;
    }

    filepath = csound->FindInputFile(csound, filename, "SFDIR;SSDIR");
    if (filepath && fluid_is_soundfont(filepath)) {
      void *fluid_synths_mutex = 0;
      FluidSoundFonts *soundfonts = 0;
      csound::QueryGlobalPointer(csound, "fluid_synths_mutex",
                                 fluid_synths_mutex);
      csound::QueryGlobalPointer(csound, "fluid_soundfonts", soundfonts);
      if (!fluid_synths_mutex || !soundfonts) {
        return csound->InitError(csound, "%s",
                                 Str("error accessing fluid SoundFont registry\n"));
      }
      {
        csound::LockGuard synthsGuard(csound, fluid_synths_mutex);
        auto &engineSoundFonts = (*soundfonts)[fluidSynth];
        auto loadedSoundFont = engineSoundFonts.find(filepath);
        if (loadedSoundFont != engineSoundFonts.end()) {
          soundFontId = loadedSoundFont->second;
          log(csound, "Reusing SoundFont: %s.\n", filepath);
        } else {
          log(csound, "Loading SoundFont: %s.\n", filepath);
          soundFontId = fluid_synth_sfload(fluidSynth, filepath, 0);
          if (soundFontId >= 0) {
            engineSoundFonts.emplace(filepath, soundFontId);
          }
        }
      }
      log(csound, "fluidSynth: 0x%p  soundFontId: %d.\n", fluidSynth,
          soundFontId);
    }
    *iInstrumentNumber = (cs_float)soundFontId;
    if (UNLIKELY(soundFontId < 0)) {
      return csound->InitError(csound, Str("fluid: unable to load %s"),
                               filename);
    }
    csound->NotifyFileOpened(csound, filepath, CSFTYPE_SOUNDFONT, 0, 0);
    if (soundFontId < 0) {
      result = NOTOK;
    } else if (listPresets) {
      fluid_sfont_t *fluidSoundfont =
          fluid_synth_get_sfont_by_id(fluidSynth, soundFontId);
#if FLUIDSYNTH_VERSION_MAJOR < 2
      fluid_preset_t fluidPreset;
      fluidSoundfont->iteration_start(fluidSoundfont);
      const OPARMS *oparms = csound->GetOParms(csound);
      if (oparms->msglevel & 0x7)
        while (fluidSoundfont->iteration_next(fluidSoundfont, &fluidPreset))
        {
          log(csound, "SoundFont: %3d  Bank: %3d  Preset: %3d  %s\n",
              soundFontId, fluidPreset.get_banknum(&fluidPreset),
              fluidPreset.get_num(&fluidPreset),
              fluidPreset.get_name(&fluidPreset));
#else
      fluid_preset_t *fluidPreset;
      fluid_sfont_iteration_start(fluidSoundfont);
      const OPARMS *oparms = csound->GetOParms(csound);
      if (oparms->msglevel & 0x7)
        while ((fluidPreset = fluid_sfont_iteration_next(fluidSoundfont)))
        {
          log(csound, "SoundFont: %3d  Bank: %3d  Preset: %3d  %s\n",
              soundFontId, fluid_preset_get_banknum(fluidPreset),
              fluid_preset_get_num(fluidPreset),
              fluid_preset_get_name(fluidPreset));
#endif
        }
    }
    return result;
  }
};

#include "arrays.h"

//Rory Walsh 2018
class FluidInfo : public csound::OpcodeBase<FluidInfo> {
  // Outputs.
  ARRAYDAT *outArr;
  // Inputs.
  cs_float *iFluidSynth;
  // State.
  fluid_synth_t *fluidSynth;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
      std::vector<std::string> programs;
      char *program;
      mutex = csound->Create_Mutex(0);
      csound::LockGuard guard(csound, mutex);
      int32_t result = OK;
      toa(iFluidSynth, fluidSynth);
      fluid_sfont_t *fluidSoundfont = fluid_synth_get_sfont(fluidSynth, 0);
#if FLUIDSYNTH_VERSION_MAJOR < 2
      fluid_preset_t fluidPreset;
      fluidSoundfont->iteration_start(fluidSoundfont);
      const OPARMS *oparms = csound->GetOParms(csound);
      if (oparms->msglevel & 0x7)
        while (fluidSoundfont->iteration_next(fluidSoundfont, &fluidPreset))
        {
            std::stringstream ss;
            ss << "Bank: " << fluidPreset.get_banknum(&fluidPreset) <<
              " Preset: " << fluidPreset.get_num(&fluidPreset) <<
                " Name: " << fluidPreset.get_name(&fluidPreset);
#else
      fluid_preset_t *fluidPreset;
      fluid_sfont_iteration_start(fluidSoundfont);
      const OPARMS *oparms = csound->GetOParms(csound);
      if (oparms->msglevel & 0x7)
        while ((fluidPreset = fluid_sfont_iteration_next(fluidSoundfont)))
        {
            std::stringstream ss;
            ss << "Bank: " << fluid_preset_get_banknum(fluidPreset) <<
              " Preset: " << fluid_preset_get_num(fluidPreset) <<
                " Name: " << fluid_preset_get_name(fluidPreset);
#endif
          programs.push_back(ss.str());
        }

      tabinit(csound, outArr, (int32_t)programs.size(), opds.insdshead);

    for (unsigned int i = 0; i < programs.size(); i++) {
        program = &programs[i][0u];
        STRINGDAT *element = csound_string_array_element(outArr, i);
        element->size = strlen(program) + 1;
        element->data = csound->Strdup(csound, program);
        element->refcount = 0;
    }

    programs.clear();

    return result;
  }
};

class FluidProgramSelect : public csound::OpcodeBase<FluidProgramSelect>
{
        // Inputs.
        cs_float *iFluidSynth;
        cs_float *iChannelNumber;
        cs_float *iInstrumentNumber;
        cs_float *iBankNumber;
        cs_float *iPresetNumber;
        // State.
        fluid_synth_t *fluidSynth;
        int32_t channel;
        uint32_t instrument;
        uint32_t bank;
        uint32_t preset;
        void *mutex;
public:
        int32_t init(CSOUND *csound)
        {
                mutex = csound->Create_Mutex(0);
                csound::LockGuard guard(csound, mutex);
                toa(iFluidSynth, fluidSynth);
                channel = (int32_t) *iChannelNumber;
                instrument = (uint32_t) *iInstrumentNumber;
                bank = (uint32_t) *iBankNumber;
                preset = (uint32_t) *iPresetNumber;
                fluid_synth_program_select(fluidSynth,
                                           channel,
                                           instrument,
                                           bank,
                                           preset);
                return OK;
        }
};

class FluidCCI : public csound::OpcodeBase<FluidCCI> {
  // Inputs.
  cs_float *iFluidSynth;
  cs_float *iChannelNumber;
  cs_float *iControllerNumber;
  cs_float *kVal;
  // State.
  fluid_synth_t *fluidSynth;
  int32_t channel;
  int32_t controller;
  int32_t value;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
    mutex = csound->Create_Mutex(0);
    csound::LockGuard guard(csound, mutex);
    toa(iFluidSynth, fluidSynth);
    channel = (int32_t)*iChannelNumber;
    controller = (int32_t)*iControllerNumber;
    value = (int32_t)*kVal;
    fluid_synth_cc(fluidSynth, channel, controller, value);
    return OK;
  }
};

class FluidCCK : public csound::OpcodeBase<FluidCCK> {
  // Inputs.
  cs_float *iFluidSynth;
  cs_float *iChannelNumber;
  cs_float *iControllerNumber;
  cs_float *kVal;
  // State.
  fluid_synth_t *fluidSynth;
  int32_t channel;
  int32_t controller;
  int32_t value;
  int32_t priorValue;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
    mutex = csound->Create_Mutex(0);
    csound::LockGuard guard(csound, mutex);
    toa(iFluidSynth, fluidSynth);
    priorValue = -1;
    return OK;
  }
  int32_t kontrol(CSOUND *csound) {
    csound::LockGuard guard(csound, mutex);
    value = (int32_t)*kVal;
    if (value != priorValue) {
      priorValue = value;
      channel = (int32_t)*iChannelNumber;
      controller = (int32_t)*iControllerNumber;
      fluid_synth_cc(fluidSynth, channel, controller, value);
    }
    return OK;
  }
};

class FluidNote : public csound::OpcodeNoteoffBase<FluidNote> {
  // Inputs.
  cs_float *iFluidSynth;
  cs_float *iChannelNumber;
  cs_float *iMidiKeyNumber;
  cs_float *iVelocity;
  // State.
  fluid_synth_t *fluidSynth;
  int32_t channel;
  int32_t key;
  int32_t velocity;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
    mutex = csound->Create_Mutex(0);
    csound::LockGuard guard(csound, mutex);
    toa(iFluidSynth, fluidSynth);
    channel = (int32_t)*iChannelNumber;
    key = (int32_t)*iMidiKeyNumber;
    velocity = (int32_t)*iVelocity;
    fluid_synth_noteon(fluidSynth, channel, key, velocity);
    return OK;
  }
  int32_t noteoff(CSOUND *csound) {
    csound::LockGuard guard(csound, mutex);
    fluid_synth_noteoff(fluidSynth, channel, key);
    return OK;
  }
};

class FluidOut : public csound::OpcodeBase<FluidOut> {
  // Outputs.
  cs_float *aLeftOut;
  cs_float *aRightOut;
  // Inputs.
  cs_float *iFluidSynth;
  // State.
  fluid_synth_t *fluidSynth;
  float leftSample;
  float rightSample;
  int32_t frame;
  int32_t ksmps;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
    mutex = csound->Create_Mutex(0);
    csound::LockGuard guard(csound, mutex);
    toa(iFluidSynth, fluidSynth);
    ksmps = opds.insdshead->ksmps;
    return OK;
  }
  int32_t audio(CSOUND *csound) {
    csound::LockGuard guard(csound, mutex);
    uint32_t offset = opds.insdshead->ksmps_offset;
    uint32_t early = opds.insdshead->ksmps_no_end;
    if (UNLIKELY(offset)) {
      memset(aLeftOut, '\0', offset * sizeof(cs_float));
      memset(aRightOut, '\0', offset * sizeof(cs_float));
    }
    if (UNLIKELY(early)) {
      ksmps -= early;
      memset(&aLeftOut[ksmps], '\0', early * sizeof(cs_float));
      memset(&aRightOut[ksmps], '\0', early * sizeof(cs_float));
    }
    for (frame = offset; frame < ksmps; frame++) {
      leftSample = 0.0f;
      rightSample = 0.0f;
      fluid_synth_write_float(fluidSynth, 1, &leftSample, 0, 1, &rightSample, 0,
                              1);
      aLeftOut[frame] = leftSample /* * csound->e0dbfs */;
      aRightOut[frame] = rightSample /* * csound->e0dbfs */;
    }
    return OK;
  }
};

class FluidAllOut : public csound::OpcodeBase<FluidAllOut> {
  // Outputs.
  cs_float *aLeftOut;
  cs_float *aRightOut;
  // State.
  float leftSample;
  float rightSample;
  int32_t frame;
  int32_t ksmps;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
    mutex = csound->Create_Mutex(0);
    csound::LockGuard guard(csound, mutex);
    ksmps = opds.insdshead->ksmps;
    return OK;
  }
  int32_t audio(CSOUND *csound) {
    csound::LockGuard guard(csound, mutex);
    uint32_t offset = opds.insdshead->ksmps_offset;
    uint32_t early = opds.insdshead->ksmps_no_end;
    if (UNLIKELY(offset)) {
      memset(aLeftOut, '\0', offset * sizeof(cs_float));
      memset(aRightOut, '\0', offset * sizeof(cs_float));
    }
    if (UNLIKELY(early)) {
      ksmps -= early;
      memset(&aLeftOut[ksmps], '\0', early * sizeof(cs_float));
      memset(&aRightOut[ksmps], '\0', early * sizeof(cs_float));
    }
    std::vector<fluid_synth_t *> *fluid_synths = 0;
    csound::QueryGlobalPointer(csound, "fluid_synths", fluid_synths);
    void *fluid_synths_mutex = 0;
    csound::QueryGlobalPointer(csound, "fluid_synths_mutex",
                               fluid_synths_mutex);
    csound::LockGuard synthsGuard(csound, fluid_synths_mutex);
    for (frame = offset; frame < ksmps; frame++) {
      aLeftOut[frame] = FL(0.0);
      aRightOut[frame] = FL(0.0);
      for (size_t i = 0, n = fluid_synths->size(); i < n; i++) {
        fluid_synth_t *fluidSynth = (*fluid_synths)[i];
        leftSample = FL(0.0);
        rightSample = FL(0.0);
        fluid_synth_write_float(fluidSynth, 1, &leftSample, 0, 1, &rightSample,
                                0, 1);
        aLeftOut[frame] += (cs_float)leftSample /* * csound->e0dbfs */;
        aRightOut[frame] += (cs_float)rightSample /* * csound->e0dbfs */;
      }
    }
    return OK;
  }
};

class FluidControl : public csound::OpcodeBase<FluidControl> {
  // Inputs.
  cs_float *iFluidSynth;
  cs_float *kMidiStatus;
  cs_float *kMidiChannel;
  cs_float *kMidiData1;
  cs_float *kMidiData2;
  cs_float *imsgs;
  // State.
  fluid_synth_t *fluidSynth;
  int32_t midiStatus;
  int32_t midiChannel;
  int32_t midiData1;
  int32_t midiData2;
  int32_t priorMidiStatus;
  int32_t priorMidiChannel;
  int32_t priorMidiData1;
  int32_t priorMidiData2;
  int32_t printMsgs;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
    mutex = csound->Create_Mutex(0);
    csound::LockGuard guard(csound, mutex);
    toa(iFluidSynth, fluidSynth);
    priorMidiStatus = -1;
    priorMidiChannel = -1;
    priorMidiData1 = -1;
    priorMidiData2 = -1;
    printMsgs = (*imsgs == 0 ? 0 : 1);
    return OK;
  }
  int32_t kontrol(CSOUND *csound) {
   csound::LockGuard guard(csound, mutex);
    midiStatus = 0xF0 & (int32_t)*kMidiStatus;
    midiChannel = (int32_t)*kMidiChannel;
    midiData1 = (int32_t)*kMidiData1;
    midiData2 = (int32_t)*kMidiData2;
    int32_t result = -1;

    if (midiData2 != priorMidiData2 || midiData1 != priorMidiData1 ||
        midiChannel != priorMidiChannel || midiStatus != priorMidiStatus) {
      switch (midiStatus) {
      case (int32_t)0x80:
      noteOff:
        result = fluid_synth_noteoff(fluidSynth, midiChannel, midiData1);
        if (printMsgs)
          csound->Message(csound, Str("result: %d\n Note off: c:%3d k:%3d\n"),
                          result, midiChannel, midiData1);
        break;
      case (int32_t)0x90:
        if (!midiData2) {
          goto noteOff;
        }
        result =
            fluid_synth_noteon(fluidSynth, midiChannel, midiData1, midiData2);
        if (printMsgs)
          log(csound, "result: %d\nNote on: c:%3d k:%3d v:%3d\n", result,
              midiChannel, midiData1, midiData2);
        break;
      case (int32_t)0xA0:
        if (printMsgs)
          log(csound, "Key pressure (not handled): "
                      "c:%3d k:%3d v:%3d\n",
              midiChannel, midiData1, midiData2);
        break;
      case (int32_t)0xB0:
        result = fluid_synth_cc(fluidSynth, midiChannel, midiData1, midiData2);
        if (printMsgs)
          log(csound, "Result: %d Control change: c:%3d c:%3d v:%3d\n", result,
              midiChannel, midiData1, midiData2);
        break;
      case (int32_t)0xC0:
        result = fluid_synth_program_change(fluidSynth, midiChannel, midiData1);
        if (printMsgs)
          log(csound, "Result: %d Program change: c:%3d p:%3d\n", result,
              midiChannel, midiData1);
        break;
      case (int32_t)0xD0:
        if (printMsgs)
          log(csound, "After touch (not handled): c:%3d v:%3d\n", midiChannel,
              midiData1);
        break;
      case (int32_t)0xE0: {
        int32_t pbVal = midiData1 + (midiData2 << 7);
        fluid_synth_pitch_bend(fluidSynth, midiChannel, pbVal);
        if (printMsgs)
          log(csound, "Result: %d, Pitch bend:     c:%d b:%d\n", result,
              midiChannel, pbVal);
      } break;
      case (int32_t)0xF0:
        if (printMsgs)
          log(csound, "System exclusive (not handled): "
                      "c:%3d v1:%3d v2:%3d\n",
              midiChannel, midiData1, midiData2);
        break;
      }
      priorMidiStatus = midiStatus;
      priorMidiChannel = midiChannel;
      priorMidiData1 = midiData1;
      priorMidiData2 = midiData2;
    }
    return OK;
  }
};

class FluidSetInterpMethod : public csound::OpcodeBase<FluidSetInterpMethod> {
  // Inputs.
  cs_float *iFluidSynth;
  cs_float *iChannelNumber;
  cs_float *iInterpMethod;
  // State.
  fluid_synth_t *fluidSynth;
  int32_t channel;
  int32_t interpolationMethod;
  void *mutex;

public:
  int32_t init(CSOUND *csound) {
    mutex = csound->Create_Mutex(0);
    csound::LockGuard guard(csound, mutex);
    toa(iFluidSynth, fluidSynth);
    channel = (int32_t)*iChannelNumber;
    interpolationMethod = (int32_t)*iInterpMethod;
    if (UNLIKELY(interpolationMethod != 0 && interpolationMethod != 1 &&
                 interpolationMethod != 4 && interpolationMethod != 7)) {
      return csound->InitError(csound, "%s",
                               Str("Illegal Interpolation Method: Must be "
                                   "either 0, 1, 4, or 7.\n"));
    } else {
      fluid_synth_set_interp_method(fluidSynth, channel, interpolationMethod);
    }
    return OK;
  }
};

static OENTRY localops[] = {
    {(char *)"fluidEngine", sizeof(FluidEngine), 0, (char *)"i", (char *)"ppoo",
     (SUBR)&FluidEngine::init_, (SUBR)0, (SUBR)0},
    {(char *)"fluidEngineReuse", sizeof(FluidEngineReuse), 0, (char *)"i",
     (char *)"Sppoo", (SUBR)&FluidEngineReuse::init_, (SUBR)0, (SUBR)0},
    {(char *)"fluidLoad", sizeof(FluidLoad), 0, (char *)"i", (char *)"Tio",
     (SUBR)&FluidLoad::init_, (SUBR)0, (SUBR)0},
    {(char *)"fluidInfo", sizeof(FluidInfo), 0, (char *)"S[]", (char *)"i",
     (SUBR)&FluidInfo::init_, (SUBR)0, (SUBR)0},
    {(char *)"fluidProgramSelect", sizeof(FluidProgramSelect), 0, (char *)"",
     (char *)"iiiii", (SUBR)&FluidProgramSelect::init_, (SUBR)0, (SUBR)0},
    {(char *)"fluidCCi", sizeof(FluidCCI), 0, (char *)"", (char *)"iiii",
     (SUBR)&FluidCCI::init_, (SUBR)0, (SUBR)0},
    {(char *)"fluidCCk", sizeof(FluidCCK), 0, (char *)"", (char *)"ikkk",
     (SUBR)&FluidCCK::init_, (SUBR)&FluidCCK::kontrol_, (SUBR)0},
    {(char *)"fluidNote", sizeof(FluidNote), 0, (char *)"", (char *)"iiii",
     (SUBR)&FluidNote::init_, (SUBR)0, (SUBR)&FluidNote::deinit_},
    {(char *)"fluidOut", sizeof(FluidOut), 0, (char *)"aa", (char *)"i",
     (SUBR)&FluidOut::init_, (SUBR)&FluidOut::audio_, (SUBR)0},
    {(char *)"fluidAllOut", sizeof(FluidAllOut), 0, (char *)"aa", (char *)"",
     (SUBR)&FluidAllOut::init_, (SUBR)&FluidAllOut::audio_, (SUBR)0},
    {(char *)"fluidControl", sizeof(FluidControl), 0, (char *)"",
     (char *)"ikkkkp", (SUBR)FluidControl::init_, (SUBR)FluidControl::kontrol_,
     (SUBR)0},
    {(char *)"fluidSetInterpMethod", sizeof(FluidSetInterpMethod), 0,
     (char *)"", (char *)"iii", (SUBR)&FluidSetInterpMethod::init_, (SUBR)0,
     (SUBR)0},
    {NULL, 0, 0, NULL, NULL, (SUBR)0, (SUBR)0, (SUBR)0, NULL, 0}};

PUBLIC int32_t csoundModuleCreate(CSOUND *csound) {
  std::vector<fluid_synth_t *> *fluid_synths =
      new std::vector<fluid_synth_t *>();
  NamedFluidEngines *named_engines = new NamedFluidEngines();
  FluidSoundFonts *soundfonts = new FluidSoundFonts();
  int32_t result = 0;
  result |= csound::CreateGlobalPointer(csound, "fluid_synths", fluid_synths);
  result |= csound::CreateGlobalPointer(csound, "fluid_named_engines",
                                        named_engines);
  result |= csound::CreateGlobalPointer(csound, "fluid_soundfonts", soundfonts);
  void *fluid_synths_mutex = csound->Create_Mutex(0);
  result |= csound::CreateGlobalPointer(csound, "fluid_synths_mutex",
                                        fluid_synths_mutex);
  return result;
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound) {
  // printf("csoundModuleInit: %p\n", csound);
  OENTRY *ep;
  int32_t err = 0;

  for (ep = (OENTRY *)&(localops[0]); ep->opname != NULL; ep++) {
    err |= csound->AppendOpcode(csound, ep->opname, ep->dsblksiz, ep->flags,
                                ep->outypes, ep->intypes,
                                (int32_t (*)(CSOUND *, void *))ep->init,
                                (int32_t (*)(CSOUND *, void *))ep->perf,
                                (int32_t (*)(CSOUND *, void *))ep->deinit);
  }
  return err;
}

/**
 * Called by Csound to de-initialize the opcode
 * just before destroying it.
 */
PUBLIC int32_t csoundModuleDestroy(CSOUND *csound) {
  // printf("csoundModuleDestroy: %p\n", csound);
  void *fluid_synths_mutex = 0;
  csound::QueryGlobalPointer(csound, "fluid_synths_mutex", fluid_synths_mutex);
  if (fluid_synths_mutex) {
    std::vector<fluid_synth_t *> *fluid_synths = 0;
    NamedFluidEngines *named_engines = 0;
    FluidSoundFonts *soundfonts = 0;
    csound::QueryGlobalPointer(csound, "fluid_synths", fluid_synths);
    csound::QueryGlobalPointer(csound, "fluid_named_engines", named_engines);
    csound::QueryGlobalPointer(csound, "fluid_soundfonts", soundfonts);
    csound->LockMutex(fluid_synths_mutex);
    auto fluid_synths_global =
        static_cast<std::vector<fluid_synth_t *> **>(
            csound->QueryGlobalVariableNoCheck(csound, "fluid_synths"));
    auto named_engines_global =
        static_cast<NamedFluidEngines **>(
            csound->QueryGlobalVariableNoCheck(csound, "fluid_named_engines"));
    auto soundfonts_global =
        static_cast<FluidSoundFonts **>(
            csound->QueryGlobalVariableNoCheck(csound, "fluid_soundfonts"));
    if (fluid_synths_global) {
      *fluid_synths_global = nullptr;
    }
    if (named_engines_global) {
      *named_engines_global = nullptr;
    }
    if (soundfonts_global) {
      *soundfonts_global = nullptr;
    }
    if (fluid_synths) {
      std::lock_guard<std::mutex> idsLock(fluidsynths_for_ids_mutex());
      for (size_t i = 0, n = fluid_synths->size(); i < n; i++) {
        fluid_synth_t *fluidSynth = (*fluid_synths)[i];
        // printf("deleting engine: %p\n", fluidSynth);
        auto &instances = fluidsynths_for_ids();
        for (auto &instance : instances) {
          if (instance == fluidSynth) {
            instance = nullptr;
          }
        }
        fluid_settings_t *fluidSettings = fluid_synth_get_settings(fluidSynth);
        delete_fluid_synth(fluidSynth);
        delete_fluid_settings(fluidSettings);
      }
      fluid_synths->clear();
      delete fluid_synths;
      fluid_synths = 0;
    }
    delete named_engines;
    delete soundfonts;
    csound->UnlockMutex(fluid_synths_mutex);
    csound->DestroyMutex(fluid_synths_mutex);
    fluid_synths_mutex = 0;
  }
  return 0;
}

PUBLIC int32_t csoundModuleInfo(void) {
  return CSOUND_MODULE_INFO;
}
