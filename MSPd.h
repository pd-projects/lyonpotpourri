#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <strings.h>
#include <string.h>

/* choose your poison */

#define __MSP__ (0)
#define __PD__ (!__MSP__)

/*
Note - Pd does silly things to a curly brace inside a string. Oh, well.
*/

#define LYONPOTPOURRI_MSG "by Eric Lyon  -{LyonPotpourri 2.0}-"

#if __MSP__
#include "ext.h"
#include "z_dsp.h"
#include "buffer.h"
#include "ext_obex.h"
#define t_floatarg double
#define resizebytes t_resizebytes
#define getbytes t_getbytes
#define freebytes t_freebytes
#endif

/* because Max and Pd have different ideas of what A_FLOAT is, use t_floatarg
to force consistency. Otherwise functions that look good will fail on some
hardware. Also note that Pd messages cannot accept arguments of type A_LONG. */

#if __PD__
#include "m_pd.h"
#define t_floatarg float
#define atom_getsymarg atom_getsymbolarg
#endif

#ifndef PIOVERTWO
#define PIOVERTWO 1.5707963268
#endif
#ifndef TWOPI
#define TWOPI 6.2831853072
#endif
#ifndef PI
#define PI 3.14159265358979
#endif


