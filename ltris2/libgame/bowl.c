/***************************************************************************
                          bowl.c  -  description
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

#include "ltris.h"
#include "config.h"
#include "tools.h"
#include "shrapnells.h"
#include "cpu.h"
#include "bowl.h"

extern Config config;
extern Sdl sdl;
extern SDL_Surface *offscreen;
extern SDL_Surface *bkgnd;
extern SDL_Surface *previewpieces;
extern Bowl *bowls[BOWL_COUNT];

enum {
	POSVALID = 0,
	POSINVAL = -1,
	POSINVAL_LEFT = -2,
	POSINVAL_RIGHT = -3,
	POSINVAL_BOTTOM = -4,
};

enum { FIGURE_COUNT = 21 };
int figures[FIGURE_COUNT][BOWL_WIDTH][BOWL_HEIGHT];

Block_Mask block_masks[BLOCK_COUNT];

extern Bowl *bowls[BOWL_COUNT];
extern int  *next_blocks, next_blocks_size;

/*
====================================================================
Locals
====================================================================
*/

/*
====================================================================
Get speed according to level of bowl.
====================================================================
*/
void bowl_set_vert_block_vel( Bowl *bowl )
{
	int base60[] = {
		48,43,38,33,28,23,18,13,8,6,
		5,5,5,4,4,4,3,3,3,2,
		2,2,2,2,2,2,2,2,2,1 };
	float ms = 1000.0 / 60.0; /* milliseconds per grid cell */
	if (bowl->level < 29)
		ms = 1000.0 * (float)(base60[bowl->level]) / 60.0;
	bowl->block_vert_vel = (float)(bowl->block_size) / ms;
	//printf( "Level %2i: %2.5f\n", i, bowl->block_vert_vel );

    /* set add action info for game mode figure (2) */
    if ( config.gametype == 2 ) {
        bowl->dismantle_saves = 1;
        /* 7 - 12 single tiles */
        if ( bowl->level >= 7 && bowl->level <= 12 ) {
            delay_set( &bowl->add_delay, 2000 + ( 12 - bowl->level ) * 500 );
            bowl->add_lines = 0;
            bowl->add_tiles = 1;
        }
        /* 13 - ... whole lines */
        if ( bowl->level >= 13 ) {
            if ( bowl->level <= 20 )
                delay_set( &bowl->add_delay, 2000 + ( 20 - bowl->level ) * 500 );
            else
                delay_set( &bowl->add_delay, 2000 );
            bowl->add_lines = 1;
            bowl->add_tiles = 0;
            bowl->add_line_holes = bowl->level - 12;
            if ( bowl->add_line_holes > 6 )
                bowl->add_line_holes = 6;
        }
    }
}

/*
====================================================================
Get position of helping shadow of block.
====================================================================
*/
int bowl_block_pos_is_valid( Bowl *bowl, int x, int y )
{
    int i, j;
    for ( i = 0; i < 4; i++ )
        for ( j = 0; j < 4; j++ )
            if ( block_masks[bowl->block.id].mask[bowl->block.rot_id][i][j] )
                if ( x + i >= 0 && x + i < bowl->w ) {
                    if ( y + j >= bowl->h ) return 0;
                    if ( y + j >= 0 )
                        if ( bowl->contents[x + i][y + j] != -1 )
                            return 0;
                }        
    return 1;
}
void bowl_compute_help_pos( Bowl *bowl )
{
    int j = bowl->block.y;
    while ( bowl_block_pos_is_valid( bowl, bowl->block.x, j ) ) j++;
    j--;
    bowl->help_sx = bowl->block.x * bowl->block_size + bowl->sx;
    bowl->help_sy = j * bowl->block_size + bowl->sy;
}

/*
====================================================================
Compute computer target
====================================================================
*/
void bowl_compute_cpu_dest( Bowl *bowl )
{
    int i, j;
    CPU_Data cpu_data;

    /* pass bowl contents to the cpu bowl */
    cpu_data.style = config.cpu_style;
    cpu_data.bowl_w = bowl->w;
    cpu_data.bowl_h = bowl->h;
    cpu_data.piece_id = bowl->block.id;
    cpu_data.preview_id = bowl->next_block_id;
    cpu_data.hold_active = bowl->hold_active;
    cpu_data.hold_id = bowl->hold_id;
    for ( i = 0; i < bowl->w; i++ )
        for ( j = 0; j < bowl->h; j++ )
            cpu_data.original_bowl[i][j] = ( bowl->contents[i][j] != -1 );

    /* use this hardcoded score set for eval
     * see explanation in tetris.c:tetris_test_cpu_algorithm() */
    if (config.cpu_style == CS_AGGRESSIVE) {
	    cpu_data.base_scores.lines = 15;
	    cpu_data.base_scores.holes = -28;
	    cpu_data.base_scores.slope = -2;
	    cpu_data.base_scores.abyss = -7;
	    cpu_data.base_scores.block = -5;
	    cpu_data.base_scores.clear = 16;
    } else {
	    cpu_data.base_scores.lines = 13;
	    cpu_data.base_scores.holes = -28;
	    cpu_data.base_scores.slope = -2;
	    cpu_data.base_scores.abyss = -7;
	    cpu_data.base_scores.block = -4;
	    cpu_data.base_scores.clear = 16;
    }

    /* get best destination */
    cpu_analyze_data(&cpu_data);
    if (cpu_data.use_hold)
    	bowl_use_hold(bowl);
    bowl->cpu_dest_x = cpu_data.result.x;
    bowl->cpu_dest_rot = cpu_data.result.rot;
    bowl->cpu_dest_score = cpu_data.result.score;
}

/** Initialize bowl's block structure to hold piece with id at top of bowl
 * and also compute position of shadow piece. */
void bowl_init_current_piece(Bowl *bowl, int id) {
	bowl->block.id = id;
	bowl->block.x = 5 - block_masks[bowl->block.id].rx;
	bowl->block.y = 0 - block_masks[bowl->block.id].ry;
	bowl->block.sx = bowl->sx + bowl->block_size * bowl->block.x;
	bowl->block.sy = bowl->sy + bowl->block_size * bowl->block.y;
	bowl->block.rot_id = block_masks[bowl->block.id].rstart;
	bowl->block.sw = 4 * bowl->block_size;
	bowl->block.sh = 4 * bowl->block_size;
	bowl->block.cur_y = bowl->block.y * bowl->block_size;
	bowl_compute_help_pos(bowl);
}

/*
====================================================================
Initiate next block. Set bowl::block::id to preview and get id of next
block to be dealt to bowl::next_block_id.
====================================================================
*/
void bowl_select_next_block( Bowl *bowl ) 
{
	int i, min, *new_next_blocks = 0;
	int num_old_blocks = 0;

	/* set preview as current block id */
	bowl->block.id = bowl->next_block_id;
	
	/* set new preview from next_blocks and fill more bags if end of
	 * buffer reached */
	bowl->next_block_id = next_blocks[bowl->next_blocks_pos++];
	DPRINTF("Bowl %p: Current = %d, Next = %d\n", bowl, bowl->block.id, 
							bowl->next_block_id);
	/* get new blocks before they run out so three piece preview will not crash */
	if ( bowl->next_blocks_pos >= next_blocks_size - 3 ) {
		DPRINTF("Need to fill new tetrominoes bags\n");
		/* we have to keep part of buffer which other bowls 
		 * might still need to go through */
		min = next_blocks_size;
		for ( i = 0; i < BOWL_COUNT; i++ )
			if ( bowls[i] && bowls[i]->next_blocks_pos < min )
				min = bowls[i]->next_blocks_pos;
		num_old_blocks = next_blocks_size - min;
			
		/* resize buffer and save old blocks */
		next_blocks_size = num_old_blocks + BLOCK_BAG_COUNT*BLOCK_COUNT;
		new_next_blocks = calloc(next_blocks_size, sizeof(int));
		if (num_old_blocks > 0) {
			memcpy( new_next_blocks, &next_blocks[min],
					sizeof(int) * num_old_blocks );
			DPRINTF("Keeping %d pieces: ", num_old_blocks);
			for (i = 0; i < num_old_blocks; i++)
				DPRINTF("%d ",new_next_blocks[i]);
			DPRINTF("\n");
		}
		free( next_blocks );
		next_blocks = new_next_blocks;
		
		/* adjust position in next_blocks for all bowls */
		for ( i = 0; i < BOWL_COUNT; i++ )
			if ( bowls[i] )
				bowls[i]->next_blocks_pos -= min;
			
		/* fill new bags */
		fill_random_block_bags( next_blocks + num_old_blocks, 
						BLOCK_BAG_COUNT, config.modern );
	}
    
	/* init rest of block structure */
	bowl_init_current_piece(bowl, bowl->block.id);

	/* count i pieces for drought and stats */
	if (bowl->block.id == 6) {
		/* check if this ends a drought */
		if (bowl->drought > 12) {
			bowl->stats.droughts++;
			if (bowl->drought > bowl->stats.max_drought)
				bowl->stats.max_drought = bowl->drought;
			bowl->stats.sum_droughts += bowl->drought;
		}
		bowl->stats.i_pieces++;
		bowl->drought = 0;
	} else
		bowl->drought++;

	/* if CPU is in control get destination row & other stuff */
	if (bowl->cpu_player) {
		/* destination */
		bowl_compute_cpu_dest( bowl );
		
		/* set delay until cpu waits with dropping block */
		delay_set( &bowl->cpu_delay, config.cpu_delay );
		bowl->cpu_down = 0;
		delay_set( &bowl->cpu_rot_delay, config.cpu_rot_delay );
	}

	/* check game over */
	if (bowl_check_piece_position(bowl,
			bowl->block.x, bowl->block.y,
			bowl->block.rot_id) != POSVALID)
		bowl_finish_game(bowl,0);
}

/*
====================================================================
Set a tile contents and pixel contents.
====================================================================
*/
void bowl_set_tile( Bowl *bowl, int x, int y, int tile_id )
{
    bowl->contents[x][y] = tile_id;
}

/*
====================================================================
Reset bowl contents and add levels figure if wanted.
====================================================================
*/
void bowl_reset_contents( Bowl *bowl )
{
    int i, j;
    for ( i = 0; i < bowl->w; i++ )
        for ( j = 0; j < bowl->h; j++ )
            bowl_set_tile( bowl, i, j, -1 );
    if ( bowl->use_figures && bowl->level < 20 /* don't have more figures */ )
        for ( i = 0; i < bowl->w; i++ )
            for ( j = 0; j < bowl->h; j++ )
                bowl_set_tile( bowl, i, j, figures[bowl->level][i][j] );
}

/** Check if current piece fits at position x,y with rotation rot.
 * Return whether position is valid, invalid or invalid because
 * left,right,lower boundary was hit.
 */
int bowl_check_piece_position(Bowl *bowl, int x, int y, int r)
{
	int i, j;
	for ( j = 0; j < 4; j++ )
		for ( i = 0; i < 4; i++ ) {
			if (!block_masks[bowl->block.id].mask[r][i][j])
				continue; /* empty tile */

			/* check boundaries */
			if (x + i < 0)
				return POSINVAL_LEFT;
			if (x + i >= bowl->w)
				return POSINVAL_RIGHT;
			if (y + j >= bowl->h)
				return POSINVAL_BOTTOM;
			if (y + j < 0)
				continue; /* at the top it's okay */

			/* check bowl */
			if (bowl->contents[x + i][y + j] != -1)
				return POSINVAL;
		}
	return POSVALID;
}

/** Check if current piece can drop one tile down. */
int bowl_piece_can_drop(Bowl *bowl)
{
	return (bowl_check_piece_position(bowl,
			bowl->block.x, bowl->block.y+1,
			bowl->block.rot_id) == POSVALID);
}

/*
====================================================================
Draw block to offscreen.
====================================================================
*/
void bowl_draw_block_to_offscreen( Bowl *bowl )
{
    int i, j;
    int tile_x = 0, tile_y = 0;
    
    if ( bowl->blind ) return;
    
    bowl->block.sx = bowl->block.x * bowl->block_size + bowl->sx;
    bowl->block.sy = bowl->block.y * bowl->block_size + bowl->sy;
    for ( j = 0; j < 4; j++ ) {
        for ( i = 0; i < 4; i++ ) {
            if ( block_masks[bowl->block.id].mask[bowl->block.rot_id][i][j] ) {
                DEST( offscreen, bowl->block.sx + tile_x, bowl->block.sy + tile_y, bowl->block_size, bowl->block_size );
                SOURCE( bowl->blocks, bowl->block.id * bowl->block_size, 0 );
                blit_surf();
            }
            tile_x += bowl->block_size;
        }
        tile_y += bowl->block_size;
        tile_x = 0;
    }
}

/*
====================================================================
Add a single tile at a random position so that it doesn't hit the 
current block nor leads to game over nor completes a line.
====================================================================
*/
void bowl_add_tile( Bowl *bowl )
{
    int j, k;
    int added = 0;
    int i = rand() % bowl->w;
    int checks = 0;
    int hole;
    while ( checks < 10 ) {
        i++; checks++;
        if ( i == bowl->w ) i = 0;
        /* get first free tile in column */
        if ( bowl->contents[i][1] != -1 )
            continue;
        else
            j = 1;
        while ( j + 1 < bowl->h && bowl->contents[i][j + 1] == -1 ) 
            j++;
        /* add tile and test if this hits the block if so remove and try another one */
        bowl_set_tile( bowl, i, j, 9 );
        if (bowl_check_piece_position(bowl,
        		bowl->block.x, bowl->block.y,
			bowl->block.rot_id) != POSVALID) {
            bowl_set_tile( bowl, i, j, -1 );
            continue;
        }
        /* if this line was completed deny, too */
        hole = 0;
        for ( k = 0; k < bowl->w; k++ )
            if ( bowl->contents[k][j] == -1 ) {
                hole = 1;
                break;
            }
        if ( !hole ) {
            bowl_set_tile( bowl, i, j, -1 );
            continue;
        }
        /* position worked out! */
        added = 1;
        bowl_draw_tile( bowl, i, j );
        break;
    }
    /* help shadow may have changed */
    if ( added )
        bowl_compute_help_pos( bowl );
}

/** Fill array @line of static length BOWL_WIDTH with random tiles and
 * @numholes holes (id = -1) in it.
 */
static void set_line(int *line, int numholes) {
	int holes[BOWL_WIDTH];

	/* set indices and random tiles */
	for (int i = 0; i < BOWL_WIDTH; i++) {
		holes[i] = i;
		line[i] = rand() % BLOCK_TILE_COUNT;
	}
	/* shuffle indices */
	for (int i = 0; i < BOWL_WIDTH*100; i++) {
		int pos1 = rand() % BOWL_WIDTH;
		int pos2 = rand() % BOWL_WIDTH;
		int aux = holes[pos1];
		holes[pos1] = holes[pos2];
		holes[pos2] = aux;
	}

	/* add holes according to first indices in list */
	for (int i = 0; i < numholes; i++) {
		line[holes[i]] = -1;
	}

	/* DEBUG printf("Line: ");
	for (int i = 0; i < BOWL_WIDTH; i++)
		printf("%d ", line[i]);
	printf("\n"); */
}

/*
====================================================================
Add a line at the bottom of a bowl and return False if bowl 
is filled to the top. If the block position becomes invalid by this
move it's positioned one up.
@wanted_holes is either the number of random gaps or -1 to use
array @line which contains tile ids including gaps.
====================================================================
*/
int bowl_add_line( Bowl * bowl, int wanted_holes, int *line)
{
    int newline[BOWL_WIDTH];
    int i, j = 0;

    if (bowl->game_over)
	    return 0;

    /* if the first line contains a tile the game is over! */
    for ( i = 0; i < bowl->w; i++ )
        if ( bowl->contents[i][0] != -1 )
            return 0;

    /* move all lines one up */
    for ( j = 0; j < bowl->h - 1; j++ )
        for ( i = 0; i < bowl->w; i++ )
            bowl_set_tile( bowl, i, j, bowl->contents[i][j + 1] );

    /* adjust cleared line indices as lines might be received during ARE
     * thus between inserting a block and collapsing the bowl. */
    if (bowl->cleared_line_count > 0)
	    for (int i = 0; i < bowl->cleared_line_count; i++)
		    bowl->cleared_line_y[i]--;

    if (wanted_holes == -1) {
	    /* use content of line */
	    for (i = 0; i < BOWL_WIDTH; i++)
		    newline[i] = line[i];
    } else {
	    /* generate random line */
	    set_line(newline, wanted_holes);
    }

    /* set new line at bottom of bowl */
    for (i = 0; i < BOWL_WIDTH; i++)
	    bowl_set_tile(bowl, i, BOWL_HEIGHT-1, newline[i]);

    /* check if block position became invalid */
    if (bowl_check_piece_position(bowl,
		    bowl->block.x, bowl->block.y,
		    bowl->block.rot_id) != POSVALID) {
        bowl->block.y -= 1;
        bowl->block.cur_y = bowl->block.y * bowl->block_size;
    }

    /* update helping shadow */
    bowl_compute_help_pos( bowl );

    return 1;
}

/*
====================================================================
Initate final animation.
====================================================================
*/
void bowl_final_animation( Bowl *bowl ) 
{
    int i, j, k;

    if ( bowl->blind ) return;
    
    for ( j = 0; j < bowl->h; j++ )
        for ( i = 0, k = 1; i < bowl->w; i++, k++ )
            if ( bowl->contents[i][j] != -1 )
                shrapnell_create( bowl->sx + i * bowl->block_size, bowl->sy + j * bowl->block_size, bowl->block_size, bowl->block_size, 0, -0.05, 0, 0.0002 );
}

/*
====================================================================
Finish game and set game over. If winner is 1 show win message.
====================================================================
*/
void bowl_finish_game( Bowl *bowl, int winner )
{
    char *msg = (winner)?_("Winner!"):_("Game Over");
    int mx = bowl->sx + bowl->sw / 2, my = bowl->sy + bowl->sh / 2;

    bowl->game_over = 1;
    bowl->hide_block = 1;
    bowl_final_animation( bowl );
    bowl->use_figures = 0;

    /* add stats for multiplayer game and move message down */
    if (config.gametype >= GAME_VS_HUMAN && config.gametype <= GAME_VS_CPU_CPU) {
	    bowl->stats_x = bowl->sx + BOWL_BLOCK_SIZE;
	    bowl->stats_y = bowl->sy + BOWL_BLOCK_SIZE;
	    bowl_draw_stats(bowl);
	    my = bowl->sy + bowl->sh - 3*BOWL_BLOCK_SIZE;
    }

    /* clear bowl and write message */
    bowl->font->align = ALIGN_X_CENTER | ALIGN_Y_CENTER;
    write_text(bowl->font, bkgnd, mx, my, msg, OPAQUE);
    bowl_reset_contents(bowl);
    bowl_draw_contents(bowl);
#ifdef SOUND
    if ( !bowl->mute ) sound_play( bowl->wav_explosion );
#endif    
}

/** Adjust bowl->score if lc lines have been cleared in current level. */
void bowl_add_score(Bowl *bowl, int lc)
{
	int score = 0;
	int base[5] = {0,40,100,300,1200};
	if (lc < 0)
		lc = 0;
	if (lc > 4)
		lc = 4; /* should not be possible */
	score = base[lc] * (bowl->level + 1);

	/* add 20% if no preview */
	if (!bowl->preview)
		score += 20 * score / 100;

	counter_add(&bowl->score, score);
}

/** Add lines to counter and check if new level has been entered. */
void bowl_add_lines( Bowl *bowl, int cleared)
{
	bowl->levelup = 0;

	/* check level up date, first level might need more than 10
	 * if higher starting level was chosen */
	if (bowl->lines < bowl->firstlevelup_lines) {
		if (bowl->lines + cleared >= bowl->firstlevelup_lines)
			bowl->levelup = 1;
	} else {
		int l = bowl->lines - bowl->firstlevelup_lines;
		if (l / 10 != (l + cleared) / 10)
			bowl->levelup = 1;
	}

	bowl->lines += cleared; /* increase lines count */

	if (bowl->levelup) {
		bowl->level++;
		bowl_set_vert_block_vel( bowl );
#ifdef SOUND
		if (!bowl->mute)
			sound_play( bowl->wav_nextlevel );
#endif
	}
}

/** collapse bowl so entirely empty lines are removed */
void bowl_collapse(Bowl *bowl)
{
	int i,j,l;
	for (j = 0; j < bowl->cleared_line_count; j++)
		for (i = 0; i < bowl->w; i++) {
			for (l = bowl->cleared_line_y[j]; l > 0; l--)
				bowl_set_tile(bowl, i, l,
						bowl->contents[i][l - 1]);
			bowl_set_tile(bowl, i, 0, -1);
		}
	bowl->cleared_line_count = 0;

	/* in game figures reset bowl contents */
	if (bowl->levelup && config.gametype == 2)
		bowl_reset_contents(bowl);

	/* collapse gets called after lines were removed with ARE.
	 * add_lines in insert_block checked if this caused a level up.
	 * so we always have to reset the flag here. */
	bowl->levelup = 0;
}

/*
====================================================================
Actually insert block and remove a line if needed, 
create shrapnells, give score etc
If game is over only insert block.
====================================================================
*/
void bowl_insert_block( Bowl *bowl )
{
  int i, j;
  int full;
  int send_count;
  int newline[BOWL_WIDTH]; /* tile id >= 0 or -1 for hole */
  int max_y; /* lowest block position in tiles */

  /* insert block */
  max_y = bowl->block.y;
  for ( i = 0; i < 4; i++ ) {
	  for ( j = 0; j < 4; j++ ) {
		  int tx = bowl->block.x + i;
		  int ty = bowl->block.y + j;
		  if (!block_masks[bowl->block.id].mask[bowl->block.rot_id][i][j])
			  continue;
		  if (ty < 0)
			  continue; /* may happen but does not end game */
		  if (tx < 0)
			  continue; /* should never happen */
		  if (tx >= bowl->w)
			  continue; /* should never happen */
		  if (ty >= bowl->h)
			  continue; /* should never happen */
		  bowl_set_tile( bowl, tx, ty,
				  block_masks[bowl->block.id].blockid );
		  if (max_y < ty)
			  max_y = ty;
	  }
  }
  bowl->stats.pieces++;

  if (bowl->game_over)
	  return;

  /* compute base are which is 10/60 s for rows 18,19
   * and +4/60 for each four rows less */
  if (max_y >= 18)
	  bowl->are = 10;
  else if (max_y >= 14)
	  bowl->are = 12;
  else if (max_y >= 10)
	  bowl->are = 14;
  else if (max_y >= 6)
	  bowl->are = 16;
  else
	  bowl->are = 18;

  /* draw block to offscreen for shrapnells */
  bowl_draw_contents( bowl );
#ifdef SOUND
  if ( !bowl->mute ) sound_play( bowl->wav_stop );
#endif    

  /* check for completed lines */
  bowl->cleared_line_count = 0;
  for ( j = 0; j < bowl->h; j++ ) {
    full = 1;
    for ( i = 0; i < bowl->w; i++ ) {
      if ( bowl->contents[i][j] == -1 ) {
	full = 0;
	break;
      }
    }
    if ( full )
      bowl->cleared_line_y[bowl->cleared_line_count++] = j;
  }
  if (bowl->cleared_line_count > 0)
	  bowl->stats.cleared[bowl->cleared_line_count-1]++;

  /* empty completed lines */
  for ( j = 0; j < bowl->cleared_line_count; j++)
    for ( i = 0; i < bowl->w; i++ )
      bowl_set_tile(bowl, i, bowl->cleared_line_y[j], -1);

  /* tetris? tell him! */
#ifdef SOUND
  if ( bowl->cleared_line_count == 4 )
    if ( !bowl->mute ) sound_play( bowl->wav_excellent );
#endif

  /* create shrapnells */
  if ( !bowl->blind )
    for ( j = 0; j < bowl->cleared_line_count; j++ )
      shrapnells_create( bowl->sx, bowl->sy + bowl->cleared_line_y[j] * bowl->block_size, bowl->sw, bowl->block_size );
#ifdef SOUND
  if ( bowl->cleared_line_count > 0 )
    if ( !bowl->mute ) sound_play( bowl->wav_explosion );
#endif

  /* animation adds 18/60 to are */
  if (bowl->cleared_line_count > 0)
    bowl->are += 18;
  /* translate are to ms */
  bowl->are = 1000 * bowl->are / 60;

  /* add lines and check level update */
  if (bowl->cleared_line_count > 0)
    bowl_add_lines(bowl, bowl->cleared_line_count);

  /* add score */
  bowl_add_score(bowl, bowl->cleared_line_count);

  /* reset delay of add_line/tile */
  if ( bowl->cleared_line_count && ( bowl->add_lines || bowl->add_tiles ) && bowl->dismantle_saves )
    delay_reset( &bowl->add_delay );

  /* update offscreen&screen */
  bowl->draw_contents = 1;

  /* send completed lines to all other bowls */
  if ( bowl->cleared_line_count > 1 ) {
      send_count = bowl->cleared_line_count;
      if ( !config.send_all )
	send_count--;
      if ( bowl->cleared_line_count == 4 && config.send_tetris )
	send_count = 4;

      for ( i = 0; i < BOWL_COUNT; i++ ) {
	      if (!bowls[i] || bowls[i] == bowl || bowls[i]->game_over)
		      continue;

	      set_line(newline, config.holes);

	      for (j = 0; j < send_count; j++)
		      if (!bowl_add_line( bowls[i],
				      	      (config.rand_holes)?config.holes:-1, newline)) {
			      bowl_finish_game( bowls[i], 0 );
			      break;
		      }
	      bowls[i]->draw_contents = 1;
      }
  }
}

/*
====================================================================
Drop block in one p-cycle but don't lock in place.
Happens next in update().
====================================================================
*/
void bowl_drop_block( Bowl *bowl )
{
	while (bowl_piece_can_drop(bowl)) {
		bowl->block.y++;
		bowl->block.cur_y = bowl->block.y * bowl->block_size;
	}
}

/*
====================================================================
Return True if CPU may drop/move down.
====================================================================
*/
int bowl_cpu_may_move_down( Bowl *bowl )
{
    if ( bowl->cpu_player && bowl->cpu_down &&
		    bowl->block.x == bowl->cpu_dest_x &&
		    bowl->block.rot_id == bowl->cpu_dest_rot )
        return 1;
    return 0;
}
int bowl_cpu_may_drop( Bowl *bowl )
{
/*    if ( config.cpu_diff == 5 && bowl_cpu_may_move_down( bowl ) )
        return 1;*/
    return 0;
}

/*
====================================================================
Publics
====================================================================
*/

/*
====================================================================
Load figures from file.
====================================================================
*/
void bowl_load_figures() {
    int i, j, k;
    FILE *file = 0;
    char path[512];
    char buf[128];
    sprintf( path, "%s/figures", SRC_DIR );
    if ( ( file = fopen( path, "r" ) ) == 0 ) {
        fprintf( stderr, "Cannot load figures from '%s'.\n", path );
        return;
    }
    for ( i = 0; i < FIGURE_COUNT; i++ )
        for ( j = 0; j < BOWL_HEIGHT; j++ ) {
            if ( feof( file ) ) {
                fprintf( stderr, "Unexpected end of file when trying to read line %i of figure %i in '%s'.\n", j, i, path );
                return;
            }
            if (fread(buf, sizeof(char), BOWL_WIDTH+1, file) != BOWL_WIDTH+1)
        	    fprintf(stderr,"%s:%d: Error reading figures\n",__FILE__,__LINE__);
            buf[BOWL_WIDTH] = 0;
            for ( k = 0; k < BOWL_WIDTH; k++ ) {
                if ( buf[k] == 32 )
                    figures[i][k][j] = -1;
                else
                    figures[i][k][j] = buf[k] - 97;
            }
        }
    fclose( file );
}
/*
====================================================================
Initate block masks.
====================================================================
*/
void bowl_init_block_masks() 
{
	int i;
	int masksize = sizeof(block_masks[0].mask); // same for all

	for (i = 0; i < 7; i++) {
		block_masks[i].rx = 2;
		block_masks[i].ry = 2;
		memset(block_masks[i].mask, 0, masksize );
	}

	/* T */
	block_masks[0].rstart = 2;
	block_masks[0].blockid = 2;
	/* Tu */
	block_masks[0].mask[0][2][1] = 1;
	block_masks[0].mask[0][1][2] = 1;
	block_masks[0].mask[0][2][2] = 1;
	block_masks[0].mask[0][3][2] = 1;
	/* Tr */
	block_masks[0].mask[1][2][1] = 1;
	block_masks[0].mask[1][2][2] = 1;
	block_masks[0].mask[1][3][2] = 1;
	block_masks[0].mask[1][2][3] = 1;
	/* Td */
	block_masks[0].mask[2][1][2] = 1;
	block_masks[0].mask[2][2][2] = 1;
	block_masks[0].mask[2][3][2] = 1;
	block_masks[0].mask[2][2][3] = 1;
	/* Tl */
	block_masks[0].mask[3][2][1] = 1;
	block_masks[0].mask[3][1][2] = 1;
	block_masks[0].mask[3][2][2] = 1;
	block_masks[0].mask[3][2][3] = 1;
	
	/* J */
	block_masks[1].rstart = 3;
	block_masks[1].blockid = 6;
	/* Jl */
	block_masks[1].mask[0][2][1] = 1;
	block_masks[1].mask[0][2][2] = 1;
	block_masks[1].mask[0][1][3] = 1;
	block_masks[1].mask[0][2][3] = 1;
	/* Ju */
	block_masks[1].mask[1][1][1] = 1;
	block_masks[1].mask[1][1][2] = 1;
	block_masks[1].mask[1][2][2] = 1;
	block_masks[1].mask[1][3][2] = 1;
	/* Jr */
	block_masks[1].mask[2][2][1] = 1;
	block_masks[1].mask[2][3][1] = 1;
	block_masks[1].mask[2][2][2] = 1;
	block_masks[1].mask[2][2][3] = 1;
	/* Jd */
	block_masks[1].mask[3][1][2] = 1;
	block_masks[1].mask[3][2][2] = 1;
	block_masks[1].mask[3][3][2] = 1;
	block_masks[1].mask[3][3][3] = 1;
	
	/* Z */
	block_masks[2].rstart = 0;
	block_masks[2].blockid = 4;
	/* Zh */
	block_masks[2].mask[0][1][2] = 1;
	block_masks[2].mask[0][2][2] = 1;
	block_masks[2].mask[0][2][3] = 1;
	block_masks[2].mask[0][3][3] = 1;
	/* Zv */
	block_masks[2].mask[1][3][1] = 1;
	block_masks[2].mask[1][2][2] = 1;
	block_masks[2].mask[1][3][2] = 1;
	block_masks[2].mask[1][2][3] = 1;
	/* Zh */
	block_masks[2].mask[2][1][2] = 1;
	block_masks[2].mask[2][2][2] = 1;
	block_masks[2].mask[2][2][3] = 1;
	block_masks[2].mask[2][3][3] = 1;
	/* Zv */
	block_masks[2].mask[3][3][1] = 1;
	block_masks[2].mask[3][2][2] = 1;
	block_masks[2].mask[3][3][2] = 1;
	block_masks[2].mask[3][2][3] = 1;
	
	/* O */
	block_masks[3].rstart = 0;
	block_masks[3].blockid = 0;
	block_masks[3].mask[0][1][2] = 1;
	block_masks[3].mask[0][2][2] = 1;
	block_masks[3].mask[0][1][3] = 1;
	block_masks[3].mask[0][2][3] = 1;
	block_masks[3].mask[1][1][2] = 1;
	block_masks[3].mask[1][2][2] = 1;
	block_masks[3].mask[1][1][3] = 1;
	block_masks[3].mask[1][2][3] = 1;
	block_masks[3].mask[2][1][2] = 1;
	block_masks[3].mask[2][2][2] = 1;
	block_masks[3].mask[2][1][3] = 1;
	block_masks[3].mask[2][2][3] = 1;
	block_masks[3].mask[3][1][2] = 1;
	block_masks[3].mask[3][2][2] = 1;
	block_masks[3].mask[3][1][3] = 1;
	block_masks[3].mask[3][2][3] = 1;
	
	/* S */
	block_masks[4].rstart = 0;
	block_masks[4].blockid = 3;
	/* Sh */
	block_masks[4].mask[0][2][2] = 1;
	block_masks[4].mask[0][3][2] = 1;
	block_masks[4].mask[0][1][3] = 1;
	block_masks[4].mask[0][2][3] = 1;
	/* Sv */
	block_masks[4].mask[1][2][1] = 1;
	block_masks[4].mask[1][2][2] = 1;
	block_masks[4].mask[1][3][2] = 1;
	block_masks[4].mask[1][3][3] = 1;
	/* Sh */
	block_masks[4].mask[2][2][2] = 1;
	block_masks[4].mask[2][3][2] = 1;
	block_masks[4].mask[2][1][3] = 1;
	block_masks[4].mask[2][2][3] = 1;
	/* Sv */
	block_masks[4].mask[3][2][1] = 1;
	block_masks[4].mask[3][2][2] = 1;
	block_masks[4].mask[3][3][2] = 1;
	block_masks[4].mask[3][3][3] = 1;
	
	/* L */
	block_masks[5].rstart = 1;
	block_masks[5].blockid = 5;
	/* Lr */
	block_masks[5].mask[0][2][1] = 1;
	block_masks[5].mask[0][2][2] = 1;
	block_masks[5].mask[0][2][3] = 1;
	block_masks[5].mask[0][3][3] = 1;
	/* Ld */
	block_masks[5].mask[1][1][2] = 1;
	block_masks[5].mask[1][2][2] = 1;
	block_masks[5].mask[1][3][2] = 1;
	block_masks[5].mask[1][1][3] = 1;
	/* Ll */
	block_masks[5].mask[2][1][1] = 1;
	block_masks[5].mask[2][2][1] = 1;
	block_masks[5].mask[2][2][2] = 1;
	block_masks[5].mask[2][2][3] = 1;
	/* Lu */
	block_masks[5].mask[3][3][1] = 1;
	block_masks[5].mask[3][1][2] = 1;
	block_masks[5].mask[3][2][2] = 1;
	block_masks[5].mask[3][3][2] = 1;
	
	/* I */
	block_masks[6].rstart = 1;
	block_masks[6].blockid = 1;
	/* Iv */
	block_masks[6].mask[0][2][0] = 1;
	block_masks[6].mask[0][2][1] = 1;
	block_masks[6].mask[0][2][2] = 1;
	block_masks[6].mask[0][2][3] = 1;
	/* Ih */
	block_masks[6].mask[1][0][2] = 1;
	block_masks[6].mask[1][1][2] = 1;
	block_masks[6].mask[1][2][2] = 1;
	block_masks[6].mask[1][3][2] = 1;
	/* Iv */
	block_masks[6].mask[2][2][0] = 1;
	block_masks[6].mask[2][2][1] = 1;
	block_masks[6].mask[2][2][2] = 1;
	block_masks[6].mask[2][2][3] = 1;
	/* Ih */
	block_masks[6].mask[3][0][2] = 1;
	block_masks[6].mask[3][1][2] = 1;
	block_masks[6].mask[3][2][2] = 1;
	block_masks[6].mask[3][3][2] = 1;
}

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
		char *name, Controls *controls )
{
    Bowl *bowl = calloc( 1, sizeof( Bowl ) );
    bowl->mute = 0;
    bowl->blind = 0;
    bowl->sx = x; bowl->sy = y;
    bowl->block_size = BOWL_BLOCK_SIZE;
    bowl->w = BOWL_WIDTH; bowl->h = BOWL_HEIGHT;
    bowl->sw = bowl->w * bowl->block_size; bowl->sh = bowl->h * bowl->block_size;
    bowl->blocks = blocks;
    bowl->unknown_preview = unknown_preview;
    strcpy( bowl->name, name );
    bowl->use_figures = ( config.gametype == 2 );
    /* we use controls only to determine cpu play.
     * actual controls are determined in tetris_set_bowl_controls */
    bowl->cpu_player = (controls == 0);

    if (config.gametype == 2) {
	    bowl->firstlevelup_lines = 10;
	    bowl->level = 0;
    } else {
	    /* starting lines to be cleared computes as
	     * min((slevel+1)*10,max(100,slevel*10-50)) */
	    int a = (config.starting_level+1) * 10;
	    int b = config.starting_level*10 - 50;
	    if (b < 100)
		    b = 100;
	    if (a < b)
		    bowl->firstlevelup_lines = a;
	    else
		    bowl->firstlevelup_lines = b;
	    bowl->level = config.starting_level;
    }

    bowl_reset_contents( bowl );
    bowl->next_block_id = next_blocks[bowl->next_blocks_pos++];
    /* translate config option vert_delay into drop_vel to avoid unnecessary
     * changes in the code. the lower the delay the higher the velocity. */
    bowl->block_drop_vel = 0.6; // fixed to 0.6; 0.8 - config.vert_delay*0.07;
    bowl_set_vert_block_vel( bowl );
    bowl->help_sw = bowl->help_sh = bowl->block_size * 4;
    if (!config.preview || preview_x == -1)
	    bowl->preview = 0;
    else if (config.modern)
	    bowl->preview = 3;
    else
	    bowl->preview = 1;
    bowl->preview_sx = preview_x;
    bowl->preview_sy = preview_y;
    bowl->preview_sw = bowl->block_size*4;
    bowl->preview_sh = bowl->block_size + bowl->block_size*3;
    if (config.modern) {
	    bowl->preview_sh += 2 * bowl->block_size*3;
	    /* passed position is for 1 piece so adjust */
	    bowl->preview_sy -= bowl->block_size*3;
    }
    bowl_select_next_block( bowl );
    bowl->font = load_fixed_font( "f_small_white.bmp", 32, 96, 8 );
#ifdef SOUND
    bowl->wav_leftright = sound_chunk_load( "leftright.wav" );
    bowl->wav_explosion = sound_chunk_load( "explosion.wav" );
    bowl->wav_stop      = sound_chunk_load( "stop.wav" );
    bowl->wav_nextlevel = sound_chunk_load( "nextlevel.wav" );
    bowl->wav_excellent = sound_chunk_load( "excellent.wav" );
#endif

    /* das charge, either class 16/6 or fast 10/3
    if (config.hyper_das) {
	    bowl->das_maxcharge = 167;
	    bowl->das_drop = 50;
    } else {
	    bowl->das_maxcharge = 267;
	    bowl->das_drop = 100;
    } */
    /* use configurable das */
    bowl->das_maxcharge = config.as_delay;
    bowl->das_drop = config.as_speed;
    bowl->das_charge = 0;

    /* lock delay */
    if (config.modern)
	    bowl->ldelay_max = 500;
    else
	    bowl->ldelay_max = 0;
    bowl->ldelay_cur = 0;

    /* hold settings */
    if (hold_x == -1 || !config.modern)
	    bowl->hold_active = 0;
    else {
	    bowl->hold_active = 1;
	    bowl->hold_id = -1;
	    bowl->hold_used = 0;
	    bowl->hold_sx = hold_x;
	    bowl->hold_sy = hold_y;
	    bowl->hold_sw = bowl->block_size*4;
	    bowl->hold_sh = bowl->block_size + bowl->block_size*3;
    }

    /* stats are only display for games with one bowl
     * which is determined in tetris.c:tetris_init()
     * XXX as it is only one bowl we hard code position here */
    bowl->show_stats = 0;
    bowl->stats_x = 30;
    bowl->stats_y = 60;
    bowl->stats_w = 160;
    bowl->stats_h = 280;

    return bowl;
}
void bowl_delete( Bowl *bowl )
{
    free_font( &bowl->font );
#ifdef SOUND
    if ( bowl->wav_excellent )
	    sound_chunk_free( bowl->wav_excellent );
    bowl->wav_excellent = 0;
    if ( bowl->wav_nextlevel )
	    sound_chunk_free( bowl->wav_nextlevel );
    bowl->wav_nextlevel = 0;
    if ( bowl->wav_stop )
	    sound_chunk_free( bowl->wav_stop );
    bowl->wav_stop = 0;
    if ( bowl->wav_leftright )
	    sound_chunk_free( bowl->wav_leftright );
    bowl->wav_leftright = 0;
    if ( bowl->wav_explosion )
	    sound_chunk_free( bowl->wav_explosion );
    bowl->wav_explosion = 0;
#endif
    free( bowl );
}

/*
====================================================================
Hide/show/update all animations handled by a bowl.
====================================================================
*/
void bowl_hide( Bowl *bowl )
{
    /* block */
    if ( !bowl->hide_block ) {
        DEST( sdl.screen, bowl->block.sx, bowl->block.sy, bowl->block.sw, bowl->block.sh );
        SOURCE( offscreen, bowl->block.sx, bowl->block.sy );
        blit_surf();
        add_refresh_rect( bowl->block.sx, bowl->block.sy, bowl->block.sw, bowl->block.sh );
    }        
    /* help */
    if ( config.modern ) {
        DEST( sdl.screen, bowl->help_sx, bowl->help_sy, bowl->help_sw, bowl->help_sh );
        SOURCE( offscreen, bowl->help_sx, bowl->help_sy );
        blit_surf();
        add_refresh_rect( bowl->help_sx, bowl->help_sy, bowl->help_sw, bowl->help_sh );
    }
    /* hold */
    if ( bowl->hold_active ) {
	    DEST( sdl.screen, bowl->hold_sx, bowl->hold_sy,
			    bowl->hold_sw, bowl->hold_sh);
	    SOURCE( offscreen, bowl->hold_sx, bowl->hold_sy);
	    blit_surf();
	    add_refresh_rect(bowl->hold_sx, bowl->hold_sy,
			    bowl->hold_sw, bowl->hold_sh);
    }
    /* preview */
    if ( bowl->preview ) {
        DEST( sdl.screen, bowl->preview_sx, bowl->preview_sy,
        		bowl->preview_sw, bowl->preview_sh);
        SOURCE( offscreen, bowl->preview_sx, bowl->preview_sy);
        blit_surf();
        add_refresh_rect(bowl->preview_sx, bowl->preview_sy,
        		bowl->preview_sw, bowl->preview_sh);
    }
    /* score */
    DEST( sdl.screen, bowl->score_sx, bowl->score_sy, bowl->score_sw, bowl->score_sh );
    SOURCE( offscreen, bowl->score_sx, bowl->score_sy );
    blit_surf();
    add_refresh_rect( bowl->score_sx, bowl->score_sy, bowl->score_sw, bowl->score_sh );
    /* stats */
    if (bowl->show_stats) {
	    DEST(sdl.screen, bowl->stats_x, bowl->stats_y, bowl->stats_w, bowl->stats_h);
	    SOURCE(offscreen, bowl->stats_x, bowl->stats_y);
	    blit_surf();
	    add_refresh_rect(bowl->stats_x, bowl->stats_y, bowl->stats_w, bowl->stats_h);
    }
}

void bowl_show( Bowl *bowl )
{
    int i, j;
    int x = bowl->block.sx, y = bowl->block.sy;
    int tile_x = 0, tile_y = 0;
    char aux[24];

    /* draw contents? */
    if ( bowl->draw_contents && !bowl->paused ) {
        bowl->draw_contents = 0;
        bowl_draw_contents( bowl );
    }

    /* piece & help */
    if ( !bowl->hide_block ) {
        for ( j = 0; j < 4; j++ ) {
            for ( i = 0; i < 4; i++ ) {
                if ( block_masks[bowl->block.id].mask[bowl->block.rot_id][i][j] ) {
                    /* help */
                    if ( config.modern && bowl->are == 0) {
                        DEST( sdl.screen, bowl->help_sx + tile_x, bowl->help_sy + tile_y, bowl->block_size, bowl->block_size );
                        SOURCE( bowl->blocks, block_masks[bowl->block.id].blockid * bowl->block_size, 0 );
                        alpha_blit_surf(160);
                        add_refresh_rect( bowl->help_sx + tile_x, bowl->help_sy + tile_y, bowl->block_size, bowl->block_size );
                    }
                    /* block */
                    if (bowl->are == 0) {
                	    int id = block_masks[bowl->block.id].blockid;
                	    /* use special block id to indicate lock delay
                	     * unless soft drop was used which will insert
                	     * the block on the next cycle */
                	    if (bowl->ldelay_cur > 0 && !bowl->sdrop_pressed)
                		    id = 10;
			    DEST( sdl.screen, x, y, bowl->block_size, bowl->block_size );
			    SOURCE( bowl->blocks,id * bowl->block_size, 0 );
			    blit_surf();
			    add_refresh_rect( x, y, bowl->block_size, bowl->block_size );
                    }
               }
                x += bowl->block_size;
                tile_x += bowl->block_size;
            }
            y += bowl->block_size;
            x = bowl->block.sx;
            tile_x = 0;
            tile_y += bowl->block_size;
        }
    }

    /* hold piece */
    if (bowl->hold_active) {
	    /* title */
	    bowl->font->align = ALIGN_X_CENTER | ALIGN_Y_CENTER;
	    write_text(bowl->font, sdl.screen,
			    bowl->hold_sx + bowl->hold_sw/2,
			    bowl->hold_sy + bowl->block_size/2,
			    _("Hold"), OPAQUE );

	    if (bowl->hold_id != -1) {
		    int hw = bowl->block_size*4, hh = bowl->block_size*2;
		    DEST( sdl.screen, bowl->hold_sx,
				    bowl->hold_sy + 3*bowl->block_size/2,
				    hw,hh);
		    SOURCE( previewpieces, 0, bowl->hold_id * hh );
		    blit_surf();
	    }

	    add_refresh_rect(bowl->hold_sx,bowl->hold_sy,bowl->block_size*4,bowl->block_size*3);
    }

    /* piece preview */
    if (bowl->preview) {
	    /* title */
	    bowl->font->align = ALIGN_X_CENTER | ALIGN_Y_CENTER;
	    write_text(bowl->font, sdl.screen,
			    bowl->preview_sx + bowl->preview_sw/2,
			    bowl->preview_sy + bowl->block_size/2,
			    _("Next"), OPAQUE );

	    /* first piece */
	    int pw = bowl->block_size*4, ph = bowl->block_size*2;
	    DEST( sdl.screen, bowl->preview_sx,
			    bowl->preview_sy + 3*bowl->block_size/2,
			    pw,ph);
	    SOURCE( previewpieces, 0, bowl->next_block_id * ph );
	    blit_surf();

	    /* only for modern show next two pieces of piece bag */
	    for (int i = 0; i < bowl->preview-1; i++) {
		    int pid = next_blocks[bowl->next_blocks_pos+i];
		    DEST( sdl.screen, bowl->preview_sx,
				    bowl->preview_sy + 3*bowl->block_size/2
				    	    + (ph + bowl->block_size)*(i+1),
				    pw,ph);
		    SOURCE( previewpieces, 0, pid * ph );
		    blit_surf();
	    }

	    add_refresh_rect(bowl->preview_sx,bowl->preview_sy,pw,ph);
    }

    /* score, lines, level */
    bowl->font->align = ALIGN_X_RIGHT | ALIGN_Y_TOP;
    sprintf( aux, "%.0f", counter_get_approach( bowl->score ) );
    write_text( bowl->font, sdl.screen, bowl->score_sx + bowl->score_sw - 4, bowl->score_sy + 1, aux, OPAQUE );
    bowl->font->align = ALIGN_X_RIGHT | ALIGN_Y_BOTTOM;
    sprintf( aux, _("%i Lvl: %i"), bowl->lines, bowl->level );
    write_text( bowl->font, sdl.screen, bowl->score_sx + bowl->score_sw - 4, bowl->score_sy + bowl->score_sh, aux, OPAQUE );

    /* stats */
    if (bowl->show_stats)
	    bowl_draw_stats(bowl);
}

/** Convert cur_y to y, for negative cur_y this means to subtract 1
 * because, e.g., pixels -1 to -20 means -1
 */
int bowl_convert_cury2y(Bowl *bowl)
{
	int y = (int)bowl->block.cur_y / bowl->block_size;
	if (bowl->block.cur_y < 0)
		if ( (((int)bowl->block.cur_y) % bowl->block_size) != 0)
			y -= 1;
	return y;
}

/** Check if a running lock delay is obsolete or lock delay
 * needs to be started for current block position.
 */
void bowl_check_lockdelay(Bowl *bowl)
{
	if (bowl->ldelay_max == 0)
		return;
	if (bowl->ldelay_cur > 0) {
		if (bowl_piece_can_drop(bowl)) {
			/* running lock delay obsolete, reset */
			bowl->ldelay_cur = 0;
			bowl->block.cur_y = bowl->block.y * bowl->block_size;
		}
	} else {
		if (!bowl_piece_can_drop(bowl)) {
			/* start lock delay */
			bowl->ldelay_cur = bowl->ldelay_max;
			bowl->block.cur_y = bowl->block.y * bowl->block_size;
		}
	}
}

void bowl_update( Bowl *bowl, int ms, BowlControls *bc, int game_over )
{
    int old_y = bowl_convert_cury2y(bowl);
    int hori_movement = 0;
    int new_rot, hori_mod, vert_mod, ret;
    float vy;

    /* SCORE */
    counter_update( &bowl->score, ms );

    if ( game_over )
	    return;

    /* soft drop used? */
    bowl->sdrop_pressed = (bc->sdrop == CS_PRESSED);

    /* ARE */
    if (bowl->are > 0) {
	    bowl->are -= ms;
	    if (bowl->are <= 0) {
		    bowl->are = 0;
		    /* kill input to keep DAS and prevent drops */
		    memset(bc,0,sizeof(BowlControls));
		    /* running are means block was inserted. when expired
		     * remove empty lines and get next block */
		    bowl_collapse(bowl);
		    bowl_select_next_block(bowl);
		    bowl->draw_contents = 1;
		    bowl->hold_used = 0; /* allow using hold again */
		    /* for simple modern version charge das if shift pressed */
		    if (config.modern)
			    bowl->das_charge = bowl->das_maxcharge;
	    }
    }

    /* BLOCK */
    if ( !bowl->hide_block && bowl->are == 0) {
	    /* handle controls */
	    if (bc->lshift == CS_DOWN || bc->rshift == CS_DOWN) {
		    /* try to instantly move and reset das charge.
		     * if shifting not possible, charge to the max */
		    bowl->das_charge = 0;
		    if (bc->lshift == CS_DOWN)
			    hori_mod = -1;
		    else
			    hori_mod = 1;
		    if (bowl_check_piece_position(bowl,
				    bowl->block.x + hori_mod,
				    bowl->block.y,
				    bowl->block.rot_id) != POSVALID)
			    bowl->das_charge = bowl->das_maxcharge;
		    else {
			    bowl->block.x += hori_mod;
			    hori_movement = 1;
			    bowl_check_lockdelay(bowl);
		    }
	    } else if (bc->lrot == CS_DOWN || bc->rrot == CS_DOWN) {
		    /* test if we actually can rotate
		     * if not shift block if modern and rotate anyways */
		    if (bc->lrot == CS_DOWN) {
			    new_rot = bowl->block.rot_id - 1;
			    if ( new_rot < 0 )
				    new_rot = 3;
		    } else {
			    new_rot = bowl->block.rot_id + 1;
			    if ( new_rot == 4 )
				    new_rot = 0;
		    }
		    /* the original wall kick rules allow for ridiculous
		     * moves so we go with a relatable simple version.
		     * it is tried to move the piece one to the right, one
		     * to the left and then one up (in this order). if
		     * any if these single steps fails the rotation fails.
		     */
		    hori_mod = 0;
		    vert_mod = 0;
		    ret = bowl_check_piece_position(bowl,
				    bowl->block.x, bowl->block.y, new_rot);
		    if (ret != POSVALID) {
			    if (bowl_check_piece_position(bowl,
					    bowl->block.x+1, bowl->block.y,
					    new_rot) == POSVALID) {
				    hori_mod = 1;
				    ret = POSVALID;
			    } else if (bowl_check_piece_position(bowl,
					    bowl->block.x-1, bowl->block.y,
					    new_rot) == POSVALID) {
				    hori_mod = -1;
				    ret = POSVALID;
			    } else if (bowl_check_piece_position(bowl,
					    bowl->block.x, bowl->block.y-1,
					    new_rot) == POSVALID) {
				    hori_mod = 0;
				    vert_mod = -1;
				    ret = POSVALID;
			    }
		    }
		    if (ret == POSVALID && (config.modern ||
				    (hori_mod == 0 && vert_mod==0))) {
			    bowl->block.rot_id = new_rot;
			    bowl->block.x += hori_mod;
			    bowl->block.y += vert_mod;
			    if (vert_mod)
				    bowl->block.cur_y = bowl->block.y*bowl->block_size;
			    bowl_compute_help_pos(bowl);
			    bowl_check_lockdelay(bowl);
		    }
	    } else if (bc->hdrop == CS_DOWN) {
		    if (config.gametype == GAME_TRAINING)
			    bowl_toggle_gravity(bowl);
		    else {
			    bowl_drop_block(bowl);
			    old_y = bowl->block.y-1; /* prevent warning below */
			    bowl_check_lockdelay(bowl);
			    if (bowl->ldelay_cur > 0)
				    bowl->ldelay_cur = 1;
		    }
	    } else if (bc->hold == CS_DOWN && bowl->hold_active && !bowl->hold_used) {
		    /* put current piece to hold, use piece in hold or next block */
		    bowl->ldelay_cur = 0; /* reset lock delay if any */
		    bowl_use_hold(bowl);
		    if (bowl->game_over)
			bowl_finish_game(bowl,0);
	    }

        /* update horizontal bowl position */
        if (bc->lshift == CS_PRESSED || bc->rshift == CS_PRESSED) {
                /* charge das */
        	if (bowl->das_charge < bowl->das_maxcharge) {
        		bowl->das_charge += ms;
        		if (bowl->das_charge > bowl->das_maxcharge)
        			bowl->das_charge = bowl->das_maxcharge;
        	}
                /* perform auto shift */
                if (bowl->das_charge == bowl->das_maxcharge) {
                	if (bc->lshift == CS_PRESSED)
                		hori_mod = -1;
                	else
                		hori_mod = 1;
                	if (bowl_check_piece_position(bowl,
                			bowl->block.x + hori_mod, bowl->block.y,
					bowl->block.rot_id) != POSVALID)
                		bowl->das_charge = bowl->das_maxcharge;
                	else {
                		bowl->block.x += hori_mod;
                		hori_movement = 1;
                        	bowl->das_charge -= bowl->das_drop;
                        	bowl_check_lockdelay(bowl);
                	}
                }
        }
        if (hori_movement) {
        	bowl_compute_help_pos( bowl );
#ifdef SOUND
        	if (!bowl->mute && config.shiftsound)
        		sound_play( bowl->wav_leftright );
#endif    
        }

        /* update horizontal float&screen position */
        bowl->block.sx = bowl->block.x * bowl->block_size + bowl->sx;

        /* update vertical position */
        if ( bowl->cpu_player && !bowl->cpu_down )
        	if ( delay_timed_out( &bowl->cpu_delay, ms ) )
        		bowl->cpu_down = 1;
        vy = bowl->block_vert_vel;
        if (bc->sdrop == CS_PRESSED || bowl_cpu_may_move_down(bowl))
        	if (bowl->block_drop_vel > vy)
        		if (config.modern || (bc->lshift!=CS_PRESSED && bc->rshift!=CS_PRESSED))
        			vy = bowl->block_drop_vel;
        if (bowl->cpu_player && config.cpu_sfactor != 100) {
        	vy = vy * config.cpu_sfactor / 100.0;
        }
        if (!bowl->zero_gravity && bowl->ldelay_cur == 0) {
        	bowl->block.cur_y += vy * ms;
        	bowl->block.y = bowl_convert_cury2y(bowl);
        }

        /* skip lock delay with soft drop */
        if (bc->sdrop == CS_PRESSED)
        	if (bowl->ldelay_cur > 0)
        		bowl->ldelay_cur = 1;

        /* check lock delay */
        if (bowl->ldelay_cur > 0) {
        	bowl->ldelay_cur -= ms;
        	/* if lock delay runs out, insert block */
        	if (bowl->ldelay_cur <= 0) {
        		bowl->ldelay_cur = 0;
			bowl_insert_block(bowl);
        	}
        } else {
        	/* no active lock delay, check all y-positions between last
        	 * and current position (might be more than one due to fast drop
        	 * on low frame rates). if position is blocked, either start
        	 * lock delay or insert block directly. */
        	for (int i = old_y+1; i <= bowl->block.y; i++) {
        		if (bowl_check_piece_position(bowl, bowl->block.x, i,
						bowl->block.rot_id) != POSVALID) {
        			/* reset position to last valid position */
				bowl->block.y = i - 1;
				bowl->block.cur_y = bowl->block.y * bowl->block_size;

        			if (bowl->ldelay_max == 0) {
        				/* lock delay disabled, insert directly */
					bowl_insert_block(bowl);
        			} else {
        				/* start lock delay */
        				bowl->ldelay_cur = bowl->ldelay_max;
        			}
        		}
        	}
        }

        /* use discrete y position if configured block_by_block, during
         * lock delay or if next position is blocked, otherwise go smooth  */
        if (bowl->ldelay_cur > 0 || config.block_by_block ||
        				!bowl_piece_can_drop(bowl))
            bowl->block.sy = bowl->block.y * bowl->block_size + bowl->sy;
        else
            bowl->block.sy = (int)bowl->block.cur_y + bowl->sy;
    }

    /* CHECK SPECIAL EVENTS */
    if ( !bowl->paused )
    {
        if ( delay_timed_out( &bowl->add_delay, ms ) )
        {
            if ( bowl->add_lines )
            {
	        bowl_add_line( bowl, bowl->add_line_holes, 0 );
                bowl->draw_contents = 1;
            }
            if ( bowl->add_tiles )
                bowl_add_tile( bowl );
        }
    }
}

/*
====================================================================
Draw a single bowl tile.
====================================================================
*/
void bowl_draw_tile( Bowl *bowl, int i, int j )
{
    int x = bowl->sx + i * bowl->block_size;
    int y = bowl->sy + j * bowl->block_size;
    int offset;

    if ( bowl->blind ) return;
    
    if ( bowl->contents[i][j] != -1 ) {
        offset = bowl->contents[i][j] * bowl->block_size;
        DEST( offscreen, x, y, bowl->block_size, bowl->block_size );
        SOURCE( bowl->blocks, offset, 0 );
        blit_surf();
    }
    else {
        DEST( offscreen, x, y, bowl->block_size, bowl->block_size );
        SOURCE( bkgnd, x, y );
        blit_surf();
    }
    DEST( sdl.screen, x, y , bowl->block_size, bowl->block_size );
    SOURCE( offscreen, x, y );
    blit_surf();
    add_refresh_rect( x, y, bowl->block_size, bowl->block_size );
}

/*
====================================================================
Draw bowl to offscreen and screen.
====================================================================
*/
void bowl_draw_contents( Bowl *bowl )
{
    int i, j;
    int x = bowl->sx, y = bowl->sy, offset;
    
    if ( bowl->blind ) return;
    
    for ( j = 0; j < bowl->h; j++ ) {
        for ( i = 0; i < bowl->w; i++ ) {
            if ( bowl->contents[i][j] != -1 ) {
                offset = bowl->contents[i][j] * bowl->block_size;
                DEST( offscreen, x, y, bowl->block_size, bowl->block_size );
                SOURCE( bowl->blocks, offset, 0 );
                blit_surf();
            }
            else {
                DEST( offscreen, x, y, bowl->block_size, bowl->block_size );
                SOURCE( bkgnd, x, y );
                blit_surf();
            }
            DEST( sdl.screen, x, y , bowl->block_size, bowl->block_size );
            SOURCE( offscreen, x, y );
            blit_surf();
            x += bowl->block_size;
        }
        x = bowl->sx;
        y += bowl->block_size;
    }
    add_refresh_rect( bowl->sx, bowl->sy, bowl->sw, bowl->sh );
}

/*
====================================================================
Draw frames and fix text to bkgnd.
====================================================================
*/
void bowl_draw_frames( Bowl *bowl )
{
    /* data box */
    int dx = bowl->sx + 10, dy = bowl->sy + bowl->sh + 20, dw = bowl->sw - 20, dh = 50;
    /* bowl itself */
    draw_3dframe( bkgnd, bowl->sx, bowl->sy, bowl->sw, bowl->sh, 6 );
    /* name&score&level */
    draw_3dframe( bkgnd, dx, dy, dw, dh, 4 );
    bowl->font->align = ALIGN_X_LEFT | ALIGN_Y_TOP;
    write_text( bowl->font, bkgnd, dx + 4, dy + 4, _("Player:"), OPAQUE );
    bowl->font->align = ALIGN_X_RIGHT | ALIGN_Y_TOP;
    write_text( bowl->font, bkgnd, dx + dw - 4, dy + 4, bowl->name, OPAQUE );
    bowl->font->align = ALIGN_X_LEFT | ALIGN_Y_CENTER;
    write_text( bowl->font, bkgnd, dx + 4, dy + dh / 2, _("Score:"), OPAQUE );
    bowl->font->align = ALIGN_X_LEFT | ALIGN_Y_BOTTOM;
    write_text( bowl->font, bkgnd, dx + 4, dy + dh - 4, _("Lines:"), OPAQUE );
    /* preview */
    if (bowl->preview)
	    draw_3dframe( bkgnd, bowl->preview_sx - 2, bowl->preview_sy - 2,
			    bowl->preview_sw + 4, bowl->preview_sh + 4, 4 );
    /* hold */
    if (bowl->hold_active)
	    draw_3dframe( bkgnd, bowl->hold_sx - 2, bowl->hold_sy - 2,
			    bowl->hold_sw + 4, bowl->hold_sh + 4, 4 );
    /* part that is updated when redrawing score/level */
    bowl->score_sx = dx + dw / 2 - 36;
    bowl->score_sy = dy + bowl->font->height + 4;
    bowl->score_sw = dw / 2 + 36;
    bowl->score_sh = dh - bowl->font->height - 8;
    /* stats */
    if (bowl->show_stats)
	    draw_3dframe(bkgnd, bowl->stats_x, bowl->stats_y, bowl->stats_w, bowl->stats_h, 4);
}

/*
====================================================================
Toggle pause of bowl.
====================================================================
*/
void bowl_toggle_pause( Bowl *bowl )
{
    if ( bowl->paused ) {
        /* unpause */
        bowl->hide_block = 0;
        bowl_draw_contents( bowl );
        bowl->paused = 0;
    }
    else {
        /* pause */
        bowl->hide_block = 1;
        DEST( offscreen, bowl->sx, bowl->sy, bowl->sw, bowl->sh );
        SOURCE( bkgnd, bowl->sx, bowl->sy );
        blit_surf();
        bowl->font->align = ALIGN_X_CENTER | ALIGN_Y_CENTER;
        write_text( bowl->font, offscreen, bowl->sx + bowl->sw / 2, bowl->sy + bowl->sh / 2, "Paused", OPAQUE );
        DEST( sdl.screen, bowl->sx, bowl->sy, bowl->sw, bowl->sh );
        SOURCE( offscreen, bowl->sx, bowl->sy );
        blit_surf();
        add_refresh_rect( bowl->sx, bowl->sy, bowl->sw, bowl->sh );
        bowl->paused = 1;
    }
}

/** Switch current and hold piece (or store piece and get next piece if
 * hold is empty). If switching pieces ends the game bowl->game_over is
 * set and can be checked afterwards to finish the game. */
void bowl_use_hold(Bowl *bowl) {
	if (!bowl->hold_active)
		return;

	bowl->hold_used = 1;

	if (bowl->hold_id != -1) {
		int iswap = bowl->hold_id;
		bowl->hold_id = bowl->block.id;
		bowl->block.id = iswap;
		bowl_init_current_piece(bowl, bowl->block.id);
		/* opposite to select_next_block init_current_piece
		 * does not check for game over so we do this here */
		if (bowl_check_piece_position(bowl,
				bowl->block.x, bowl->block.y,
				bowl->block.rot_id) != POSVALID)
			bowl->game_over = 1;
	} else {
		bowl->hold_id = bowl->block.id;
	        bowl_select_next_block(bowl);
	}
}

/*
====================================================================
Play an optimized mute game. (used for stats)
@llimit is number of max lines or -1 for unlimited play
====================================================================
*/
void bowl_quick_game( Bowl *bowl, CPU_ScoreSet *bscores, int llimit )
{
    int old_level;
    int line_count;
    int line_y[4];
    int i, j, l;
    CPU_Data cpu_data;

    /* constant cpu data */
    cpu_data.bowl_w = bowl->w;
    cpu_data.bowl_h = bowl->h;
    cpu_data.style = config.cpu_style;

    /* reset bowl */
    for ( i = 0; i < bowl->w; i++ ) {
        for ( j = 0; j < bowl->h; j++ )
            bowl->contents[i][j] = -1;
    }
    memset(&bowl->stats,0,sizeof(BowlStats));
    bowl->score.value = 0;
    bowl->preview = 1; /* avoid no preview bonus */
    bowl->lines = bowl->level = bowl->use_figures = 0;
    bowl->game_over = 0;
    bowl->add_lines = bowl->add_tiles = 0;
    bowl->next_block_id = next_blocks[bowl->next_blocks_pos++];
    while ( !bowl->game_over ) {
        /* get next block */
        bowl_select_next_block(bowl);
        /* compute cpu dest */
        cpu_data.piece_id = bowl->block.id;
        cpu_data.preview_id = bowl->next_block_id;
        cpu_data.hold_active = bowl->hold_active;
        cpu_data.hold_id = bowl->hold_id;
        cpu_data.base_scores = *bscores;
        for ( i = 0; i < bowl->w; i++ )
            for ( j = 0; j < bowl->h; j++ )
                cpu_data.original_bowl[i][j] = ( bowl->contents[i][j] != -1 );
        cpu_analyze_data( &cpu_data );
        /* switch pieces if eval says hold is better */
        if (cpu_data.use_hold)
        	bowl_use_hold(bowl);
        if (cpu_data.result.id != bowl->block.id)
        	printf("Oopsy... id mismatch after CPU eval???\n");
        /* insert -- no additional checks as there is no chance for an illegal block else the fucking CPU sucks!!!! */
        for ( i = 0; i < 4; i++ ) {
            for ( j = 0; j < 4; j++ )
                if ( block_masks[cpu_data.result.id].mask[cpu_data.result.rot][i][j] ) {
                    if ( j + cpu_data.result.y < 0 ) {
                        bowl->game_over = 1;
                        break;
                    }
                    bowl->contents[i + cpu_data.result.x][j + cpu_data.result.y] = 1;
                }
            if ( bowl->game_over ) break;
        }
        if ( bowl->game_over ) break;
        /* check for completed lines */
        line_count = 0;
        for ( j = 0; j < bowl->h; j++ ) {
            for ( i = 0; i < bowl->w; i++ ) {
                if ( bowl->contents[i][j] == -1 )
                    break;
            }
            if ( i == bowl->w )
                line_y[line_count++] = j;
        }
        for ( j = 0; j < line_count; j++ )
            for ( i = 0; i < bowl->w; i++ ) {
                for ( l = line_y[j]; l > 0; l-- )
                    bowl->contents[i][l] = bowl->contents[i][l - 1];
            bowl->contents[i][0] = -1;
        }
        /* score */
        bowl_add_score(bowl, line_count);
        /* line and level update */
        old_level = bowl->lines / 10;
        bowl->lines += line_count;
        if ( old_level != bowl->lines / 10 ) {
            /* new level */
            bowl->level++;
            bowl_set_vert_block_vel( bowl );
        }
        /* stats */
        if (line_count > 0)
        	bowl->stats.cleared[line_count-1]++;
        /* stop at max lines */
        if (llimit != -1 && bowl->lines >= llimit) {
        	bowl->game_over = 1;
        	break;
        }

    }
}

/** Draw statistics */
void write_stat_line(Bowl *bowl, const char *name, int val, int x, int *y)
{
	char str[32];
	if (val < 0)
		snprintf(str, 32, "%s -", name );
	else
		snprintf(str, 32, "%s %d", name, val );
	if (bowl->game_over)
		write_text(bowl->font, bkgnd, x, *y, str, OPAQUE );
	write_text(bowl->font, sdl.screen, x, *y, str, OPAQUE );
	*y += bowl->font->height + 2;
}
void bowl_draw_stats(Bowl *bowl)
{
	BowlStats *s = &bowl->stats;
	int x = bowl->stats_x + 5, y = bowl->stats_y + 10;
	int trate, avg, dc, trans;

	bowl->font->align = ALIGN_X_LEFT | ALIGN_Y_TOP;

	write_stat_line(bowl, _("Pieces Placed:"), s->pieces, x, &y);
	write_stat_line(bowl, _("I-Pieces:     "), s->i_pieces, x, &y);
	y += bowl->font->height + 2;

	write_stat_line(bowl, _("Singles: "), s->cleared[0], x, &y);
	write_stat_line(bowl, _("Doubles: "), s->cleared[1], x, &y);
	write_stat_line(bowl, _("Triples: "), s->cleared[2], x, &y);
	write_stat_line(bowl, _("Tetrises:"), s->cleared[3], x, &y);
	y += bowl->font->height + 2;

	trate = s->cleared[0] + s->cleared[1]*2 +
			s->cleared[2]*3 + s->cleared[3]*4;
	if (trate > 0)
		trate = ((1000 * (s->cleared[3]*4) / trate) + 5) / 10;
	else
		trate = -1;
	write_stat_line(bowl, "Tetris Rate:", trate, x, &y);
	y += bowl->font->height + 2;

	write_stat_line(bowl, _("Droughts:   "), s->droughts, x, &y);
	write_stat_line(bowl, _("Drought Max:"), s->max_drought, x, &y);
	if (s->droughts > 0)
		avg = ((10 * s->sum_droughts / s->droughts) + 5) / 10;
	else
		avg = -1;
	write_stat_line(bowl, _("Drought Avg:"), avg, x, &y);
	y += bowl->font->height + 2;

	if (bowl->lines < bowl->firstlevelup_lines)
		trans = bowl->firstlevelup_lines - bowl->lines;
	else
		trans = -1;
	write_stat_line(bowl, _("Transition:"), trans, x, &y);
	dc = ((1000 * bowl->das_charge / bowl->das_maxcharge) + 5) / 10;
	write_stat_line(bowl, _("DAS Charge:"), dc, x, &y);
	if (bowl->drought > 12)
		dc = bowl->drought;
	else
		dc = -1;
	write_stat_line(bowl, _("Drought:   "), dc, x, &y);
}

void bowl_toggle_gravity(Bowl *bowl)
{
	if (!bowl)
		return;
	bowl->zero_gravity = !bowl->zero_gravity;
}
