/***************************************************************************
                          event.c  -  description
                             -------------------
    begin                : Sat Sep 8 2001
    copyright            : (C) 2001 by Michael Speck
    email                : kulkanie@gmx.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ltris.h"
#include "event.h"

extern Sdl sdl;
extern SDL_Cursor *empty_cursor;
extern SDL_Cursor *std_cursor;
extern Config config;
int keystate[SDLK_LAST];
int buttonstate[BUTTON_COUNT];
int rel_motion = 0; /* relative mouse motion? */
int motion_x = 0, motion_y = 0; /* current position of mouse */
int motion_rel_x = 0; /* position of mouse relative to old position */
int motion = 0; /* motion occured? */
int motion_button = 0; /* button pressed while moving */
float motion_mod;
/* interna */
int intern_motion = 0; /* event_filter noted a motion so event_poll will set motion next time */
int intern_motion_rel_x = 0;
int intern_motion_x = 0, intern_motion_y = 0;
int intern_motion_last_x, intern_motion_last_y = 0;
int intern_motion_button = 0;
int intern_block_motion = 0;

extern int use_shadow_surface, video_sw, video_sh, video_xoff, video_yoff;

/*
====================================================================
Event filter used to get motion x.
====================================================================
*/
int event_filter( const SDL_Event *event )
{
    return 1;
}

/*
====================================================================
Reset event states
====================================================================
*/
void event_reset()
{
    memset( keystate, 0, sizeof( keystate ) );
    memset( buttonstate, 0, sizeof( buttonstate ) );
    motion_mod = (float)(config.motion_mod) / 100;
    intern_motion = motion = 0;
}
/*
====================================================================
Grab or release input. Hide cursor if events are kept in window.
Use relative mouse motion and grab if config tells so.
====================================================================
*/
void event_grab_input()
{
}
void event_ungrab_input()
{
}
/*
====================================================================
Poll next event and set key and mousestate.
Return Value: True if event occured
====================================================================
*/
int event_poll( SDL_Event *event )
{
	return 0;
}
/*
====================================================================
Block/unblock motion event
====================================================================
*/
void event_block_motion( int block )
{
}
