/* This file is part of statusbar - a status bar for dwm
   Copyright (C) 2020  Ellis Rhys Thomas <e.rhys.thomas@gmail.com>
   See COPYING for licence details. */

#ifndef AUDIO_H
#define AUDIO_H

typedef struct server_connection Audio; 

Audio *audio_server_open(void);
void audio_server_close(Audio *server);

char *audio_get_sink_name(Audio *server);
char *audio_get_port_name(Audio *server, char *sinkname);
unsigned audio_get_volume(Audio *server, char *sinkname);
unsigned audio_is_mute(Audio *server, char *sinkname);


#endif	/* AUDIO_H */
