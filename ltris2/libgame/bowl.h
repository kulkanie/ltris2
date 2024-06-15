/***************************************************************************
                          bowl.h  -  description
                             -------------------
    begin                : Tue Dec 25 2001
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
 
#ifndef __BOWL_H
#define __BOWL_H

#include "ltris.h"

/* control states */
enum {
	CS_RELEASED = 0, /* continuously released */
	CS_PRESSED, /* continuously pressed */
	CS_DOWN, /* just pressed down */
	CS_UP /* just released */
};

/* bowl controls */
typedef struct {
	int lshift, rshift;
	int lrot, rrot;
	int sdrop, hdrop;
	int hold;
} BowlControls;

typedef struct {
    int sx, sy; /* screen position */
    int sw, sh; /* tile size in pixels */
    int x, y; /* map position */
    float cur_y; /* float pixel y position IN bowl */
    int id; /* picture&structure id */
    int rot_id; /* 0 - 3 rotation positions */
} Block;

typedef struct {
	int transition; /* lines until first transition */
	int pieces; /* pieces placed */
	int i_pieces; /* how many i pieces */
	int cleared[4]; /* singles, doubles, triples, tetris */
	int tetris_rate; /* how many lines in tetrises */
	int droughts; /* number of droughts */
	int max_drought; /* max length of droughts */
	int sum_droughts; /* sum of all droughts for average */
} BowlStats;

typedef struct {
    int mute; /* if mute no sounds are played */
    int blind; /* if this is true all graphical stuff called in a function not 
                  ending with hide/show is disabled. */
    Font *font;
    int sx, sy; /* screen position of very first block tile */
    int sw, sh; /* screen size */
    int w, h; /* measurements in blocks */
    int block_size; /* blocksize in pixels */
    int das_charge; /* current charge in ms */
    int das_maxcharge; /* maximum charge in ms */
    int das_drop; /* das charge drop if shifting piece */
    int are; /* in ms, if > 0 next piece is blocked until delay times out */
    int ldelay_max; /* time on collision until piece is inserted */
    int ldelay_cur; /* current value if > 0 */
    SDL_Surface *blocks; /* pointer to the block graphics */
    SDL_Surface *unknown_preview; /* if preview's unknown this is displayed */
    char name[32]; /* player's name for this bowl */
    Counter score; /* score gained by this player */
    int level; /* level to which player has played (starts at 0) */
    int levelup; /* 1 if last clear caused level up */
    int firstlevelup_lines; /* number of lines needed for first level up */
    int lines; /* number of cleared lines in total */
    int cleared_line_y[4]; /* line indices of cleared lines for last insertion */
    int cleared_line_count; /* how many lines where cleared */
    int use_figures; /* draw a figure each new level? */
    int add_lines, add_tiles; /* add lines or tiles after time out? */
    int add_line_holes; /* number of holes in added line */
    int dismantle_saves; /* if a line was removed the delay is reset */
    Delay add_delay; /* delay until next add action */
    int contents[BOWL_WIDTH][BOWL_HEIGHT]; /* indices of blocks or -1 */
    Block block;/* current block */
    int next_block_id; /* id of next block */
    int use_same_blocks; /* use global block list? */
    int next_blocks_pos; /* position in tetris next_blocks for 
                            mulitplayer games */
    float block_vert_vel; /* velocity per ms */
    float block_drop_vel;
    int score_sx, score_sy, score_sw, score_sh; /* region with score and lines/level */
    int game_over; /* set if bowl is filled */
    int hide_block; /* block ain't updated */
    int paused;
    int draw_contents; /* set if bowl needs a full redraw next bowl_show() */
    int help_sx, help_sy, help_sw, help_sh; /* position of helping shadow */
    int sdrop_pressed; /* 1 if soft drop in current cycle, 0 otherwise */

    int preview; /* 0 = no preview, otherwise number of pieces */
    int preview_sx, preview_sy; /* preview position */
    int preview_sw, preview_sh; /* preview size */

    int hold_active; /* whether hold can be used for this bowl */
    int hold_id; /* block id or -1 if none */
    int hold_used; /* true if hold was used, gets reset when block is inserted */
    int hold_sx, hold_sy, hold_sw, hold_sh; /* region with hold block */

    int cpu_player; /* if 1 cpu controlled */
    int cpu_dest_x; /* move block to this position (computed in bowl_select_next_block() */
    int cpu_dest_rot; /* destination rotation */
    int cpu_dest_score; /* AI score */
    Delay cpu_delay; /* CPU delay before moving down fast */
    Delay cpu_rot_delay; /* rotation delay of CPU */
    int cpu_down; /* move down fast? flag is set when delay expires */
#ifdef SOUND
    Sound_Chunk *wav_leftright;
    Sound_Chunk *wav_explosion;
    Sound_Chunk *wav_stop;
    Sound_Chunk *wav_nextlevel;
    Sound_Chunk *wav_excellent;
#endif

    /* statistics */
    int show_stats;
    int stats_x, stats_y, stats_w, stats_h;
    BowlStats stats;
    int drought; /* current drought: pieces since last i piece */

    /* training */
    int zero_gravity;
} Bowl;

/*
====================================================================
Load level figures from file.
====================================================================
*/
void bowl_load_figures();
/*
====================================================================
Initate block masks.
====================================================================
*/
void bowl_init_block_masks() ;
    
/*
====================================================================
Create a bowl at screen position x,y. Measurements are the same for
all bowls. Controls are the player's controls defined in config.c.
====================================================================
*/
Bowl *bowl_create( int x, int y,
		int preview_x, int preview_y,
		int hold_x, int hold_y,
		SDL_Surface *blocks, SDL_Surface *unknown_preview,
		char *name, Controls *controls );
void bowl_delete( Bowl *bowl );

/*
====================================================================
Finish game and set game over.
====================================================================
*/
void bowl_finish_game( Bowl *bowl, int winner );

/*
====================================================================
Hide/show/update all animations handled by a bowl.
If game_over only score is updated in bowl_update().
====================================================================
*/
void bowl_hide( Bowl *bowl );
void bowl_show( Bowl *bowl );
void bowl_update( Bowl *bowl, int ms, BowlControls *bc, int game_over );

/*
====================================================================
Draw a single bowl tile.
====================================================================
*/
void bowl_draw_tile( Bowl *bowl, int i, int j );

/*
====================================================================
Draw bowl contents to offscreen and screen.
====================================================================
*/
void bowl_draw_contents( Bowl *bowl );

/*
====================================================================
Draw frames and fix text to bkgnd.
====================================================================
*/
void bowl_draw_frames( Bowl *bowl );

/*
====================================================================
Toggle pause of bowl.
====================================================================
*/
void bowl_toggle_pause( Bowl *bowl );

/** Switch current and hold piece (or store piece and get next piece if
 * hold is empty). If switching pieces ends the game bowl->game_over is
 * set and can be checked afterwards to finish the game. */
void bowl_use_hold(Bowl *bowl);

/*
====================================================================
Play an optimized mute game. (used for stats)
====================================================================
*/
void bowl_quick_game( Bowl *bowl, CPU_ScoreSet *bscores, int llimit );

/*
====================================================================
Actually insert block and remove a line if needed,
create shrapnells, give score etc
If game is over only insert block.
====================================================================
*/
void bowl_insert_block( Bowl *bowl );

void bowl_draw_stats(Bowl *bowl);

void bowl_toggle_gravity(Bowl *bowl);

#endif
