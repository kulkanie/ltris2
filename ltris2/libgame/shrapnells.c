/***************************************************************************
                          shrapnells.c  -  description
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
#include "config.h"
#include "list.h"
#include "shrapnells.h"

extern Config config;
extern Sdl sdl;
extern SDL_Surface *offscreen;
List *shrapnells = 0;

/*
====================================================================
Local
====================================================================
*/

/*
====================================================================
Create a single shrapnell and add to list.
====================================================================
*/
void shrapnell_create( int x, int y, int w, int h, float vx, float vy, float gx, float gy )
{
    Shrapnell *shr = calloc( 1, sizeof( Shrapnell ) );
    shr->pic = create_surf(w, h, SDL_SWSURFACE);
    SDL_SetColorKey(shr->pic, 0, 0);
    DEST(shr->pic, 0, 0, w, h);
    SOURCE(offscreen, x, y);
    blit_surf();
    shr->x = x;
    shr->y = y;
    shr->v.x = vx;
    shr->v.y = vy;
    shr->g.x = gx;
    shr->g.y = gy;
    shr->alpha = 64;
    list_add( shrapnells, shr);
}
/*
====================================================================
Delete a shrapnell
====================================================================
*/
void shrapnell_delete( void *ptr )
{
    Shrapnell *shr = (Shrapnell*)ptr;
    SDL_FreeSurface( shr->pic );
    free( shr );
}

/*
====================================================================
Public
====================================================================
*/

/*
====================================================================
Initate or delete shrapnell list
====================================================================
*/
void shrapnells_init()
{
    shrapnells = list_create( LIST_AUTO_DELETE, shrapnell_delete );
}
void shrapnells_delete()
{
    list_delete( shrapnells );
}
/*
====================================================================
Create shrapnells from position in offscreen for one line of blocks.
====================================================================
*/
void shrapnells_create( int x, int y, int w, int h )
{
	int type, i, j;
	int shr_w = BOWL_BLOCK_SIZE, shr_h = h;
	float vx;

	type = rand() % 8;
	if (config.anim == 0)
		type = 0;

	/* create shrapnells */
	switch (type) {
	case 0:
		/* simple sideways animation */
		for ( i = 0, j = 1; i < w; i += shr_w, j++ ) {
			if (j <= 5)
				vx = (j-1) * -0.0667;
			else
				vx = (10 - j) * 0.0667;
			shrapnell_create(x + i, y, shr_w, shr_h, vx, 0, 0, 0);
		}
		break;
	case 1:
	case 2:
		for ( i = 0, j = 1; i < w; i += shr_w, j++ ) {
			if ( type == 1 )
				shrapnell_create(x + i, y, shr_w, shr_h, 0, ( 11 - j ) * -0.015, 0, 0.0002 );
			else
				shrapnell_create(x + i, y, shr_w, shr_h, 0, j * -0.015, 0, 0.0002 );
		}
		break;
	case 3:
	case 4:
		for ( i = 0, j = 1; i < w / 2; i += shr_w, j++ ) {
			if ( type == 3 ) {
				shrapnell_create( x + i, y, shr_w, shr_h, 0, j * -0.016, 0, 0.0002 );
				shrapnell_create( x + w - i - shr_w, y, shr_w, shr_h, 0, j * -0.016, 0, 0.0002 );
			}
			else {
				shrapnell_create( x + i, y, shr_w, shr_h, 0, ( 6 - j ) * -0.016, 0, 0.0002 );
				shrapnell_create( x + w - i - shr_w, y, shr_w, shr_h, 0, ( 6 - j ) * -0.016, 0, 0.0002 );
			}
		}
		break;
	case 5:
		for ( i = 0, j = 1; i < w / 2; i += shr_w, j++ ) {
			shrapnell_create( x + i, y, shr_w, shr_h, ( 6 - j ) * 0.02, 0, 0, 0 );
			shrapnell_create( x + w - i - shr_w, y, shr_w, shr_h, ( 6 - j ) * -0.02, 0, 0, 0 );
		}
		break;
	case 6:
	case 7:
		for ( i = 0, j = 1; i < w; i += shr_w, j++ ) {
			if ( type == 6 )
				shrapnell_create( x + i, y, shr_w, shr_h, j * -0.02, ( 11 - j ) * -0.01, 0, 0.0002 );
			else
				shrapnell_create( x + i, y, shr_w, shr_h, ( 11 - j ) * 0.02, ( 11 - j ) * -0.01, 0, 0.0002 );
		}
		break;
	}
}
/*
====================================================================
Remove all shrapnells
====================================================================
*/
void shrapnells_reset()
{
    list_clear( shrapnells );
}
/*
====================================================================
Show, hide all shrapnells.
====================================================================
*/
void shrapnells_hide()
{
    List_Entry  *entry = shrapnells->head.next;
    Shrapnell   *shr;
    int         x, y;
    while ( entry != &shrapnells->tail ) {
        shr = entry->item;
        x = (int)shr->x;
        y = (int)shr->y;
        DEST(sdl.screen, x, y, shr->pic->w, shr->pic->h);
        SOURCE(offscreen, x, y);
        blit_surf();
        add_refresh_rect(x, y, shr->pic->w, shr->pic->h);
        entry = entry->next;
    }
}
void shrapnells_show()
{
    List_Entry  *entry = shrapnells->head.next;
    Shrapnell   *shr;
    int         x, y;
    while ( entry != &shrapnells->tail ) {
        shr = entry->item;
        x = (int)shr->x;
        y = (int)shr->y;
        DEST(sdl.screen, x, y, shr->pic->w, shr->pic->h);
        SOURCE(shr->pic, 0, 0);
        if (shr->alpha == 0)
            blit_surf();
        else
            alpha_blit_surf((int)shr->alpha);
        add_refresh_rect(x, y, shr->pic->w, shr->pic->h);
        entry = entry->next;
    }
}
/*
====================================================================
Update and remove expired shrapnells
====================================================================
*/
void shrapnells_update( int ms )
{
    List_Entry  *entry = shrapnells->head.next;
    Shrapnell   *shr;
    while ( entry != &shrapnells->tail ) {
        shr = entry->item;
        shr->v.x += shr->g.x * ms;
        shr->v.y += shr->g.y * ms;
        shr->x += shr->v.x * ms;
        shr->y += shr->v.y * ms;
        if (shr->alpha < 255)
            shr->alpha += 0.64 * ms;
        entry = entry->next;
        if (shr->alpha >= 255 || shr->x + shr->pic->w < 0 || shr->y + shr->pic->h < 0 || shr->x > sdl.screen->w || shr->y > sdl.screen->h)
            list_delete_entry( shrapnells, entry->prev );
    }
}

/*
====================================================================
Brick explosion animations.
====================================================================
*/
#define exp_change 0.008
#define exp_alpha_change 0.2
List *exps;
SDL_Surface *exp_pic = 0;
int   exp_w, exp_h; /* size */
int   exp_frame_count; /* frame count */
typedef struct {
    float alpha;
    float cur; /* current frame */
    int x, y; /* position in screen */
} Exp;
void exp_load()
{
    exps = list_create( LIST_AUTO_DELETE, LIST_NO_CALLBACK );
    exp_pic = load_surf( "explosion.bmp", SDL_SWSURFACE );
    exp_w = exp_h = exp_pic->h;
    exp_frame_count = exp_pic->w / exp_w;
}
void exps_clear()
{
    list_clear( exps );
}
void exp_delete()
{
    if ( exp_pic )
	    SDL_FreeSurface( exp_pic );
    exp_pic = 0;
    if ( exps )
	    list_delete( exps );
    exps = 0;
}
void exp_create( int x, int y )
{
    Exp *exp = calloc( 1, sizeof( Exp ) );
    exp->x = x - ( exp_w >> 1 );
    exp->y = y - ( exp_h >> 1 );
    exp->alpha = 0;
    list_add( exps, exp );
}
void exps_hide()
{
    Exp *exp;
    list_reset( exps );
    while ( ( exp = list_next( exps ) ) ) {
        DEST( sdl.screen, exp->x, exp->y, exp_w, exp_h );
        SOURCE( offscreen, exp->x, exp->y );
        blit_surf();
        add_refresh_rect( exp->x, exp->y, exp_w, exp_h );
    }
}
void exps_show()
{
    Exp *exp;
    list_reset( exps );
    while ( ( exp = list_next( exps ) ) ) {
        DEST( sdl.screen, exp->x, exp->y, exp_w, exp_h );
        SOURCE( exp_pic, (int)exp->cur * exp_w, 0 );
        alpha_blit_surf( exp->alpha );
    }
}
void exps_update( int ms )
{
    Exp *exp;
    list_reset( exps );
    while ( ( exp = list_next( exps ) ) ) {
        if ( exp->alpha < 255 ) {
            exp->alpha += exp_alpha_change * ms;
            if ( exp->alpha > 255 ) exp->alpha = 255;
        }
        exp->cur += exp_change * ms;
        if ( exp->cur >= exp_frame_count )
            list_delete_current( exps );
    }    
}
