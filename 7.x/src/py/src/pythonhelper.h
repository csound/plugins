/*
  * pythonhelper.h
  *
  * Copyright (C) 2002 Maurizio Umberto Puxeddu
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
  * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
*/

#ifndef _pycsound_pythonhelper_h_
#define _pycsound_pythonhelper_h_

#ifdef _DEBUG
# undef _DEBUG
#  include <Python.h>
# define _DEBUG
#else
# include <Python.h>
#endif

/* PyLocal variable is now a global variable indexed by INSDS */
static inline PyObject *GetPyLocal(INSDS *ids){
  char insds[32];
  CSOUND *csound = ids->csound;
  snprintf(insds, 32, "%p", ids);
  PyObject **p = (PyObject **) csound->QueryGlobalVariable(csound,insds);
  return *p;
}

static inline void SetPyLocal(INSDS *ids, void *pp){
  char insds[32];
  CSOUND *csound = ids->csound;
  snprintf(insds, 32, "%p", ids);
  PyObject **p = (PyObject **) csound->QueryGlobalVariable(csound,insds);
  *p = (PyObject *) pp;
}

#define GETPYLOCAL(ids) (GetPyLocal(ids))
#define SETPYLOCAL(ids, p) { SetPyLocal(ids, p); }

#endif
