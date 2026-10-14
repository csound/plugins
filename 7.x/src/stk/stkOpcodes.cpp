/*
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
*/
/*
 * CSOUND 7 OPCODES FOR PERRY COOK'S SYNTHESIS TOOLKIT IN C++ (STK) INSTRUMENT
 *
 * This code is copyright (C) 2005 by Michael Gogins. It is licensed under the
 * same terms as the Synthesis Toolkit in C++ by Perry R. Cook and Gary P.
 * Scavone.
 *
 * The STK sources are bundled in this directory (include/, src/ and
 * rawwaves/) so that a patched copy is always used, independent of any system
 * or package-manager STK installation. STK_EMBED_RAWWAVES stages the bundled
 * samples at module initialization; otherwise the rawwaves path defaults to
 * the installed location (DEFAULT_RAWWAVE_PATH). The RAWWAVE_PATH environment
 * variable overrides either default.
 *
 * All these opcodes are named "STK" + the STK classname,
 * e.g. "STKBowed" for the Bowed instrument.
 *
 * All the STK opcodes have the same signature:
 *
 * aout STKName ifrequency igain {kcontroller0, kvalue1,...,kcontroller7,
 * kvalue7}
 *
 * They take a frequency in Hertz and a gain parameter in the range [0, 1],
 * plus up to eight optional krate controller-value pairs, and return an arate
 * signal that should be more or less in the range [-1, +1].
 * See the STK class documentation to determine the controller numbers
 * used by each instrument.
 */

/* Standard library headers must be included before any STK or Csound headers so
   that STK's and Csound's "PI" do not clash with the standard library. */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#ifdef STK_EMBED_RAWWAVES
#include <cerrno>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
#include "stk_rawwaves.inc"
#endif

#include "BandedWG.h"
#include "BeeThree.h"
#include "BlowBotl.h"
#include "BlowHole.h"
#include "Bowed.h"
#include "Brass.h"
#include "Clarinet.h"
#include "Drummer.h"
#include "FMVoices.h"
#include "Flute.h"
#include "HevyMetl.h"
#include "Mandolin.h"
#include "Stk.h"
#include "ModalBar.h"
#include "Moog.h"
#include "PercFlut.h"
#include "Plucked.h"
#include "Resonate.h"
#include "Rhodey.h"
#include "Saxofony.h"
#include "Shakers.h"
#include "Simple.h"
#include "Sitar.h"
#include "StifKarp.h"
#include "TubeBell.h"
#include "VoicForm.h"
#include "Whistle.h"
#include "Wurley.h"

extern "C" {
#include <csdl.h>
}

#include "OpcodeBase.hpp"

using namespace stk;
using namespace csound;
using namespace std;

template <typename T>
class STKInstrumentAdapter : public OpcodeBase<STKInstrumentAdapter<T>> {
public:
  // Outputs.
  cs_float *aoutput;
  // Inputs.
  cs_float *ifrequency;
  cs_float *igain;
  cs_float *kcontroller0;
  cs_float *kvalue0;
  cs_float *kcontroller1;
  cs_float *kvalue1;
  cs_float *kcontroller2;
  cs_float *kvalue2;
  cs_float *kcontroller3;
  cs_float *kvalue3;
  cs_float *kcontroller4;
  cs_float *kvalue4;
  cs_float *kcontroller5;
  cs_float *kvalue5;
  cs_float *kcontroller6;
  cs_float *kvalue6;
  cs_float *kcontroller7;
  cs_float *kvalue7;
  // State.
  T *instrument;
  bool released;
  cs_float oldkcontroller0;
  cs_float oldkvalue0;
  cs_float oldkcontroller1;
  cs_float oldkvalue1;
  cs_float oldkcontroller2;
  cs_float oldkvalue2;
  cs_float oldkcontroller3;
  cs_float oldkvalue3;
  cs_float oldkcontroller4;
  cs_float oldkvalue4;
  cs_float oldkcontroller5;
  cs_float oldkvalue5;
  cs_float oldkcontroller6;
  cs_float oldkvalue6;
  cs_float oldkcontroller7;
  cs_float oldkvalue7;
  STKInstrumentAdapter() : instrument(0) {}
  int32_t init(CSOUND *csound) {
    if (!instrument) {
      try {
        instrument = new T();
      } catch (StkError &e) {
        return csound->InitError(csound, "STK: %s", e.getMessage().c_str());
      }
    }
    instrument->noteOn(*ifrequency, *igain);
    released = false;
    oldkcontroller0 = -1.0;
    oldkvalue0 = -1.0;
    oldkcontroller1 = -1.0;
    oldkvalue1 = -1.0;
    oldkcontroller2 = -1.0;
    oldkvalue2 = -1.0;
    oldkcontroller3 = -1.0;
    oldkvalue3 = -1.0;
    oldkcontroller4 = -1.0;
    oldkvalue4 = -1.0;
    oldkcontroller5 = -1.0;
    oldkvalue5 = -1.0;
    oldkcontroller6 = -1.0;
    oldkvalue6 = -1.0;
    oldkcontroller7 = -1.0;
    oldkvalue7 = -1.0;
    return OK;
  }
  int32_t kontrol(CSOUND *csound) {
    if (!released) {
      if (*kcontroller0 != oldkcontroller0 || *kvalue0 != oldkvalue0) {
        instrument->controlChange(static_cast<int32_t>(*kcontroller0), *kvalue0);
        oldkcontroller0 = *kcontroller0;
        oldkvalue0 = *kvalue0;
      }
      if (*kcontroller1 != oldkcontroller1 || *kvalue1 != oldkvalue1) {
        instrument->controlChange(static_cast<int32_t>(*kcontroller1), *kvalue1);
        oldkcontroller1 = *kcontroller1;
        oldkvalue1 = *kvalue1;
      }
      if (*kcontroller2 != oldkcontroller2 || *kvalue2 != oldkvalue2) {
        instrument->controlChange(static_cast<int32_t>(*kcontroller2), *kvalue2);
        oldkcontroller2 = *kcontroller2;
        oldkvalue2 = *kvalue2;
      }
      if (*kcontroller3 != oldkcontroller3 || *kvalue3 != oldkvalue3) {
        instrument->controlChange(static_cast<int32_t>(*kcontroller3), *kvalue3);
        oldkcontroller3 = *kcontroller3;
        oldkvalue3 = *kvalue3;
      }
      if (*kcontroller4 != oldkcontroller4 || *kvalue4 != oldkvalue4) {
        instrument->controlChange(static_cast<int32_t>(*kcontroller4), *kvalue4);
        oldkcontroller4 = *kcontroller4;
        oldkvalue4 = *kvalue4;
      }
      if (*kcontroller5 != oldkcontroller5 || *kvalue5 != oldkvalue5) {
        instrument->controlChange(static_cast<int32_t>(*kcontroller5), *kvalue5);
        oldkcontroller5 = *kcontroller5;
        oldkvalue5 = *kvalue5;
      }
      if (*kcontroller6 != oldkcontroller6 || *kvalue6 != oldkvalue6) {
        instrument->controlChange(static_cast<int32_t>(*kcontroller6), *kvalue6);
        oldkcontroller6 = *kcontroller6;
        oldkvalue6 = *kvalue6;
      }
      if (*kcontroller7 != oldkcontroller7 || *kvalue7 != oldkvalue7) {
        instrument->controlChange(static_cast<int32_t>(*kcontroller7), *kvalue7);
        oldkcontroller7 = *kcontroller7;
        oldkvalue7 = *kvalue7;
      }
    }
    uint32_t offset = this->kperiodOffset();
    uint32_t end = this->kperiodEnd();
    uint32_t nsmps = this->ksmps();
    for (uint32_t i = 0; i < offset; i++) {
      aoutput[i] = 0;
    }
    if (!released) {
      for (uint32_t i = offset; i < end; i++) {
        aoutput[i] = instrument->tick();
      }
    } else {
      for (uint32_t i = offset; i < end; i++) {
        aoutput[i] = 0;
      }
    }
    for (uint32_t i = end; i < nsmps; i++) {
      aoutput[i] = 0;
    }
    return OK;
  }
  int32_t deinit(CSOUND *csound) {
    if (instrument) {
      delete instrument;
      instrument = 0;
    }
    return OK;
  }
  static int32_t deinit_(CSOUND *csound, void *opcode) {
    return reinterpret_cast<STKInstrumentAdapter<T> *>(opcode)->deinit(csound);
  }
};

template <typename T>
class STKInstrumentAdapter1 : public OpcodeBase<STKInstrumentAdapter1<T>> {
public:
  // Outputs.
  cs_float *aoutput;
  // Inputs.
  cs_float *ifrequency;
  cs_float *igain;
  cs_float *kcontroller0;
  cs_float *kvalue0;
  cs_float *kcontroller1;
  cs_float *kvalue1;
  cs_float *kcontroller2;
  cs_float *kvalue2;
  cs_float *kcontroller3;
  cs_float *kvalue3;
  cs_float *kcontroller4;
  cs_float *kvalue4;
  cs_float *kcontroller5;
  cs_float *kvalue5;
  cs_float *kcontroller6;
  cs_float *kvalue6;
  cs_float *kcontroller7;
  cs_float *kvalue7;
  // State.
  T *instrument;
  bool released;
  cs_float oldkcontroller0;
  cs_float oldkvalue0;
  cs_float oldkcontroller1;
  cs_float oldkvalue1;
  cs_float oldkcontroller2;
  cs_float oldkvalue2;
  cs_float oldkcontroller3;
  cs_float oldkvalue3;
  cs_float oldkcontroller4;
  cs_float oldkvalue4;
  cs_float oldkcontroller5;
  cs_float oldkvalue5;
  cs_float oldkcontroller6;
  cs_float oldkvalue6;
  cs_float oldkcontroller7;
  cs_float oldkvalue7;
  STKInstrumentAdapter1() : instrument(0) {}
  int32_t init(CSOUND *csound) {
    if (!instrument) {
      try {
        instrument = new T((StkFloat)10.0);
      } catch (StkError &e) {
        return csound->InitError(csound, "STK: %s", e.getMessage().c_str());
      }
    }
    instrument->noteOn(*ifrequency, *igain);
    released = false;
    oldkcontroller0 = -1.0;
    oldkvalue0 = -1.0;
    oldkcontroller1 = -1.0;
    oldkvalue1 = -1.0;
    oldkcontroller2 = -1.0;
    oldkvalue2 = -1.0;
    oldkcontroller3 = -1.0;
    oldkvalue3 = -1.0;
    oldkcontroller4 = -1.0;
    oldkvalue4 = -1.0;
    oldkcontroller5 = -1.0;
    oldkvalue5 = -1.0;
    oldkcontroller6 = -1.0;
    oldkvalue6 = -1.0;
    oldkcontroller7 = -1.0;
    oldkvalue7 = -1.0;
    return OK;
  }
  int32_t kontrol(CSOUND *csound) {
    if (!released) {
      if (*kcontroller0 != oldkcontroller0 || *kvalue0 != oldkvalue0) {
        instrument->controlChange(static_cast<int32_t>(*kcontroller0), *kvalue0);
        oldkcontroller0 = *kcontroller0;
        oldkvalue0 = *kvalue0;
      }
      if (*kcontroller1 != oldkcontroller1 || *kvalue1 != oldkvalue1) {
        instrument->controlChange(static_cast<int32_t>(*kcontroller1), *kvalue1);
        oldkcontroller1 = *kcontroller1;
        oldkvalue1 = *kvalue1;
      }
      if (*kcontroller2 != oldkcontroller2 || *kvalue2 != oldkvalue2) {
        instrument->controlChange(static_cast<int32_t>(*kcontroller2), *kvalue2);
        oldkcontroller2 = *kcontroller2;
        oldkvalue2 = *kvalue2;
      }
      if (*kcontroller3 != oldkcontroller3 || *kvalue3 != oldkvalue3) {
        instrument->controlChange(static_cast<int32_t>(*kcontroller3), *kvalue3);
        oldkcontroller3 = *kcontroller3;
        oldkvalue3 = *kvalue3;
      }
      if (*kcontroller4 != oldkcontroller4 || *kvalue4 != oldkvalue4) {
        instrument->controlChange(static_cast<int32_t>(*kcontroller4), *kvalue4);
        oldkcontroller4 = *kcontroller4;
        oldkvalue4 = *kvalue4;
      }
      if (*kcontroller5 != oldkcontroller5 || *kvalue5 != oldkvalue5) {
        instrument->controlChange(static_cast<int32_t>(*kcontroller5), *kvalue5);
        oldkcontroller5 = *kcontroller5;
        oldkvalue5 = *kvalue5;
      }
      if (*kcontroller6 != oldkcontroller6 || *kvalue6 != oldkvalue6) {
        instrument->controlChange(static_cast<int32_t>(*kcontroller6), *kvalue6);
        oldkcontroller6 = *kcontroller6;
        oldkvalue6 = *kvalue6;
      }
      if (*kcontroller7 != oldkcontroller7 || *kvalue7 != oldkvalue7) {
        instrument->controlChange(static_cast<int32_t>(*kcontroller7), *kvalue7);
        oldkcontroller7 = *kcontroller7;
        oldkvalue7 = *kvalue7;
      }
    }
    uint32_t offset = this->kperiodOffset();
    uint32_t end = this->kperiodEnd();
    uint32_t nsmps = this->ksmps();
    for (uint32_t i = 0; i < offset; i++) {
      aoutput[i] = 0;
    }
    if (!released) {
      for (uint32_t i = offset; i < end; i++) {
        aoutput[i] = instrument->tick();
      }
    } else {
      for (uint32_t i = offset; i < end; i++) {
        aoutput[i] = 0;
      }
    }
    for (uint32_t i = end; i < nsmps; i++) {
      aoutput[i] = 0;
    }
    return OK;
  }
  int32_t deinit(CSOUND *csound) {
    if (instrument) {
      delete instrument;
      instrument = 0;
    }
    return OK;
  }
  static int32_t deinit_(CSOUND *csound, void *opcode) {
    return reinterpret_cast<STKInstrumentAdapter1<T> *>(opcode)->deinit(csound);
  }
};

extern "C" {
OENTRY oentries[] = {
    {
        (char *)"STKBandedWG", sizeof(STKInstrumentAdapter<BandedWG>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<BandedWG>::init_,
        (SUBR)STKInstrumentAdapter<BandedWG>::kontrol_,
        (SUBR)STKInstrumentAdapter<BandedWG>::deinit_, 0, 0,
    },
    {
        (char *)"STKBeeThree", sizeof(STKInstrumentAdapter<BeeThree>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<BeeThree>::init_,
        (SUBR)STKInstrumentAdapter<BeeThree>::kontrol_,
        (SUBR)STKInstrumentAdapter<BeeThree>::deinit_, 0, 0,
    },
    {
        (char *)"STKBlowBotl", sizeof(STKInstrumentAdapter<BlowBotl>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<BlowBotl>::init_,
        (SUBR)STKInstrumentAdapter<BlowBotl>::kontrol_,
        (SUBR)STKInstrumentAdapter<BlowBotl>::deinit_, 0, 0,
    },
    {
        (char *)"STKBlowHole", sizeof(STKInstrumentAdapter1<BlowHole>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter1<BlowHole>::init_,
        (SUBR)STKInstrumentAdapter1<BlowHole>::kontrol_,
        (SUBR)STKInstrumentAdapter1<BlowHole>::deinit_, 0, 0,
    },
    {
        (char *)"STKBowed", sizeof(STKInstrumentAdapter1<Bowed>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter1<Bowed>::init_,
        (SUBR)STKInstrumentAdapter1<Bowed>::kontrol_,
        (SUBR)STKInstrumentAdapter1<Bowed>::deinit_, 0, 0,
    },
    {
        (char *)"STKBrass", sizeof(STKInstrumentAdapter1<Brass>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter1<Brass>::init_,
        (SUBR)STKInstrumentAdapter1<Brass>::kontrol_,
        (SUBR)STKInstrumentAdapter1<Brass>::deinit_, 0, 0,
    },
    {
        (char *)"STKClarinet", sizeof(STKInstrumentAdapter1<Clarinet>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter1<Clarinet>::init_,
        (SUBR)STKInstrumentAdapter1<Clarinet>::kontrol_,
        (SUBR)STKInstrumentAdapter1<Clarinet>::deinit_, 0, 0,
    },
    {
        (char *)"STKDrummer", sizeof(STKInstrumentAdapter<Drummer>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<Drummer>::init_,
        (SUBR)STKInstrumentAdapter<Drummer>::kontrol_,
        (SUBR)STKInstrumentAdapter<Drummer>::deinit_, 0, 0,
    },
    {
        (char *)"STKFlute", sizeof(STKInstrumentAdapter1<Flute>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter1<Flute>::init_,
        (SUBR)STKInstrumentAdapter1<Flute>::kontrol_,
        (SUBR)STKInstrumentAdapter1<Flute>::deinit_, 0, 0,
    },
    {
        (char *)"STKFMVoices", sizeof(STKInstrumentAdapter<FMVoices>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<FMVoices>::init_,
        (SUBR)STKInstrumentAdapter<FMVoices>::kontrol_,
        (SUBR)STKInstrumentAdapter<FMVoices>::deinit_, 0, 0,
    },
    {
        (char *)"STKHevyMetl", sizeof(STKInstrumentAdapter<HevyMetl>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<HevyMetl>::init_,
        (SUBR)STKInstrumentAdapter<HevyMetl>::kontrol_,
        (SUBR)STKInstrumentAdapter<HevyMetl>::deinit_, 0, 0,
    },
    {
        (char *)"STKMandolin", sizeof(STKInstrumentAdapter1<Mandolin>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter1<Mandolin>::init_,
        (SUBR)STKInstrumentAdapter1<Mandolin>::kontrol_,
        (SUBR)STKInstrumentAdapter1<Mandolin>::deinit_, 0, 0,
    },
    {
        (char *)"STKModalBar", sizeof(STKInstrumentAdapter<ModalBar>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<ModalBar>::init_,
        (SUBR)STKInstrumentAdapter<ModalBar>::kontrol_,
        (SUBR)STKInstrumentAdapter<ModalBar>::deinit_, 0, 0,
    },
    {
        (char *)"STKMoog", sizeof(STKInstrumentAdapter<Moog>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<Moog>::init_,
        (SUBR)STKInstrumentAdapter<Moog>::kontrol_,
        (SUBR)STKInstrumentAdapter<Moog>::deinit_, 0, 0,
    },
    {
        (char *)"STKPercFlut", sizeof(STKInstrumentAdapter<PercFlut>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<PercFlut>::init_,
        (SUBR)STKInstrumentAdapter<PercFlut>::kontrol_,
        (SUBR)STKInstrumentAdapter<PercFlut>::deinit_, 0, 0,
    },
    {
        (char *)"STKPlucked", sizeof(STKInstrumentAdapter1<Plucked>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter1<Plucked>::init_,
        (SUBR)STKInstrumentAdapter1<Plucked>::kontrol_,
        (SUBR)STKInstrumentAdapter1<Plucked>::deinit_, 0, 0,
    },
    {
        (char *)"STKResonate", sizeof(STKInstrumentAdapter<Resonate>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<Resonate>::init_,
        (SUBR)STKInstrumentAdapter<Resonate>::kontrol_,
        (SUBR)STKInstrumentAdapter<Resonate>::deinit_, 0, 0,
    },
    {
        (char *)"STKRhodey", sizeof(STKInstrumentAdapter<Rhodey>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<Rhodey>::init_,
        (SUBR)STKInstrumentAdapter<Rhodey>::kontrol_,
        (SUBR)STKInstrumentAdapter<Rhodey>::deinit_, 0, 0,
    },
    {
        (char *)"STKSaxofony", sizeof(STKInstrumentAdapter1<Saxofony>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter1<Saxofony>::init_,
        (SUBR)STKInstrumentAdapter1<Saxofony>::kontrol_,
        (SUBR)STKInstrumentAdapter1<Saxofony>::deinit_, 0, 0,
    },
    {
        (char *)"STKShakers", sizeof(STKInstrumentAdapter<Shakers>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<Shakers>::init_,
        (SUBR)STKInstrumentAdapter<Shakers>::kontrol_,
        (SUBR)STKInstrumentAdapter<Shakers>::deinit_, 0, 0,
    },
    {
        (char *)"STKSimple", sizeof(STKInstrumentAdapter<Simple>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<Simple>::init_,
        (SUBR)STKInstrumentAdapter<Simple>::kontrol_,
        (SUBR)STKInstrumentAdapter<Simple>::deinit_, 0, 0,
    },
    {
        (char *)"STKSitar", sizeof(STKInstrumentAdapter<Sitar>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<Sitar>::init_,
        (SUBR)STKInstrumentAdapter<Sitar>::kontrol_,
        (SUBR)STKInstrumentAdapter<Sitar>::deinit_, 0, 0,
    },
    {
        (char *)"STKStifKarp", sizeof(STKInstrumentAdapter1<StifKarp>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter1<StifKarp>::init_,
        (SUBR)STKInstrumentAdapter1<StifKarp>::kontrol_,
        (SUBR)STKInstrumentAdapter1<StifKarp>::deinit_, 0, 0,
    },
    {
        (char *)"STKTubeBell", sizeof(STKInstrumentAdapter<TubeBell>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<TubeBell>::init_,
        (SUBR)STKInstrumentAdapter<TubeBell>::kontrol_,
        (SUBR)STKInstrumentAdapter<TubeBell>::deinit_, 0, 0,
    },
    {
        (char *)"STKVoicForm", sizeof(STKInstrumentAdapter<VoicForm>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<VoicForm>::init_,
        (SUBR)STKInstrumentAdapter<VoicForm>::kontrol_,
        (SUBR)STKInstrumentAdapter<VoicForm>::deinit_, 0, 0,
    },
    {
        (char *)"STKWhistle", sizeof(STKInstrumentAdapter<Whistle>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<Whistle>::init_,
        (SUBR)STKInstrumentAdapter<Whistle>::kontrol_,
        (SUBR)STKInstrumentAdapter<Whistle>::deinit_, 0, 0,
    },
    {
        (char *)"STKWurley", sizeof(STKInstrumentAdapter<Wurley>), 0,
        (char *)"a", (char *)"iiJJJJJJJJJJJJJJJJ",
        (SUBR)STKInstrumentAdapter<Wurley>::init_,
        (SUBR)STKInstrumentAdapter<Wurley>::kontrol_,
        (SUBR)STKInstrumentAdapter<Wurley>::deinit_, 0, 0,
    },
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    }};
}

PUBLIC int32_t csoundModuleCreate(CSOUND *csound) { return 0; }

#ifdef STK_EMBED_RAWWAVES
// Keep STK's file reader unchanged by staging the embedded bytes at module init.
static bool writeEmbeddedRawwaves(CSOUND *csound, const char *directory) {
#ifdef _WIN32
  const int result = _mkdir(directory);
#else
  const int result = mkdir(directory, 0700);
#endif
  if (result != 0 && errno != EEXIST) {
    csound->Message(csound, "STK: cannot create raw-wave directory %s\n", directory);
    return false;
  }
  for (const auto &wave : stkRawwaves) {
    const std::string filename = std::string(directory) + "/" + wave.name;
    FILE *file = fopen(filename.c_str(), "wb");
    if (!file) {
      csound->Message(csound, "STK: cannot open embedded raw wave %s\n", filename.c_str());
      return false;
    }
    const bool written = fwrite(wave.bytes, 1, wave.size, file) == wave.size;
    const int closed = fclose(file);
    if (!written || closed != 0) {
      csound->Message(csound, "STK: cannot write embedded raw wave %s\n", filename.c_str());
      return false;
    }
  }
  return true;
}
#endif

PUBLIC int32_t csoundModuleInit(CSOUND *csound) {
  const char *path = csound->GetEnv(csound, "RAWWAVE_PATH");
#ifdef STK_EMBED_RAWWAVES
  if (path == NULL || strlen(path) == 0) {
    path = ".csound-stk-rawwaves";
    if (!writeEmbeddedRawwaves(csound, path))
      return NOTOK;
  }
#endif
#ifdef DEFAULT_RAWWAVE_PATH
  if (path == NULL || strlen(path) == 0)
    path = DEFAULT_RAWWAVE_PATH;
#endif
  if (path == NULL || strlen(path) == 0) {
    if (UNLIKELY(csound->GetDebug(csound)))
      csound->Warning(csound,
                      "%s", Str("STK opcodes not available: define environment "
                                "variable RAWWAVE_PATH\n(points "
                                "to rawwaves directory) to use STK opcodes."));
    return 0;
  } else {
    Stk::setRawwavePath(path);
  }
  int32_t status = 0;
  for (OENTRY *ep = &oentries[0]; ep->opname; ep++) {
    status |= csound->AppendOpcode(
        csound, ep->opname, ep->dsblksiz, ep->flags, ep->outypes, ep->intypes,
        (int32_t (*)(CSOUND *, void *))ep->init,
        (int32_t (*)(CSOUND *, void *))ep->perf,
        (int32_t (*)(CSOUND *, void *))ep->deinit);
  }
  return status;
}

PUBLIC int32_t csoundModuleDestroy(CSOUND *csound) { return 0; }

PUBLIC int32_t csoundModuleInfo(void) { return CSOUND_MODULE_INFO; }
