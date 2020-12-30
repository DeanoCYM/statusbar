/* This file is part of statusbar - a status bar for dwm
   Copyright (C) 2020  Ellis Rhys Thomas <e.rhys.thomas@gmail.com>
   See COPYING for licence details. */


/* statusbar configuration file */


/* Paths to devices in sysfs, arrays must be null terminated. */
const char *sysfs_batt =  "/sys/class/power_supply/BAT0";
const char *sysfs_networks[] = {
    "/sys/class/net/enp0s31f6",
    "/sys/class/net/wlp1s0",
    NULL };

/* Sets the format of the date and time. See man 3 strftime. */
#define DATE_FORMAT      "%d/%m/%y %H:%M"

/* State symbols */
const char *symbol_speakers   = "🔈";
const char *symbol_jack       = "➰";
const char *symbol_bt         = "🎧";
const char *symbol_mute       = "🔇";
const char *symbol_batt       = "🔋";
const char *symbol_ac         = "🔌";
const char *symbol_nonetwork  = "​🚫";
const char *symbol_wlan       = "📶"; 
const char *symbol_wwan       = "📡";
const char *symbol_ethernet   = "🌍";
const char *symbol_unknown    = "??"; 
