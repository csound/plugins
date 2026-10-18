/*
  This file is part of Csound.
  Copyright (C) 2005 Michael Gogins.

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
#pragma once

#include "Stk.h"
#include "OpcodeBase.hpp"

namespace csound {

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
    // Constructors size delay lines and filters from STK's shared sample rate.
    // Set it on reinit too, before noteOn() retunes an existing instrument.
    stk::Stk::setSampleRate(static_cast<stk::StkFloat>(this->opds.insdshead->esr));
    if (!instrument) {
      try {
        instrument = new T();
      } catch (stk::StkError &e) {
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
    // Constructors size delay lines and filters from STK's shared sample rate.
    // Set it on reinit too, before noteOn() retunes an existing instrument.
    stk::Stk::setSampleRate(static_cast<stk::StkFloat>(this->opds.insdshead->esr));
    if (!instrument) {
      try {
        instrument = new T((stk::StkFloat)10.0);
      } catch (stk::StkError &e) {
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

} // namespace csound
