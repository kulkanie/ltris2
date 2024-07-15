/*
 * vbowl.cpp
 * (C) 2024 by Michael Speck
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "../libgame/bowl.h"
#include "../libgame/tetris.h"
#include "sdl.h"
#include "mixer.h"
#include "tools.h"
#include "theme.h"
#include "vconfig.h"
#include "vbowl.h"

extern SDL_Renderer *mrc;
extern Renderer renderer;
extern VConfig vconfig;
extern Theme theme;
extern Bowl *bowls[];
extern Block_Mask block_masks[BLOCK_COUNT];
extern int  *next_blocks;
extern Config config;

VBowl::VBowl() {
	bowl = NULL;
	w = h = tileSize = 0;
}

/* Initialize representation of libgame bowl @id at screen position @_sx,@_sy
 * and piece tile size @_tsize. */
void VBowl::init(uint id, uint tsize, SDL_Rect &rb, SDL_Rect &rp,
					SDL_Rect &rh, SDL_Rect &rs)
{
	if (id >= MAXNUMPLAYERS || !bowls[id]) {
		_logerr("VBowl init: libgame bowl %d does not exist\n",id);
		return;
	}

	bowl = bowls[id];
	w = BOWL_WIDTH;
	h = BOWL_HEIGHT;
	tileSize = tsize;
	rBowl = rb;
	rPreview = rp;
	rHold = rh;
	rScore = rs;

	/* TEST for sprites
	for (int i = 0; i < 9; i++) {
		bowl->contents[i][19] = 1;
		bowl->contents[i][18] = 2;
		bowl->contents[i][17] = 3;
		bowl->contents[i][16] = 4;
	} */

	_loginfo("  set vbowl %d at (%d,%d), tilesize=%d\n",id,rBowl.x,rBowl.y,tileSize);
}

/** Render bowl by drawing pieces, preview, hold, score, ... */
void VBowl::render() {
	int pid, x, y;

	if (bowl == NULL)
		return;

	/* player info */
	int ix = rScore.x + rScore.w/2, iy = rScore.y + tileSize/2;
	theme.vbaFontNormal.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	theme.vbaFontBold.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	theme.vbaFontSmall.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	theme.vbaFontBold.write(ix,iy,bowl->name);
	iy += tileSize;
	theme.vbaFontNormal.write(ix,iy,to_string((int)counter_get_approach(bowl->score)));
	iy += 1.5*tileSize;
	theme.vbaFontBold.write(ix,iy,_("Level"));
	iy += tileSize;
	theme.vbaFontNormal.write(ix,iy,to_string(bowl->level));
	iy += 1.5*tileSize;
	theme.vbaFontBold.write(ix,iy,_("Lines"));
	iy += tileSize;
	theme.vbaFontNormal.write(ix,iy,to_string(bowl->lines));

	/* show nothing more on pause or game over */
	if (bowl->paused) {
		theme.vbaFontNormal.write(rBowl.x + rBowl.w/2,
					rBowl.y + rBowl.h/2, _("Paused"));
		return;
	}
	if (bowl->game_over) {
		theme.vbaFontNormal.write(rBowl.x + rBowl.w/2,
					rBowl.y + rBowl.h/2, _("Game Over"));
		return;
	}

	/* bowl content */
	for (int j = 0; j < h; j++)
		for (int i = 0; i < w; i++) {
			if (bowl->contents[i][j] == -1)
				continue;
			pid = bowl->contents[i][j];
			x = rBowl.x + i*tileSize;
			y = rBowl.y + j*tileSize;
			theme.vbaTiles[pid].copy(x,y);
		}

	/* (ghost) piece: block_masks[block.id].blockid is the picture id */
	if (!bowl->hide_block && bowl->are == 0) {
		/* get starting screen position for piece. y is a bit of an issue
		 * for smooth drop as we need to check here again if we use y or
		 * cur_y as sy from libgame is 480p and thus too coarse. */
		x = rBowl.x + bowl->block.x*tileSize;
		if (bowl->ldelay_cur > 0 || !vconfig.smoothdrop ||
				!bowl_piece_can_drop(bowl))
			y = rBowl.y + bowl->block.y*tileSize;
		else
			y = rBowl.y + bowl->block.cur_y / bowl->block_size * tileSize;
		int xstart = x;

		/* get starting y for ghost piece, x is same. to interfere as little
		 * with the original code we just translate the 480p position back */
		int gy = rBowl.y + ((bowl->help_sy - bowl->sy) / bowl->block_size)*tileSize;

		for (int j = 0; j < 4; j++) {
			for (int i = 0; i < 4; i++) {
				if (block_masks[bowl->block.id].mask[bowl->block.rot_id][i][j]) {
					pid = block_masks[bowl->block.id].blockid;

					/* ghost piece */
					if (config.modern) {
						theme.vbaTiles[pid].setAlpha(96);
						theme.vbaTiles[pid].copy(x,gy);
						theme.vbaTiles[pid].setAlpha(SDL_ALPHA_OPAQUE);
					}

					/* actual piece */
					theme.vbaTiles[pid].copy(x,y);

					/* use special tile id layered to indicate lock
					 * delay unless soft drop was used which will
					 * insert the block on the next cycle */
					if (bowl->ldelay_cur > 0 && !bowl->sdrop_pressed) {
						pid = LOCKDELAYTILEID;
						theme.vbaTiles[pid].setAlpha(128);
						theme.vbaTiles[pid].copy(x,y);
						theme.vbaTiles[pid].setAlpha(SDL_ALPHA_OPAQUE);
					}

				}
				x += tileSize;
			}
			x = xstart;
			y += tileSize;
			gy += tileSize;
		}
	}

	/* preview */
	if (bowl->preview && bowl->next_block_id >= 0) {
		theme.vbaPreviews[bowl->next_block_id].copy(rPreview.x,rPreview.y+1.5*tileSize);

		/* only for modern: show next two pieces of piece bag */
		for (int i = 0; i < bowl->preview-1; i++) {
			pid = next_blocks[bowl->next_blocks_pos+i];
			theme.vbaPreviews[pid].copy(rPreview.x,
					rPreview.y+4.5*tileSize+i*3*tileSize);
		}
	}

	/* hold piece */
	if (bowl->hold_active && bowl->hold_id != -1)
		theme.vbaPreviews[bowl->hold_id].copy(rHold.x,rHold.y+1.5*tileSize);

}

/** Update bowl according to passed time @ms in milliseconds and input. */
void VBowl::update(uint ms, BowlControls &bc) {
	if (bowl == NULL)
		return;

	if (!bowl->paused)
		bowl_update(bowl, ms, &bc, 0);
}
