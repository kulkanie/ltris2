/***************************************************************************
                          sdl.c  -  description
                             -------------------
    begin                : Thu Apr 20 2000
    copyright            : (C) 2000 by Michael Speck
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

/** Empty wrappers for old SDL1. Tetris core in libgame runs without any
 * access to SDL. This is done by the View object. */

//#include <SDL.h>
#include <stdlib.h>
#include <string.h>
#include "sdl.h"

#ifdef USE_PNG
#include <png.h>
#endif

extern int  term_game;

Sdl sdl;
SDL_Cursor *empty_cursor = 0;
SDL_Cursor *std_cursor = 0;
SDL_Joystick *gamepad = 0;
int gamepad_numbuttons = 0;
Uint8 gamepad_state[GPAD_LAST1];
Uint8 gamepad_oldstate[GPAD_LAST1];

/* shadow surface stuff */
int use_shadow_surface = 0;
SDL_Surface *video_surface = 0; /* sdl.screen is just regular surface
				with shadow surface. this is the real screen */
int video_scale = 0; /* factors: 0 = 1, 1 = 1.5, 2 = 2 */
int video_xoff = 0, video_yoff = 0; /* offset of scaled surface in display */
int video_sw = 0, video_sh = 0; /* size of scaled shadow surface */
int display_w = 0, display_h = 0; /* original size of display */
int video_forced_w = 0, video_forced_h = 0; /* given by command line */

/*
====================================================================
Default video modes. The first value is the id and indicates
if a mode is a standard video mode. If the mode was created by
directly by video_mode() this id is set to -1. The very last
value indicates if this is a valid mode and is checked by
init_sdl(). Init_sdl sets the available desktop bit depth.
====================================================================
*/
int const mode_count = 2;
Video_Mode modes[] = {
    { 0, "640x480x16 Window",     640, 480, BITDEPTH, SDL_SWSURFACE, 0 },
    { 1, "640x480x16 Fullscreen", 640, 480, BITDEPTH, SDL_SWSURFACE | SDL_FULLSCREEN, 0 },
};
Video_Mode *def_mode = &modes[0]; /* default resolution */
Video_Mode cur_mode; /* current video mode set in set_video_mode */

/* timer */
int cur_time, last_time;

/* sdl surface */

#ifdef USE_PNG
/* loads an image from png file and returns surface
 * or NULL in case of error;
 * you can get additional information with SDL_GetError
 *
 * stolen from SDL_image:
 *
 * Copyright (C) 1999  Sam Lantinga
 *
 * Sam Lantinga
 * 5635-34 Springhouse Dr.
 * Pleasanton, CA 94588 (USA)
 * slouken@devolution.com
 */
SDL_Surface *load_png( const char *file )
{
	FILE		*volatile fp = NULL;
	SDL_Surface	*volatile surface = NULL;
	png_structp	png_ptr = NULL;
	png_infop	info_ptr = NULL;
	png_bytep	*volatile row_pointers = NULL;
	png_uint_32	width, height;
	int		bit_depth, color_type, interlace_type;
	Uint32		Rmask;
	Uint32		Gmask;
	Uint32		Bmask;
	Uint32		Amask;
	SDL_Palette	*palette;
	int		row, i;
	volatile int	ckey = -1;
	png_color_16	*transv;

	/* create the PNG loading context structure */
	png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING,
					  NULL, NULL, NULL );
	if( png_ptr == NULL ) {
		SDL_SetError( "Couldn't allocate memory for PNG file" );
		goto done;
	}

	/* allocate/initialize the memory for image information.  REQUIRED. */
	info_ptr = png_create_info_struct( png_ptr );
	if( info_ptr == NULL ) {
		SDL_SetError( "Couldn't create image information for PNG file" );
		goto done;
	}

	/* set error handling if you are using setjmp/longjmp method (this is
	 * the normal method of doing things with libpng).  REQUIRED unless you
	 * set up your own error handlers in png_create_read_struct() earlier.
	 */
	if( setjmp( png_ptr->jmpbuf ) ) {
		SDL_SetError( "Error reading the PNG file." );
		goto done;
	}

	/* open file */
	fp = fopen( file, "r" );
	if( fp == NULL ) {
		SDL_SetError( "Could not open png file." );
		goto done;
	}
	png_init_io( png_ptr, fp );

	/* read PNG header info */
	png_read_info( png_ptr, info_ptr );
	png_get_IHDR( png_ptr, info_ptr, &width, &height, &bit_depth,
			&color_type, &interlace_type, NULL, NULL );

	/* tell libpng to strip 16 bit/color files down to 8 bits/color */
	png_set_strip_16( png_ptr );

	/* extract multiple pixels with bit depths of 1, 2, and 4 from a single
	 * byte into separate bytes (useful for paletted and grayscale images).
	 */
	png_set_packing( png_ptr );

	/* scale greyscale values to the range 0..255 */
	if( color_type == PNG_COLOR_TYPE_GRAY )
		png_set_expand( png_ptr );

	/* for images with a single "transparent colour", set colour key;
	   if more than one index has transparency, use full alpha channel */
	if( png_get_valid( png_ptr, info_ptr, PNG_INFO_tRNS ) ) {
	        int num_trans;
		Uint8 *trans;
		png_get_tRNS( png_ptr, info_ptr, &trans, &num_trans,
			      &transv );
		if( color_type == PNG_COLOR_TYPE_PALETTE ) {
			if( num_trans == 1 ) {
				/* exactly one transparent value: set colour key */
				ckey = trans[0];
			} else
				png_set_expand( png_ptr );
		} else
		    ckey = 0; /* actual value will be set later */
	}

	if( color_type == PNG_COLOR_TYPE_GRAY_ALPHA )
		png_set_gray_to_rgb( png_ptr );

	png_read_update_info( png_ptr, info_ptr );

	png_get_IHDR( png_ptr, info_ptr, &width, &height, &bit_depth,
			&color_type, &interlace_type, NULL, NULL );

	/* allocate the SDL surface to hold the image */
	Rmask = Gmask = Bmask = Amask = 0 ;
	if( color_type != PNG_COLOR_TYPE_PALETTE ) {
		if( SDL_BYTEORDER == SDL_LIL_ENDIAN ) {
			Rmask = 0x000000FF;
			Gmask = 0x0000FF00;
			Bmask = 0x00FF0000;
			Amask = (info_ptr->channels == 4) ? 0xFF000000 : 0;
		} else {
		        int s = (info_ptr->channels == 4) ? 0 : 8;
			Rmask = 0xFF000000 >> s;
			Gmask = 0x00FF0000 >> s;
			Bmask = 0x0000FF00 >> s;
			Amask = 0x000000FF >> s;
		}
	}
	surface = SDL_AllocSurface( SDL_SWSURFACE, width, height,
			bit_depth * info_ptr->channels, Rmask, Gmask, Bmask, Amask );
	if( surface == NULL ) {
		SDL_SetError( "Out of memory" );
		goto done;
	}

	if( ckey != -1 ) {
		if( color_type != PNG_COLOR_TYPE_PALETTE )
			/* FIXME: should these be truncated or shifted down? */
			ckey = SDL_MapRGB( surface->format,
					   (Uint8)transv->red,
					   (Uint8)transv->green,
					   (Uint8)transv->blue );
		SDL_SetColorKey( surface, SDL_SRCCOLORKEY, ckey );
	}

	/* create the array of pointers to image data */
	row_pointers = (png_bytep*)malloc( sizeof( png_bytep ) * height );
	if( ( row_pointers == NULL ) ) {
		SDL_SetError( "Out of memory" );
		SDL_FreeSurface( surface );
		surface = NULL;
		goto done;
	}
	for( row = 0; row < (int)height; row++ ) {
		row_pointers[row] = (png_bytep)
				(Uint8*)surface->pixels + row * surface->pitch;
	}

	/* read the entire image in one go */
	png_read_image( png_ptr, row_pointers );

	/* read rest of file, get additional chunks in info_ptr - REQUIRED */
	png_read_end( png_ptr, info_ptr );

	/* load the palette, if any */
	palette = surface->format->palette;
	if( palette ) {
	    if( color_type == PNG_COLOR_TYPE_GRAY ) {
		palette->ncolors = 256;
		for( i = 0; i < 256; i++ ) {
		    palette->colors[i].r = i;
		    palette->colors[i].g = i;
		    palette->colors[i].b = i;
		}
	    } else if( info_ptr->num_palette > 0 ) {
		palette->ncolors = info_ptr->num_palette;
		for( i = 0; i < info_ptr->num_palette; ++i ) {
		    palette->colors[i].b = info_ptr->palette[i].blue;
		    palette->colors[i].g = info_ptr->palette[i].green;
		    palette->colors[i].r = info_ptr->palette[i].red;
		}
	    }
	}

done:
	/* clean up and return */
	png_destroy_read_struct( &png_ptr, info_ptr ? &info_ptr : (png_infopp)0,
								  (png_infopp)0 );
	if( row_pointers )
		free( row_pointers );
	if( fp )
		fclose( fp );

	return surface;
}
#endif

/* return full path of bitmap */
void get_full_bmp_path( char *full_path, char *file_name )
{
    sprintf(full_path,  "%s/gfx/%s", SRC_DIR, file_name );
}

/*
    load a surface from file putting it in soft or hardware mem
*/
SDL_Surface* load_surf(char *fname, int f)
{
	/** create small dummy surface to allow access of surface->w/h */
	return SDL_CreateRGBSurface(0,10,10,24,0xff000000,0x00ff0000,0x0000ff00,0x000000ff);
}

/*
    create an surface
    MUST NOT BE USED IF NO SDLSCREEN IS SET
*/
SDL_Surface* create_surf(int w, int h, int f)
{
	/** create small dummy surface to allow access of surface->w/h */
	return SDL_CreateRGBSurface(0,10,10,24,0xff000000,0x00ff0000,0x0000ff00,0x000000ff);
}

/*
 * Free a surface if != NULL and set pointer to NULL
 */
void free_surf(SDL_Surface **surf)
{
	if (*surf)
		SDL_FreeSurface(*surf);
	*surf = 0;
}
/*
    lock surface
*/
void lock_surf(SDL_Surface *sur)
{
}

/*
    unlock surface
*/
void unlock_surf(SDL_Surface *sur)
{
}

/*
    blit surface with destination DEST and source SOURCE using it's actual alpha and color key settings
*/
void blit_surf(void)
{
}

/*
    do an alpha blit
*/
void alpha_blit_surf(int alpha)
{
}

/*
    fill surface with color c
*/
void fill_surf(int c)
{
}

/* set clipping rect */
void set_surf_clip( SDL_Surface *surf, int x, int y, int w, int h )
{
}

/* set pixel */
Uint32 set_pixel( SDL_Surface *surf, int x, int y, int pixel )
{
}

/* get pixel */
Uint32 get_pixel( SDL_Surface *surf, int x, int y )
{
    return 0;
}

/* draw a shadowed frame and darken contents which starts at cx,cy */
void draw_3dframe( SDL_Surface *surf, int cx, int cy, int w, int h, int border )
{
}

/* sdl font */

/* return full font path */
void get_full_font_path( char *path, char *file_name )
{
    strcpy( path, file_name );
/*    sprintf(path, "./gfx/fonts/%s", file_name ); */
}

/*
    load a font using the width values in the file
*/
Font* load_font(char *fname)
{
	return (Font*)(calloc(1,sizeof(Font)));
}

/*
    load a font with fixed size
*/
Font *load_fixed_font(char *f, int off, int len, int w)
{
	return (Font*)(calloc(1,sizeof(Font)));
}

/*
    free memory
*/
void free_font(Font **fnt)
{
	if (*fnt)
		free(*fnt);
	*fnt = 0;
}

/*
    write something with transparency
*/
int write_text(Font *fnt, SDL_Surface *dest, int x, int y, char *str, int alpha)
{
    return 0;
}

/*
    lock font surface
*/
void lock_font(Font *fnt)
{
}

/*
    unlock font surface
*/
void unlock_font(Font *fnt)
{
}
	
/*
    return last update region
*/
SDL_Rect last_write_rect(Font *fnt)
{
    SDL_Rect    rect={0,0,0,0};
    return rect;
}

/*
    return the text width in pixels
*/
int text_width(Font *fnt, char *str)
{
    return 0;
}

/* sdl */

/*
    initialize sdl
*/
void init_sdl( int f )
{
	/* create dummy surface for "screen" access by libgame */
	sdl.screen = SDL_CreateRGBSurface(0,10,10,24,0xff000000,0x00ff0000,0x0000ff00,0x000000ff);
	video_surface = sdl.screen;
}

/*
    free screen
*/
void quit_sdl()
{
	/* free dummy surface for "screen" access by libgame */
	free_surf(&sdl.screen);
}

/*
====================================================================
Get a verified video mode.
====================================================================
*/
Video_Mode def_video_mode()
{
    return *def_mode;
}
Video_Mode std_video_mode( int id )
{
    return modes[id];
}
Video_Mode video_mode( int width, int height, int depth, int flags )
{
    return def_video_mode();
}
/*
====================================================================
Current video mode.
====================================================================
*/
Video_Mode* cur_video_mode()
{
    return &cur_mode;
}
/*
====================================================================
Get a list with all valid standard mode names.
====================================================================
*/
char** get_mode_names( int *count )
{
    char **lines;
    int i, j;

    *count = 0;
    for ( i = 0; i < mode_count; i++ )
        if ( modes[i].ok )
            (*count)++;
    lines = calloc( *count, sizeof( char* ) );
    for ( i = 0, j = 0; i < mode_count; i++ )
        if ( modes[i].ok )
            lines[j++] = strdup( modes[i].name );
    return lines;
}

/** Find best resolution for shadow surface */
static void select_best_video_mode(int *best_w, int *best_h)
{
	*best_w = 640;
	*best_h = 480;
}

/*
====================================================================
Switch to passed video mode.
XXX we use 640x480 here directly as it is hardcoded to this
resolution anyways.
====================================================================
*/
int	set_video_mode( int fullscreen )
{
    return 0;
}

/*
    show hardware capabilities
*/
void hardware_cap()
{
}

/** Scale src to dst. Only works with factors 1,1.5,2 */
static void scale_surface(SDL_Surface *src, SDL_Surface *dst)
{
}

/** Render shadow surface sdl.screen to sdl.real_screen with zoom. */
void render_shadow_surface() {
}

/*
    update rectangle (0,0,0,0)->fullscreen
*/
void refresh_screen(int x, int y, int w, int h)
{
}

/*
    draw all update regions
*/
void refresh_rects()
{
}

/*
    add update region
*/
void add_refresh_rect(int x, int y, int w, int h)
{
}

/*
    fade screen to black
*/
void dim_screen(int steps, int delay, int trp)
{
}

/*
    undim screen
*/
void undim_screen(int steps, int delay, int trp)
{
}

/*
    wait for a key
*/
int wait_for_key()
{
}

/*
    wait for a key or mouse click
*/
void wait_for_click()
{
}

/*
    lock surface
*/
void lock_screen()
{
}

/*
    unlock surface
*/
void unlock_screen()
{
}

/*
    flip hardware screens (double buffer)
*/
void flip_screen()
{
}

/* cursor */

/* creates cursor */
SDL_Cursor* create_cursor( int width, int height, int hot_x, int hot_y, char *source )
{
}

/*
    get milliseconds since last call
*/
int get_time()
{
    return 0;
}

/*
    reset timer
*/
void reset_timer()
{
}

void fade_screen( int type, int length )
{
}

void take_screenshot( int i )
{
}

void gamepad_open()
{
}
int gamepad_opened()
{
	return 0;
}
void gamepad_close()
{
}
const Uint8 *gamepad_update()
{
	return gamepad_state;
}

int gamepad_ctrl_isdown(uint cid)
{
	return 0;
}
int gamepad_ctrl_ispressed(uint cid)
{
	return 0;
}
int gamepad_ctrl_isactive(uint cid)
{
	return 0;
}
