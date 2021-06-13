/* This file is part of statusbar - a status bar for dwm
   Copyright (C) 2020  Ellis Rhys Thomas <e.rhys.thomas@gmail.com>
   See COPYING for licence details. */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pulse/pulseaudio.h>
#include <assert.h>
#include <string.h>

#include "misc.h"
#include "audio.h"

struct server_connection {
    pa_mainloop               *mainloop;
    pa_mainloop_api           *api;
    pa_context                *context;
};

/* Forward Declarations */
static int  pulseaudio_server_connect(struct server_connection *pulseaudio);
static void pulseaudio_server_disconnect(struct server_connection *pulseaudio);
static int  pulseaudio_mainloop_state(struct server_connection *pulseaudio, enum pa_context_state *state);
static int  pulseaudio_operation_state(struct server_connection *pulseaudio, pa_operation *id);

static void mainloop_state_cb(pa_context *c, void *userdata);
static void default_sink_name_cb(pa_context *c, const pa_server_info*i, void *userdata);
static void sink_activeportname_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata);
static void sink_volume_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata);
static void sink_ismute_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata);


/* Creates a PulseAudio context and returns an opaque handle */
struct server_connection *
audio_server_open(void)
{
    struct server_connection *pulseaudio;

    pulseaudio = calloc(1, sizeof *pulseaudio);
    if (!pulseaudio)
	return NULL;

    if (pulseaudio_server_connect(pulseaudio)) {
	pulseaudio_server_disconnect(pulseaudio);
	return NULL;
    }
	
    return pulseaudio;
}

void
audio_server_close(struct server_connection *pulseaudio)
{
    pulseaudio_server_disconnect(pulseaudio);
}

char *
audio_get_sink_name(struct server_connection *pulseaudio)
{
    char namebuf[512];
    pa_operation *id;
    int state;

    assert(pulseaudio);
    id = pa_context_get_server_info(pulseaudio->context,
				    default_sink_name_cb,
				    namebuf);
    
    state = pulseaudio_operation_state(pulseaudio, id);
    pa_operation_unref(id);

    return state ? NULL : smprintf("%s", namebuf);
}

char *
audio_get_port_name(struct server_connection *pulseaudio, char *sinkname)
{
    char namebuf[512];
    pa_operation *id;
    int state;

    assert(pulseaudio); 

    id = pa_context_get_sink_info_by_name(pulseaudio->context,
					  sinkname,
					  sink_activeportname_cb,
					  namebuf);
    
    state = pulseaudio_operation_state(pulseaudio, id);
    pa_operation_unref(id);

    return state ? NULL : smprintf("%s", namebuf);
}

unsigned int
audio_get_volume(struct server_connection *pulseaudio, char *sinkname)
{
    unsigned int volume;
    pa_operation *id;
    int state;

    assert(pulseaudio && sinkname);

    id = pa_context_get_sink_info_by_name(pulseaudio->context,
					  sinkname,
					  sink_volume_cb,
					  &volume);
					  
    state = pulseaudio_operation_state(pulseaudio, id);
    pa_operation_unref(id);

    return state ? 0 : volume;
}

unsigned int
audio_is_mute(struct server_connection *pulseaudio, char *sinkname)
{
    unsigned int ismute;
    pa_operation *id;
    int state;

    assert(pulseaudio && sinkname);

    id = pa_context_get_sink_info_by_name(pulseaudio->context,
					  sinkname,
					  sink_ismute_cb,
					  &ismute);

    state = pulseaudio_operation_state(pulseaudio, id);
    pa_operation_unref(id);

    return state ? 2 : ismute;    
}

/*
   Functions using the PulseAudio API
 */

/* Returns 0 if successfully connected 1 on failure  */
static int
pulseaudio_server_connect(struct server_connection *pulseaudio)
{
    enum pa_context_state state;

    pulseaudio->mainloop = pa_mainloop_new();
    if (!pulseaudio->mainloop) 
	return 1;
    pulseaudio->api = pa_mainloop_get_api(pulseaudio->mainloop);
    if (!pulseaudio->api)
	return 1;

    pulseaudio->context = pa_context_new(pulseaudio->api, "statusbar");
    if (!pulseaudio->context)
	return 1;

    pa_context_set_state_callback(pulseaudio->context, mainloop_state_cb, &state);
    pa_context_connect(pulseaudio->context, NULL, 0, NULL);

    return pulseaudio_mainloop_state(pulseaudio, &state);
}

/* pulseaudio_server_disconnect():  */
static void
pulseaudio_server_disconnect(struct server_connection *pulseaudio)
{
    if (pulseaudio) {

	if (pulseaudio->context) {
	    pa_context_disconnect(pulseaudio->context);
	    pa_context_unref(pulseaudio->context);
	    pulseaudio->context = NULL; 
	}

	if (pulseaudio->mainloop) {
	    pa_mainloop_free(pulseaudio->mainloop);
	    pulseaudio->mainloop = NULL;
	}

	free(pulseaudio);
    }
}

/* pulseaudio_mainloop_state(): Recurses until operation complete (0)
   or failed (1) is returned */
static int
pulseaudio_mainloop_state(struct server_connection *pulseaudio, enum pa_context_state *state)
{
    switch (*state) {
    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING: 	
    case PA_CONTEXT_AUTHORIZING: 	
    case PA_CONTEXT_SETTING_NAME: 	
	pa_mainloop_iterate(pulseaudio->mainloop, 1, NULL);
	break;
    case PA_CONTEXT_READY: 	
	return 0;
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
	return 1;
    }
	    
    return pulseaudio_mainloop_state(pulseaudio, state);
}

/* pulseaudio_operation_state(): Recurses until operation complete (0)
   or failed (1) is returned */
static int
pulseaudio_operation_state(struct server_connection *pulseaudio, pa_operation *id)
{
    switch (pa_operation_get_state(id)) {
    case PA_OPERATION_RUNNING:
	pa_mainloop_iterate(pulseaudio->mainloop, 1, NULL);
	break;
    case PA_OPERATION_DONE:
	return 0;
    case PA_OPERATION_CANCELLED: /* failure case */
	return 1;
    }
    
    return pulseaudio_operation_state(pulseaudio, id);
}

/*
  PulseAudio mainloop callbacks.
*/

/* mainloop_state_cb(): populates userdata with a STATE enumeration,
   describing the state of context registration. */
static void
mainloop_state_cb(pa_context *c, void *userdata)
{
    enum pa_context_state *state;

    assert(c);
    state = userdata;
    *state = c ? pa_context_get_state(c) : PA_CONTEXT_FAILED;
}

/* default_sink_name_cb(): populates userdata with the name of the
   server's default sink */
static void
default_sink_name_cb(__attribute__((unused)) pa_context *c, const pa_server_info*i, void *userdata)
{
    char *name;

    assert(c);
    name = userdata;
    strcpy(name, i ? i->default_sink_name : "error");
}

/* sink_activeportname_cb(): populates userdata with the name of the
   active port */
static void
sink_activeportname_cb(__attribute__((unused)) pa_context *c, const pa_sink_info *i, int eol, void *userdata)
{
    char *name;

    if (eol > 0) /* final entry read, do nothing */
	return;

    assert(c);
    name = userdata;
    strcpy(name, i ? i->active_port->name : "error");
}

/* sink_volume_cb(): populate userdata with the integer average volume
   of the sink in percent. */
static void
sink_volume_cb(__attribute__((unused)) pa_context *c, const pa_sink_info *i, int eol, void *userdata)
{
    unsigned int *vol;

    if (!c || eol > 0) /* final entry read, do nothing */
	return;

    assert(c);
    vol = userdata;
    *vol = i ? 100*pa_cvolume_avg(&i->volume) / PA_VOLUME_NORM : 0;
}

/* sink_ismute_cb(): populate userdata with 0, 1 or 2 if the mute is
   active, inactive or error respectively. */
static void
sink_ismute_cb(__attribute__((unused)) pa_context *c, const pa_sink_info *i, int eol, void *userdata)
{
    unsigned int *ismute;

    if (eol > 0) /* final entry read, do nothing */
	return;

    assert(c);
    ismute = userdata;
    *ismute = i ? i->mute : 2;
}

