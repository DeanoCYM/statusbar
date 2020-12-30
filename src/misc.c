/* This file is part of statusbar - a status bar for dwm
   Copyright (C) 2020  Ellis Rhys Thomas <e.rhys.thomas@gmail.com>
   See COPYING for licence details. */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "misc.h"

/* Returns dynamically allocated string generated from a printf style
   format and varg list. */
char *
smprintf(char *fmt, ...)
{
    va_list  fmtargs;
    char    *str;
    int      len;

    va_start(fmtargs, fmt);
    len = vsnprintf(NULL, 0, fmt, fmtargs);
    va_end(fmtargs);

    str = malloc(++len);
    if (str == NULL)
	exit(1);

    va_start(fmtargs, fmt);
    vsnprintf(str, len, fmt, fmtargs);
    va_end(fmtargs);

    return str;
}
