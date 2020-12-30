/* This file is part of statusbar - a status bar for dwm
   Copyright (C) 2020  Ellis Rhys Thomas <e.rhys.thomas@gmail.com>
   See COPYING for licence details. */

#ifndef SB_MISC_H
#define SB_MISC_H

#include <stdarg.h>

/* returns malloced string generated from printf style format
   string (must provide enough arguments to match format string) */
char *smprintf(char *fmt, ...);

#endif
