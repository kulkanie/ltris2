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
extern VConfig vconfig;
extern Theme theme;
extern Bowl *bowls[];
extern Block_Mask block_masks[BLOCK_COUNT];
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
	_loginfo("  set vbowl %d at (%d,%d), tilesize=%d\n",id,rBowl.x,rBowl.y,tileSize);
}

/** Render bowl by drawing pieces, preview, hold, score, ... */
void VBowl::render() {
	int osize = theme.blocks.getHeight();
	int pid, x, y;

	/* bowl content */
	for (int j = 0; j < h; j++)
		for (int i = 0; i < w; i++) {
			if (bowl->contents[i][j] == -1)
				continue;
			pid = bowl->contents[i][j];
			x = rBowl.x + i*tileSize;
			y = rBowl.y + j*tileSize;
			theme.blocks.copy(pid*osize,0,osize,osize,x,y,tileSize,tileSize);
		}

	/* current piece: block.id is the index in block_masks and
	 * block_masks[].blockid is the picture id */
	if (bowl->are == 0) {
		pid = block_masks[bowl->block.id].blockid;
		x = rBowl.x + bowl->block.x*tileSize;
		if (bowl->ldelay_cur > 0 || config.block_by_block ||
				!bowl_piece_can_drop(bowl))
			y = rBowl.y + bowl->block.y*tileSize;
		else
			y = rBowl.y + bowl->block.cur_y / bowl->block_size * tileSize;
		for (int j = 0; j < 4; j++) {
			for (int i = 0; i < 4; i++) {
				if (block_masks[bowl->block.id].mask[bowl->block.rot_id][i][j])
					theme.blocks.copy(pid*osize,0,osize,osize,
							x,y,tileSize,tileSize);
				x += tileSize;
			}
			x = rBowl.x + bowl->block.x*tileSize;
			y += tileSize;
		}
	}
}

/** Update bowl according to passed time @ms in milliseconds and input. */
void VBowl::update(uint ms, BowlControls &bc) {
	bowl_update(bowl, ms, &bc, 0);
}
