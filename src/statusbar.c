/* This file is part of statusbar - a status bar for dwm
   Copyright (C) 2020  Ellis Rhys Thomas <e.rhys.thomas@gmail.com>
   See COPYING for licence details. */

/* Based on dwmstatus <https://github.com/kamiyaa/dwmstatus> */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <X11/Xlib.h>

#include "config.h"
#include "audio.h"
#include "misc.h"

/* mainloopbreak: event loop condition in main proceedure. */
volatile sig_atomic_t mainloopbreak;
void set_mainloopbreak(int signum);

/* sysfs: reading kernel entries in the system file system */
#define SYS_LMAX        512
char *sysfs_getline(const char *root, const char *file);
unsigned long sysfs_getul(const char *root, const char *file);

/* mk: string creation proceedures */
char *mkbat(const char *syspath);
char *mkvol(Audio *pulse);
char *mktim(const struct tm *loctime);
	      
/* x11: control the display */
Display *x11open(void);
void    x11print(Display *x11d, const char *str);
void    x11close(Display *toclose);

/* misc */
void    update_time(struct tm **out);

char *
sysfs_getline(const char *root, const char *file)
{
    char *path, buf[SYS_LMAX+1];
    FILE *sysfh;
    size_t len;

    path = smprintf("%s/%s", root, file);
    sysfh = fopen(path, "r");
    free(path);
    if (!sysfh)
	return smprintf("Can't open %s", file);

    len = fread(buf, 1, SYS_LMAX, sysfh);
    fclose(sysfh);
    if (len == 0)
	return smprintf("Can't read %s", file);

    if (buf[len-1] == '\n')
	buf[len-1] = '\0';
    else
	buf[len] = '\0';

    return smprintf("%s", buf);
}

unsigned long
sysfs_getul(const char *root, const char *file)
{
    char          *line;
    unsigned long  ret;

    line = sysfs_getline(root, file);
    ret = strtoul(line, NULL, 10);    
    free(line);

    return ret;
}

void
update_time(struct tm **out)
{
    time_t systime;

    systime = time(NULL);
    *out = localtime(&systime);
    if (!*out)
	exit(1);
}
/*
  String making functions
*/

char *
mkbat(const char *syspath)
{
    const char    *symbol;
    char          *status;
    unsigned long  now, full;

    full   = sysfs_getul(syspath, "energy_full");
    now    = sysfs_getul(syspath, "energy_now");
    status = sysfs_getline(syspath, "status");

    if (!strcmp(status, "Charging"))
	symbol = symbol_ac;
    else if (!strcmp(status, "Not charging")) /* still pulugged in but full */
	symbol = symbol_ac;
    else if (!strcmp(status, "Discharging"))
	symbol = symbol_batt;
    else
	symbol = symbol_unknown;

    free(status);

    return smprintf("%s%u%%", symbol, full==0 ? 0L : (100L*now)/full);
}

/* mkvol(): create string with audio sink volume % and an icon to
   represent the type. */
char *
mkvol(Audio *server)
{
    char *s, *sinkname, *portname;
    unsigned volume;

    sinkname = audio_get_sink_name(server);
    portname = audio_get_port_name(server, sinkname);
    volume   = audio_get_volume(server, sinkname);

    fprintf(stderr, "portname = \"%s\"\n", portname); 
    fprintf(stderr, "sinkname = \"%s\"\n", sinkname); 

    s = smprintf("%s%u%%", lookup(sinks, sinkname), lookup(ports, portname)
		 volume);

    free(sinkname);
    free(portname);
    
    return ;
}

/* mktime(): create a string with date and time */
char *
mktim(const struct tm *loctime)
{
    char *ptr;

    ptr = malloc(256);
    if (!ptr)
	exit(1);

    strftime(ptr, 256, DATE_FORMAT, loctime);

    return ptr;
}

/*
  X11 functions
 */

/* x11open(): connect to display server */
Display *
x11open(void)
{
    Display *new;

    new = XOpenDisplay(NULL);
    if (!new)
	exit(1);

    return new;
}

/* x11print(): push a string to the display server. */
void
x11print(Display *x11d, const char *str)
{
#ifdef DEBUG
    (void)x11d;
    printf("[statusbar]:%s\n", str);
#else
    XStoreName(x11d, DefaultRootWindow(x11d), str);
    XSync(x11d, False);
#endif
}

/* x11close(): close the connection to the display server. */
void
x11close
(Display *toclose)
{
    XCloseDisplay(toclose);
}

/* set_mainloopbreak(): when signal number is non-zero this proceedure
   breaks the event loop in main. */
void
set_mainloopbreak(int signum)
{
    mainloopbreak = signum;
}

/* geticon(): returns a icon string from a table using a string */
const char *
geticon(const char **table, const char *s)
{
    




}

/* statusbar: updates the root window name in x11. */
int main(void)
{
    struct sigaction mainloop_handler;
    struct tm       *loctime;
    Display         *x11d;
    Audio           *pulse;
    char            *vol, *bat, *tim, *sstr;

    mainloop_handler.sa_handler = set_mainloopbreak;
    mainloop_handler.sa_flags = 0;
    sigemptyset(&mainloop_handler.sa_mask);
    sigaction(SIGINT, &mainloop_handler, NULL);

    x11d  = x11open();
    pulse = audio_server_open();

    while (!mainloopbreak) {

	vol = mkvol(pulse);
	bat = mkbat(sysfs_batt);
	update_time(&loctime);
	tim = mktim(loctime);

	sstr = smprintf("%s %s %s", vol, bat, tim);
	x11print(x11d, sstr);

	free(vol);
	free(bat);
	free(tim);
	free(sstr);
	sleep(1);
    }; 

    audio_server_close(pulse);
    x11close(x11d);
    
    return 0;
}
