/* Native DSSI/LADSPA library lookup. */
#ifndef DSSI4CS_UTILS_H
#define DSSI4CS_UTILS_H
#include "csdl.h"
/* Search path used when neither DSSI_PATH nor LADSPA_PATH is set. The build
   may override this (see CMakeLists.txt) to prepend the multiarch library
   directory detected on the build host, e.g. /usr/lib/x86_64-linux-gnu. */
#ifndef DSSI4CS_DEFAULT_PATH
#define DSSI4CS_DEFAULT_PATH \
    "/usr/local/lib/dssi:/usr/lib/dssi:/usr/lib64/dssi:" \
    "/usr/local/lib/ladspa:/usr/lib/ladspa:/usr/lib64/ladspa"
#endif
void *dlopenLADSPA(CSOUND *, const char *, int32_t);
#endif
