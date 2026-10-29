/*  faustgen.cpp

    Copyright (c) Victor Lazzarini, 2013

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA

    Faust Csound opcodes

    These four opcodes allow Faust to be embedded in Csound code:

    faustcompile: compiles a Faust program
    faustaudio: creates a DSP instance from a compiled Faust program
    (any number of instances from a single compiled program are
    allowed)
    faustctl: sets the value of a given control of a Faust DSP instance

    faustgen: compiles and creates a single DSP instance from a Faust program
    (convenient for one-off Faust programs)

*/
/* The C++ standard headers must be included before "csdl.h": the latter pulls
   in Csound's "interlocks.h", which defines short generic macros such as
   _CR and TR that clash with identifiers used inside <chrono> (reached via
   <mutex>) in both libstdc++ and the MSVC STL. */
#include <string>
#include <cstdlib>
#include <mutex>
#include "csdl.h"
#ifndef FAUSTFLOAT
#define FAUSTFLOAT cs_float
#endif
#include "faust/dsp/llvm-dsp.h"
#include "faust/gui/UI.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/stat.h>
#endif

#define MAXARG 40

/* Faust/LLVM factory creation is not thread-safe, so all calls to
   createDSPFactoryFromString are serialised by this lock. It is deliberately
   separate from the registry lock: a compilation can take a long time, and
   holding the registry lock for its duration would stall factory lookups
   (and thus the performance thread) in faustdsp/faustaudio. */
static std::mutex faust_compile_mutex;

/* Protects the shared ::factory and ::dsp registry lists. Held only for the
   short operations that read or modify those lists. */
static std::mutex faust_registry_mutex;

static int dir_exists(const char *p) {
#ifdef _WIN32
  DWORD a = GetFileAttributesA(p);
  return (a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY));
#else
  struct stat st;
  return (stat(p, &st) == 0 && S_ISDIR(st.st_mode));
#endif
}

static void get_plugin_dir(char *out, size_t n) {
  out[0] = '\0';
#ifdef _WIN32
  HMODULE hm = NULL;
  GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                         GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                     (LPCSTR)&get_plugin_dir, &hm);
  GetModuleFileNameA(hm, out, (DWORD)n);
#else
  Dl_info info;
  if (dladdr((void *)&get_plugin_dir, &info) && info.dli_fname)
    snprintf(out, n, "%s", info.dli_fname);
#endif
  char *slash = strrchr(out, '/');
#ifdef _WIN32
  char *bslash = strrchr(out, '\\');
  if (!slash || (bslash && bslash > slash))
    slash = bslash;
#endif
  if (slash)
    *slash = '\0';
}

static void set_faust_lib_path(const char *path) {
#ifdef _WIN32
  _putenv_s("FAUST_LIB_PATH", path);
#else
  setenv("FAUST_LIB_PATH", path, 1);
#endif
}

/* Locate the Faust standard library directory so that programs using
   import("stdfaust.lib") work without a separate Faust installation. A
   user-provided FAUST_LIB_PATH always takes precedence; otherwise we look
   for a "faust" directory next to the plugin, then fall back to the path
   configured at build time. */
static void setup_faust_path(CSOUND *csound) {
  const char *env = getenv("FAUST_LIB_PATH");
  if (env != NULL && env[0] != '\0')
    return;
  char dir[4096], cand[4096];
  get_plugin_dir(dir, sizeof(dir));
  if (dir[0] != '\0') {
    snprintf(cand, sizeof(cand), "%s/faust", dir);
    if (dir_exists(cand)) {
      set_faust_lib_path(cand);
      return;
    }
  }
#ifdef DEFAULT_FAUST_LIB_PATH
  if (dir_exists(DEFAULT_FAUST_LIB_PATH)) {
    set_faust_lib_path(DEFAULT_FAUST_LIB_PATH);
    return;
  }
#endif
  (void)csound;
}

/**
 * Faust controls class for Csound
 *
 **/
class controls : public UI {

  struct ctl {
    FAUSTFLOAT *zone;
    char label[65];
    cs_float min, max;
    ctl *nxt;
  } anchor;

  void addctl(const char *label, FAUSTFLOAT *zone, FAUSTFLOAT min,
              FAUSTFLOAT max) {
    ctl *pctl = &anchor;
    while (pctl->nxt)
      pctl = pctl->nxt;
    pctl->nxt = new ctl;
    pctl = pctl->nxt;
    strncpy(pctl->label, label, 63);
    pctl->label[63] = '\0'; pctl->label[64] = '\0';
    pctl->zone = zone;
    pctl->min = min;
    pctl->max = max;
    pctl->nxt = NULL;
  }

public:
  controls() {
    anchor.nxt = NULL;
    anchor.label[0] = '\0';
  }
  ~controls() {
    ctl *pctl = &anchor, *tmp;
    pctl = pctl->nxt;
    while (pctl) {
      tmp = pctl;
      pctl = pctl->nxt;
      delete tmp;
    }
  }

  virtual void openTabBox(const char *label){};
  virtual void openHorizontalBox(const char *label){};
  virtual void openVerticalBox(const char *label){};
  virtual void closeBox(){};

  virtual void addSoundfile(const char *label, const char *filename,
                            Soundfile **sf_zone){};

  virtual void addButton(const char *label, FAUSTFLOAT *zone) {
    addctl(label, zone, 0, 0);
  }
  virtual void addCheckButton(const char *label, FAUSTFLOAT *zone) {
    addctl(label, zone, 0, 0);
  }
  virtual void addVerticalSlider(const char *label, FAUSTFLOAT *zone,
                                 FAUSTFLOAT init, FAUSTFLOAT min,
                                 FAUSTFLOAT max, FAUSTFLOAT step) {
    addctl(label, zone, min, max);
  }
  virtual void addHorizontalSlider(const char *label, FAUSTFLOAT *zone,
                                   FAUSTFLOAT init, FAUSTFLOAT min,
                                   FAUSTFLOAT max, FAUSTFLOAT step) {
    addctl(label, zone, min, max);
  }
  virtual void addNumEntry(const char *label, FAUSTFLOAT *zone, FAUSTFLOAT init,
                           FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) {
    addctl(label, zone, min, max);
  }
  virtual void addHorizontalBargraph(const char *label, FAUSTFLOAT *zone,
                                     FAUSTFLOAT min, FAUSTFLOAT max){};
  virtual void addVerticalBargraph(const char *label, FAUSTFLOAT *zone,
                                   FAUSTFLOAT min, FAUSTFLOAT max){};

  FAUSTFLOAT *getZone(char *label) {
    ctl *pctl = &anchor;
    pctl = pctl->nxt;
    while (pctl) {
      if (strcmp(pctl->label, label) == 0)
        break;
      pctl = pctl->nxt;
    }
    if (pctl)
      return pctl->zone;
    else
      return NULL;
  }
  cs_float getMax(char *label) {
    ctl *pctl = &anchor;
    pctl = pctl->nxt;
    while (pctl) {
      if (strcmp(pctl->label, label) == 0)
        break;
      pctl = pctl->nxt;
    }
    if (pctl)
      return pctl->max;
    else
      return 0;
  }
  cs_float getMin(char *label) {
    ctl *pctl = &anchor;
    pctl = pctl->nxt;
    while (pctl) {
      if (strcmp(pctl->label, label) == 0)
        break;
      pctl = pctl->nxt;
    }
    if (pctl)
      return pctl->min;
    else
      return 0;
  }
};

/**
 * Faust object handle
 *
 **/
struct faustobj {
  void *obj;
  controls *ctls;
  faustobj *nxt;
  uint64_t cnt;
};

/**
 * Faust compile opcode

 usage:
 ihandle  faustcompile Scode, Sargs[,iasync, istacksize[,Starget]]

 ihandle - handle to compiled code
 Scode - Faust program
 Sargs - Faust compiler args
 istacksize - compiler stack size in megabytes (default 1MB). NOTE: this
              argument is accepted for backwards compatibility but is currently ignored,
              as the Csound 7 threading API no longer supports setting the stack size.
 iasync - async operation (1 = on, 0 = off) default to 1
 Starget - optional LLVM target string (empty selects the native target)
**/
struct faustcompile {
  OPDS h;
  cs_float *hptr;
  STRINGDAT *code;
  STRINGDAT *args;
  cs_float *async;
  cs_float *stacksize;
  STRINGDAT *target;         /* optional LLVM target string */
};

/* Persistent state for a compiled factory. The asynchronous compilation
   thread and the reset callback must not refer to the per-note faustcompile
   opcode, whose memory is released as soon as the instrument that runs
   faustcompile ends. */
struct faustcompile_state {
  CSOUND *csound;
  llvm_dsp_factory *factory;
  void *thread;
  faustobj *node;  /* reserved ::factory slot, published by the worker */
};

char **parse_cmd(CSOUND *csound, char *str, int32_t *argc) {
  char **argv;
  int32_t i = 0, n = 0, end = strlen(str);
  while (str[i] == ' ')
    i++;
  if (str[i] != '\0')
    *argc = 1;
  while (str[i] != '\0') {
    if (str[i] == ' ') {
      while (str[++i] == ' ')
        ;
      if (str[i] == '\0')
        break;
      (*argc)++;
    }
    i++;
  }
  argv = (char **)csound->Calloc(csound, sizeof(char *) * (*argc));
  i = 0;
  while (str[i] == ' ')
    i++;
  for (n = 0; n < *argc && i < end; n++) {
    argv[n] = &(str[i]);
    while (str[++i] != ' ' && i < end)
      ;
    if (i >= end)
      break;
    str[i] = '\0';
    while (str[++i] == ' ' && i < end)
      ;
  }
  return argv;
}

int32_t delete_faustcompile(CSOUND *csound, void *p) {

  faustcompile_state *st = ((faustcompile_state *)p);
  faustobj *fobj, *prv, **pfobj;
  if (st->thread)
    csound->JoinThread(st->thread);
  {
    std::lock_guard<std::mutex> guard(faust_registry_mutex);
    pfobj = (faustobj **)csound->QueryGlobalVariable(csound, "::factory");
    if(pfobj != NULL) {
      fobj = *pfobj;
      prv = NULL;
      while (fobj != NULL) {
        if (fobj == st->node) {
          if (prv == NULL)
            *pfobj = fobj->nxt;
          else
            prv->nxt = fobj->nxt;
          break;
        }
        prv = fobj;
        fobj = fobj->nxt;
      }
      if (fobj != NULL) {
        if (st->factory != NULL)
          deleteDSPFactory(st->factory);
        csound->Free(csound, fobj);
      }
    }
  }
  csound->Free(csound, st);
  return OK;
}

struct hdata {
  CSOUND *csound;
  char *code;
  char *args;
  char *target;
  faustcompile_state *state;
};

/* A factory slot is published in three states: obj == NULL while the
   asynchronous compilation is still running, obj == (void *)-1 if it
   failed, and otherwise obj is the llvm_dsp_factory. */
#define FAUST_SLOT_PENDING ((void *)NULL)
#define FAUST_SLOT_FAILED ((void *)-1L)

void *init_faustcompile_thread(void *pp) {

  hdata *hd = (hdata *)pp;
  CSOUND *csound = hd->csound;
  llvm_dsp_factory *factory;
  int32_t argc = 0;
  std::string err_msg;
  size_t alen = hd->args ? strlen(hd->args) : 0;
  char *cmd = (char *) csound->Calloc(csound, alen + 9);
  const char *ccode = hd->code;
  const char *target = hd->target ? hd->target : "";
  if (alen)
    strcpy(cmd, hd->args);
#ifdef USE_DOUBLE
  strcat(cmd, " -double");
#endif
  const char **argv = (const char **) parse_cmd(csound, cmd, &argc);

  {
    std::lock_guard<std::mutex> guard(faust_compile_mutex);
    factory = createDSPFactoryFromString("faustop", ccode,
                                         argc, argv, target, err_msg, 3);
  }
  hd->state->factory = factory;

  if (factory == NULL)
    csound->Message(csound, Str("\nFaust compilation problem:\nline %s\n"),
                    err_msg.c_str());

  /* Publish the outcome into the slot reserved during init; never touch the
     per-note output pointer, which may already have been reclaimed. */
  {
    std::lock_guard<std::mutex> guard(faust_registry_mutex);
    if (hd->state->node != NULL)
      hd->state->node->obj = factory ? (void *)factory : FAUST_SLOT_FAILED;
  }

  csound->Free(csound, argv);
  csound->Free(csound, cmd);
  csound->Free(csound, hd->code);
  csound->Free(csound, hd->args);
  if (hd->target)
    csound->Free(csound, hd->target);
  csound->Free(csound, hd);

  return NULL;
}

static uintptr_t init_faustcompile_thread_runner(void *pp) {
  init_faustcompile_thread(pp);
  return 0;
}

/* Append a new (pending) entry to the ::factory registry and return it. Must
   be called with faust_registry_mutex held. */
static faustobj *reserve_factory_slot(CSOUND *csound) {
  faustobj **pffactory, *ffactory, *fnew;
  const char *varname = "::factory";

  pffactory = (faustobj **)csound->QueryGlobalVariable(csound, varname);
  if (pffactory == NULL) {
    csound->CreateGlobalVariable(csound, varname, sizeof(faustobj *));
    pffactory = (faustobj **)csound->QueryGlobalVariable(csound, varname);
  }
  ffactory = *pffactory;
  if (ffactory == NULL) {
    fnew = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
    fnew->obj = FAUST_SLOT_PENDING;
    fnew->nxt = NULL;
    fnew->cnt = 0ul;
    *pffactory = fnew;
    return fnew;
  }
  while (ffactory->nxt != NULL)
    ffactory = ffactory->nxt;
  fnew = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
  fnew->obj = FAUST_SLOT_PENDING;
  fnew->nxt = NULL;
  fnew->cnt = ffactory->cnt + 1ul;
  ffactory->nxt = fnew;
  return fnew;
}

/* Resolve a faustcompile handle to its registry entry, waiting for an
   asynchronous compilation to publish the factory. Returns NULL (after
   issuing an error) on failure. */
static faustobj *resolve_factory(CSOUND *csound, int32_t handle) {
  faustobj **fobjp, *fobj = NULL;
  int timout = 0;
  int failed = (handle < 0);

  if (!failed) {
    fobjp = (faustobj **)csound->QueryGlobalVariable(csound, "::factory");
    if (fobjp == NULL) {
      csound->InitError(csound, "%s", Str("no factory available\n"));
      return NULL;
    }
    for (;;) {
      {
        std::lock_guard<std::mutex> guard(faust_registry_mutex);
        fobj = *fobjp;
        while (fobj != NULL && (int32_t)fobj->cnt != handle)
          fobj = fobj->nxt;
        if (fobj == NULL || fobj->obj != FAUST_SLOT_PENDING)
          break;
      }
      csound->Sleep(1);
      if (++timout > 1000) {
        csound->InitError(csound, "%s", Str("Faust code was not ready. Try compiling it \n"
                                            "in a separate instrument prior to running it here\n"));
        return NULL;
      }
    }
    if (fobj == NULL) {
      csound->InitError(csound, Str("factory not found %d\n"), handle);
      return NULL;
    }
    failed = (fobj->obj == FAUST_SLOT_FAILED);
  }
  if (failed) {
    csound->InitError(csound, "%s", Str("Faust code did not compile properly.\n"
                                        "Check above messages for Faust compiler errors\n"));
    return NULL;
  }
  return fobj;
}

int32_t init_faustcompile(CSOUND *csound, faustcompile *p) {
  faustcompile_state *st =
      (faustcompile_state *)csound->Malloc(csound, sizeof(faustcompile_state));
  st->csound = csound;
  st->factory = NULL;
  st->thread = NULL;
  st->node = NULL;

  /* Snapshot everything the asynchronous thread needs, so that it never
     touches the per-note opcode memory (which is freed when this instrument
     ends). The thread publishes its result into st->node, never into the
     output pointer. */
  hdata *data = (hdata *)csound->Malloc(csound, sizeof(hdata));
  data->csound = csound;
  data->code = csound->Strdup(csound, p->code->data);
  data->args = csound->Strdup(csound, p->args ? p->args->data : "");
  data->target = NULL;
  if (p->INOCOUNT > 4 && p->target != NULL && p->target->data != NULL)
    data->target = csound->Strdup(csound, p->target->data);
  data->state = st;

  /* Reserve the registry slot and publish the handle while the opcode is
     still alive, so that no write to per-note memory happens after the
     instrument ends. */
  {
    std::lock_guard<std::mutex> guard(faust_registry_mutex);
    st->node = reserve_factory_slot(csound);
  }
  *p->hptr = FL(st->node->cnt);

  if(!(int)*p->async) {
    /* synchronous compilation, run on the calling thread (large stack) */
    init_faustcompile_thread(data);
    /* Preserve the historical output convention for synchronous failure. */
    if (st->node->obj == FAUST_SLOT_FAILED)
      *p->hptr = -2.0;
  } else {
    st->thread = csound->CreateThread(init_faustcompile_thread_runner, data);
  }
  /* Factory cleanup is deferred to reset, so that instruments using the
     compiled factory are not affected when the compiling instrument ends.
     The callback owns the persistent state, not the opcode. */
  csound->RegisterResetCallback(csound, st, delete_faustcompile);
  return OK;
}

struct faustdsp {
  OPDS h;
  cs_float *ohptr;
  cs_float *code;            /* faust compiled code handle */
  llvm_dsp *engine;          /* faust DSP */
  llvm_dsp_factory *factory; /* DSP factory */
};

/* deinit function
   delete faust dsp objects
*/
int32_t delete_faustdsp(CSOUND *csound, void *p) {
  faustdsp *pp = (faustdsp *)p;
  faustobj *fobj, *prv, **pfobj;
  pfobj = (faustobj **)csound->QueryGlobalVariable(csound, "::dsp");
  if (pfobj != NULL) {
    fobj = *pfobj;
    prv = fobj;
    while (fobj != NULL) {
      if (fobj->obj == (void *)pp->engine) {
        prv->nxt = fobj->nxt;
        break;
      }
      prv = fobj;
      fobj = fobj->nxt;
    }
    if (fobj != NULL) {
      if (*pfobj == fobj)
        *pfobj = fobj->nxt;
      delete fobj->ctls;
      csound->Free(csound, fobj);
      delete pp->engine;
    } else
      csound->Warning(csound, Str("could not find DSP %p for deletion"),
                      pp->engine);
  }
  if (pp->factory)
    deleteDSPFactory(pp->factory);

  return OK;
}

int32_t init_faustdsp(CSOUND *csound, faustdsp *p) {
  int32_t factory;
  faustobj *fobj, **pfdsp, *fdsp;
  llvm_dsp *dsp;
  controls *ctls = new controls();
  const char *varname = "::dsp";

  factory = (int32_t)*((cs_float *)p->code);
  fobj = resolve_factory(csound, factory);
  if (fobj == NULL)
    return CSOUND_ERROR;

  dsp = ((llvm_dsp_factory *)fobj->obj)->createDSPInstance();
  if (dsp == NULL)
    return csound->InitError(csound, "%s", Str("Faust instantiation problem\n"));

  dsp->buildUserInterface(ctls);
  pfdsp = (faustobj **)csound->QueryGlobalVariable(csound, varname);
  if (pfdsp == NULL) {
    csound->CreateGlobalVariable(csound, varname, sizeof(faustobj *));
    pfdsp = (faustobj **)csound->QueryGlobalVariable(csound, varname);
    fdsp = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
    fdsp->obj = dsp;
    fdsp->ctls = ctls;
    fdsp->nxt = NULL;
    fdsp->cnt = 0;
    *pfdsp = fdsp;
  } else {
    fdsp = *pfdsp;
    if (fdsp != NULL) {
      while (fdsp->nxt) {
        fdsp = fdsp->nxt;
      }
      fdsp->nxt = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
      fdsp->nxt->cnt = fdsp->cnt + 1;
      fdsp = fdsp->nxt;
      fdsp->obj = dsp;
      fdsp->ctls = ctls;
    } else {
      fdsp = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
      fdsp->obj = dsp;
      fdsp->ctls = ctls;
      fdsp->nxt = NULL;
      fdsp->cnt = 0;
      *pfdsp = fdsp;
    }
  }

  p->factory = NULL; // this opcode does not own the factory
  p->engine = (llvm_dsp *)fdsp->obj;
  p->engine->init((int)csound->GetEngineSr(csound));
  *p->ohptr = (cs_float)fdsp->cnt;
  return OK;

}

struct faustplay {
  OPDS h;
  cs_float *outs[MAXARG];    /* outputs */
  cs_float *inst;
  cs_float *ins[VARGMAX];    /* inputs */
  llvm_dsp *engine;          /* faust DSP */
  llvm_dsp_factory *factory; /* DSP factory */
  AUXCH memin;
  AUXCH memout;
};


int32_t init_faustplay(CSOUND *csound, faustplay *p) {
  faustobj *fobj, **fobjp;
  const OPARMS *parms = csound->GetOParms(csound);
  int32_t instance = (int32_t)*p->inst;

  fobjp = (faustobj **)csound->QueryGlobalVariable(csound, "::dsp");
  if (fobjp == NULL)
    return csound->InitError(csound, "%s", Str("no dsp instances available\n"));
  fobj = *fobjp;
  if (fobj == NULL)
    return csound->InitError(csound, "%s", Str("no dsp instances available\n"));

  while ((int32_t)fobj->cnt != instance) {
    fobj = fobj->nxt;
    if (fobj == NULL)
      return csound->InitError(csound, Str("dsp instance not found %d\n"),
                               (int32_t)*p->inst);
  }

  p->engine = (llvm_dsp *)fobj->obj;

  /* The engine is owned and initialised by the opcode that created it
     (faustdsp, faustaudio or faustgen), so it is neither re-initialised nor
     deleted here; doing so would reset its state and invalidate the
     registry. */
  if (p->engine->getNumInputs() != p->INOCOUNT - 1) {
    p->engine = NULL;
    return csound->InitError(csound, "%s", Str("wrong number of input args\n"));
  }
  if (p->engine->getNumOutputs() != p->OUTOCOUNT) {
    p->engine = NULL;
    return csound->InitError(csound, "%s", Str("wrong number of output args\n"));
  }

  /* memory for sampAccurate offsets */
  if (parms->sampleAccurate) {
    size_t size;
    size = p->engine->getNumInputs() * sizeof(cs_float *);
    if (p->memin.auxp == NULL || p->memin.size < size)
      csound->AuxAlloc(csound, size, &p->memin);
    size = p->engine->getNumOutputs() * sizeof(cs_float *);
    if (p->memout.auxp == NULL || p->memout.size < size)
      csound->AuxAlloc(csound, size, &p->memout);
  }

  return OK;
}

int32_t perf_faustplay(CSOUND *csound, faustplay *p) {
  int32_t nsmps = CS_KSMPS, i;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early = p->h.insdshead->ksmps_no_end;
  cs_float **in_tmp = (cs_float **)p->memin.auxp;
  cs_float **out_tmp = (cs_float **)p->memout.auxp;

  if (UNLIKELY(early)) {
    for (i = 0; i < p->OUTOCOUNT; i++)
      memset(p->outs[i], '\0', nsmps * sizeof(cs_float));
    nsmps -= early;
  }
  if (UNLIKELY(offset)) {
    /* offset pointers, save current pos */
    for (i = 0; i < p->OUTOCOUNT; i++) {
      memset(p->outs[i], '\0', nsmps * sizeof(cs_float));
      out_tmp[i] = p->outs[i];
      p->outs[i] = &(p->outs[i][offset]);
    }
    for (i = 0; i < p->INOCOUNT - 1; i++) {
      in_tmp[i] = p->ins[i];
      p->ins[i] = &(p->ins[i][offset]);
    }
    nsmps -= offset;
  }

  p->engine->compute(nsmps, p->ins, p->outs);

  if (UNLIKELY(offset)) {
    /* restore pos  */
    for (i = 0; i < p->OUTOCOUNT; i++)
      p->outs[i] = out_tmp[i];
    for (i = 0; i < p->INOCOUNT - 1; i++)
      p->ins[i] = in_tmp[i];
  }
  return OK;
}


/**
 * faustgen and faustaudio opcodes

 usage:
 ihandle[,asig1,...] faustgen    Scode[,ain1,...]
 ihandle[,asig1,...] faustaudio  ifactory,[,ain1,...]

 Scode - Faust program
 ifactory - handle pointing to compiled code from faustcompile
 asig1 ... - audio outputs from Faust program
 ain1 ...  - audio inputs to Faust program

 ihandle - handle identifying this Faust DSP instance

**/
struct faustgen {
  OPDS h;
  cs_float *ohptr;
  cs_float *outs[MAXARG];    /* outputs */
  STRINGDAT *code;           /* faust code as string */
  cs_float *ins[VARGMAX];    /* inputs */
  llvm_dsp *engine;          /* faust DSP */
  llvm_dsp_factory *factory; /* DSP factory */
  controls *ctls;
  AUXCH memin;
  AUXCH memout;
};

struct hdata2 {
  faustgen *p;
  CSOUND *csound;
};


/* deinit function
   delete faust objects
*/
int32_t delete_faustgen(CSOUND *csound, void *p) {
  faustgen *pp = (faustgen *)p;
  faustobj *fobj, *prv, **pfobj;
  if((pfobj = (faustobj **)csound->QueryGlobalVariable(csound, "::dsp"))
     != NULL) {
    fobj = *pfobj;
    prv = fobj;
    while (fobj != NULL) {
      if (fobj->obj == (void *)pp->engine) {
        prv->nxt = fobj->nxt;
        break;
      }
      prv = fobj;
      fobj = fobj->nxt;
    }
    if (fobj != NULL) {
      if (*pfobj == fobj)
        *pfobj = fobj->nxt;
      csound->Free(csound, fobj);
      delete pp->ctls;
      delete pp->engine;
    }
  }
  if (pp->factory)
    deleteDSPFactory(pp->factory);

  return OK;
}


int32_t init_faustaudio(CSOUND *csound, faustgen *p) {
  int32_t factory;
  const OPARMS *parms = csound->GetOParms(csound);
  faustobj *fobj, **pfdsp, *fdsp;
  llvm_dsp *dsp;
  controls *ctls = new controls();
  const char *varname = "::dsp";

  factory = (int32_t)*((cs_float *)p->code);
  fobj = resolve_factory(csound, factory);
  if (fobj == NULL)
    return CSOUND_ERROR;

  dsp = ((llvm_dsp_factory *)fobj->obj)->createDSPInstance();
  if (dsp == NULL)
    return csound->InitError(csound, "%s", Str("Faust instantiation problem\n"));

  /* Validate the channel counts before publishing the instance, so that an
     invalid DSP is never left reachable through the ::dsp registry. */
  if (dsp->getNumInputs() != p->INOCOUNT - 1) {
    delete ctls;
    delete dsp;
    return csound->InitError(csound, "%s", Str("wrong number of input args\n"));
  }
  if (dsp->getNumOutputs() != p->OUTOCOUNT - 1) {
    delete ctls;
    delete dsp;
    return csound->InitError(csound, "%s", Str("wrong number of output args\n"));
  }

  dsp->buildUserInterface(ctls);
  pfdsp = (faustobj **)csound->QueryGlobalVariable(csound, varname);
  if (pfdsp == NULL) {
    csound->CreateGlobalVariable(csound, varname, sizeof(faustobj *));
    pfdsp = (faustobj **)csound->QueryGlobalVariable(csound, varname);
    fdsp = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
    fdsp->obj = dsp;
    fdsp->ctls = ctls;
    fdsp->nxt = NULL;
    fdsp->cnt = 0;
    *pfdsp = fdsp;
  } else {
    fdsp = *pfdsp;
    if (fdsp != NULL) {
      while (fdsp->nxt) {
        fdsp = fdsp->nxt;
      }
      fdsp->nxt = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
      fdsp->nxt->cnt = fdsp->cnt + 1;
      fdsp = fdsp->nxt;
      fdsp->obj = dsp;
      fdsp->ctls = ctls;
    } else {
      fdsp = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
      fdsp->obj = dsp;
      fdsp->ctls = ctls;
      fdsp->nxt = NULL;
      fdsp->cnt = 0;
      *pfdsp = fdsp;
    }
  }

  p->factory = NULL; // this opcode does not own the factory
  p->engine = (llvm_dsp *)fdsp->obj;
  p->engine->init((int)csound->GetEngineSr(csound));

  /* memory for sampAccurate offsets */
  if (parms->sampleAccurate) {
    size_t size;
    size = p->engine->getNumInputs() * sizeof(cs_float *);
    if (p->memin.auxp == NULL || p->memin.size < size)
      csound->AuxAlloc(csound, size, &p->memin);
    size = p->engine->getNumOutputs() * sizeof(cs_float *);
    if (p->memout.auxp == NULL || p->memout.size < size)
      csound->AuxAlloc(csound, size, &p->memout);
  }

  p->ctls = ctls;
  *p->ohptr = (cs_float)fdsp->cnt;
  return OK;
}

void *init_faustgen_thread(void *pp) {
  CSOUND *csound = ((hdata2 *)pp)->csound;
  faustgen *p = ((hdata2 *)pp)->p;
  const OPARMS *parms = csound->GetOParms(csound);
  std::string err_msg;
#ifdef USE_DOUBLE
  int32_t argc = 4;
  const char *argv[] = {"-vec", "-lv", " 1", "-double"};
#else
  int32_t argc = 3;
  const char *argv[] = {"-vec", "-lv", " 1"};
#endif
  faustobj **pfdsp, *fdsp;
  llvm_dsp *dsp;
  controls *ctls = new controls();
  const char *varname = "::dsp";
  p->engine = NULL;
  {
    std::lock_guard<std::mutex> guard(faust_compile_mutex);
    p->factory = createDSPFactoryFromString(
                                            "faustop", (const char *)p->code->data, argc, argv, "", err_msg, 3);
  }
  if (p->factory == NULL) {
    csound->InitError(csound, Str("Faust compilation problem: %s\n"),
                      err_msg.c_str());
    csound->Free(csound, pp);
    return NULL;
  }

  dsp = p->factory->createDSPInstance();
  if (dsp == NULL) {
    csound->InitError(csound, "%s",
                      Str("Faust instantiation problem\n"));
    csound->Free(csound, pp);
    return NULL;
  }

  dsp->buildUserInterface(ctls);

  /* Validate the channel counts before publishing the instance, so that an
     invalid DSP is never left reachable through the ::dsp registry. */
  if (dsp->getNumInputs() != p->INOCOUNT - 1) {
    csound->InitError(csound, "%s", Str("wrong number of input args\n"));
    delete ctls;
    delete dsp;
    deleteDSPFactory(p->factory);
    p->factory = NULL;
    csound->Free(csound, pp);
    return NULL;
  }
  if (dsp->getNumOutputs() != p->OUTOCOUNT - 1) {
    csound->InitError(csound,
                      Str("wrong number of output args: need %d had %d\n"),
                      dsp->getNumOutputs(),
                      p->OUTOCOUNT - 1);
    delete ctls;
    delete dsp;
    deleteDSPFactory(p->factory);
    p->factory = NULL;
    csound->Free(csound, pp);
    return NULL;
  }

  pfdsp = (faustobj **)csound->QueryGlobalVariable(csound, varname);
  if (pfdsp == NULL ||  *pfdsp == NULL) {
    csound->CreateGlobalVariable(csound, varname, sizeof(faustobj *));
    pfdsp = (faustobj **)csound->QueryGlobalVariable(csound, varname);
    fdsp = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
    fdsp->obj = dsp;
    fdsp->ctls = ctls;
    fdsp->nxt = NULL;
    fdsp->cnt = 0;
    *pfdsp = fdsp;
  } else {
    fdsp = *pfdsp;
    while (fdsp->nxt) {
      fdsp = fdsp->nxt;
    }
    fdsp->nxt = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
    fdsp->nxt->cnt = fdsp->cnt + 1;
    fdsp = fdsp->nxt;
    fdsp->obj = dsp;
    fdsp->ctls = ctls;
  }

  p->engine = dsp;
  dsp->init((int)csound->GetEngineSr(csound));

  /* memory for sampAccurate offsets */
  if (parms->sampleAccurate) {
    size_t size;
    size = p->engine->getNumInputs() * sizeof(cs_float *);
    if (p->memin.auxp == NULL || p->memin.size < size)
      csound->AuxAlloc(csound, size, &p->memin);
    size = p->engine->getNumOutputs() * sizeof(cs_float *);
    if (p->memout.auxp == NULL || p->memout.size < size)
      csound->AuxAlloc(csound, size, &p->memout);
  }
  p->ctls = ctls;
  *p->ohptr = (cs_float)fdsp->cnt;
  csound->Free(csound, pp);
  return NULL;
}

int32_t init_faustgen(CSOUND *csound, faustgen *p) {
  hdata2 *data = (hdata2 *)csound->Malloc(csound, sizeof(hdata2));
  data->csound = csound;
  data->p = p;
  /* Compile synchronously on the calling thread, which has a large stack;
     the original code also joined the worker thread immediately. */
  init_faustgen_thread(data);
  return OK;
}

int32_t perf_faust(CSOUND *csound, faustgen *p) {
  int32_t nsmps = CS_KSMPS, i;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early = p->h.insdshead->ksmps_no_end;
  cs_float **in_tmp = (cs_float **)p->memin.auxp;
  cs_float **out_tmp = (cs_float **)p->memout.auxp;

  if (UNLIKELY(early)) {
    for (i = 0; i < p->OUTOCOUNT - 1; i++)
      memset(p->outs[i], '\0', nsmps * sizeof(cs_float));
    nsmps -= early;
  }
  if (UNLIKELY(offset)) {
    /* offset pointers, save current pos */
    for (i = 0; i < p->OUTOCOUNT - 1; i++) {
      memset(p->outs[i], '\0', nsmps * sizeof(cs_float));
      out_tmp[i] = p->outs[i];
      p->outs[i] = &(p->outs[i][offset]);
    }
    for (i = 0; i < p->INOCOUNT - 1; i++) {
      in_tmp[i] = p->ins[i];
      p->ins[i] = &(p->ins[i][offset]);
    }
    nsmps -= offset;
  }

  p->engine->compute(nsmps, p->ins, p->outs);

  if (UNLIKELY(offset)) {
    /* restore pos  -- could be memcpy? */
    for (i = 0; i < p->OUTOCOUNT - 1; i++)
      p->outs[i] = out_tmp[i];
    for (i = 0; i < p->INOCOUNT - 1; i++)
      p->ins[i] = in_tmp[i];
  }
  return OK;
}

#define MAXPARAM 128

/**
 * faustctl opcode

 usage:
 faustctl  idsp, Slabel, kval[, Slabel1, kval1 ...]

 idsp - handle from an existing Faust DSP instance
 Slabel - name of control (in Faust program)
 kval - value to be sent to control

**/
struct faustctl {
  OPDS h;
  cs_float *inst;
  STRINGDAT *label;
  cs_float *val;
  cs_float *extraparam[MAXPARAM];
  FAUSTFLOAT *zone;
  cs_float min, max;
  cs_float minextra[MAXPARAM/2], maxextra[MAXPARAM/2];
  FAUSTFLOAT *zonextra[MAXPARAM/2];
};

int32_t init_faustctl(CSOUND *csound, faustctl *p) {

  faustobj *fobj, **fobjp;
  int32_t instance = (int32_t)*p->inst;

  /* checks that extra parameter count is even */
  if((p->INOCOUNT - 3)%2)
    return csound->InitError(csound, "unbalanced parameter count \n");

  fobjp = (faustobj **)csound->QueryGlobalVariable(csound, "::dsp");
  if (fobjp == NULL)
    return csound->InitError(csound, "%s", Str("no dsp instances available\n"));
  fobj = *fobjp;
  if (fobj == NULL)
    return csound->InitError(csound, "%s", Str("no dsp instances available\n"));

  while ((int32_t)fobj->cnt != instance) {
    fobj = fobj->nxt;
    if (fobj == NULL)
      return csound->InitError(csound, Str("dsp instance not found %d\n"),
                               (int32_t)*p->inst);
  }

  p->zone = fobj->ctls->getZone(p->label->data);
  if (p->zone == NULL)
    return csound->InitError(csound, Str("dsp control %s not found\n"),
                             p->label->data);
  p->max = fobj->ctls->getMax(p->label->data);
  p->min = fobj->ctls->getMin(p->label->data);
  {
    cs_float val = *p->val;
    if (p->min != p->max)
      val = val < p->min ? p->min : (val > p->max ? p->max : val);
    *p->zone = val;
  }


  /* implementation of extra optional parameters */
  for(int n = 0; n < p->INOCOUNT - 3; n+=2) {
    char *name = ((STRINGDAT *)p->extraparam[n])->data;
    p->zonextra[n/2] = fobj->ctls->getZone(name);
    if (p->zonextra[n/2] == NULL)
      return csound->InitError(csound, Str("dsp control %s not found\n"),
                               name);
    p->maxextra[n/2] = fobj->ctls->getMax(name);
    p->minextra[n/2] = fobj->ctls->getMin(name);
    {
      cs_float val = *(p->extraparam[n+1]);
      cs_float min = p->minextra[n/2];
      cs_float max = p->maxextra[n/2];
      if (min != max)
        val = val < min ? min : (val > max ? max : val);
      *(p->zonextra[n/2]) = val;
    }
  }

  return OK;
}

int32_t perf_faustctl(CSOUND *csound, faustctl *p) {
  cs_float val = *p->val;
  if (p->min != p->max)
    val = val < p->min ? p->min : (val > p->max ? p->max : val);
  *p->zone = val;

  /* implementation of extra optional parameters */
  for(int n = 0; n < p->INOCOUNT - 3; n+=2) {
    cs_float val = *(p->extraparam[n+1]);
    cs_float min = p->minextra[n/2];
    cs_float max = p->maxextra[n/2];
    if (min != max)
      val = val < min ? min : (val > max ? max : val);
    *(p->zonextra[n/2]) = val;
  }
  return OK;
}

#define S(x) sizeof(x)

static OENTRY localops[] = {
  {(char *)"faustgen", S(faustgen), 0,
   (char *)"immmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm", (char *)"SM",
   (SUBR)init_faustgen, (SUBR)perf_faust, (SUBR)delete_faustgen, NULL, 0},
  {(char *)"faustcompile", S(faustcompile), 0, (char *)"i", (char *)"SSpp",
   (SUBR)init_faustcompile, NULL, NULL, NULL, 0},
  {(char *)"faustcompile", S(faustcompile), 0, (char *)"i", (char *)"SSppS",
   (SUBR)init_faustcompile, NULL, NULL, NULL, 0},
  {(char *)"faustaudio", S(faustgen), 0,
   (char *)"immmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm", (char *)"iM",
   (SUBR)init_faustaudio, (SUBR)perf_faust, (SUBR)delete_faustgen, NULL, 0},
  {(char *)"faustdsp", S(faustdsp), 0,
   (char *)"i", (char *)"i",
   (SUBR)init_faustdsp, NULL, (SUBR)delete_faustdsp, NULL, 0},
  {(char *)"faustplay", S(faustplay), 0,
   (char *)"mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm", (char *)"iM",
   (SUBR)init_faustplay, (SUBR)perf_faustplay, NULL, NULL, 0},
  {(char *)"faustctl.i", S(faustgen), 0, (char *)"", (char *)"iSiN",
   (SUBR)init_faustctl, NULL, NULL, NULL, 0},
  {(char *)"faustctl.k ", S(faustgen), 0, (char *)"", (char *)"iSkN",
   (SUBR)init_faustctl, (SUBR)perf_faustctl, NULL, NULL, 0},
  {NULL, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, 0}};

PUBLIC int32_t csoundModuleCreate(CSOUND *csound) {
  (void)csound;
  return 0;
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound) {
  int32_t err = 0;
  OENTRY *ep = (OENTRY *)&(localops[0]);
  setup_faust_path(csound);
  while (ep->opname != NULL) {
    err |= csound->AppendOpcode(csound,
                                ep->opname, ep->dsblksiz, ep->flags,
                                ep->outypes, ep->intypes,
                                (int32_t (*)(CSOUND *, void *))ep->init,
                                (int32_t (*)(CSOUND *, void *))ep->perf,
                                (int32_t (*)(CSOUND *, void *))ep->deinit);
    ep++;
  }
  return err;
}

PUBLIC int32_t csoundModuleDestroy(CSOUND *csound) {
  (void)csound;
  return 0;
}

PUBLIC int32_t csoundModuleInfo(void) {
  return CSOUND_MODULE_INFO;
}
