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

#define SYS_LMAX        512

/*
  Forward Declarations
*/
char *sysfs_getline(const char *root, const char *file);
unsigned long sysfs_getul(const char *root, const char *file);

char *mkbat(const char *syspath);
char *mkvol(Audio *pulse);
char *mktim(const struct tm *loctime);
	      
void    update_time(struct tm **out);
Display *x11open(void);
void    x11print(Display *x11d, const char *str);
void    x11close(Display *toclose);

volatile sig_atomic_t mainloopbreak;

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

char *
mkvol(Audio *server)
{
    const char *symbol;
    char *sinkname, *portname;
    unsigned volume;

    sinkname = audio_get_sink_name(server);
    portname = audio_get_port_name(server, sinkname);
    volume   = audio_get_volume(server, sinkname);

    if (audio_is_mute(server, sinkname)) 
	symbol = symbol_mute;
    else if (strstr(portname, "headphones")) 
	symbol = symbol_jack;
    else if (strstr(portname, "speaker"))
	symbol = symbol_speakers;
    else if (strstr(portname, "headset"))
	symbol = symbol_bt;
    else 
	symbol = symbol_unknown;

    free(sinkname);
    free(portname);
    
    return smprintf("%s%u%%", symbol, volume);
}

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
Display *
x11open(void)
{
    Display *new;

    new = XOpenDisplay(NULL);
    if (!new)
	exit(1);

    return new;
}

/* Push the status string to the X11 output buffer and update */
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

void
x11close(Display *toclose)
{
    XCloseDisplay(toclose);
}

/* sets the global to the signal number */
void
set_mainloopbreak(int signum)
{
    mainloopbreak = signum;
}

int
main(void)
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
