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

VBowl::VBowl() {
	bowl = NULL;
	w = h = tileSize = sx = sy = px = py = hx = hy = 0;
}

/* Initialize representation of libgame bowl @id at screen position @_sx,@_sy
 * and piece tile size @_tsize. */
void VBowl::init(uint id, int _sx, int _sy, int _tsize) {
	if (id >= MAXNUMPLAYERS || !bowls[id]) {
		_logerr("VBowl init: libgame bowl %d does not exist\n",id);
		return;
	}
	bowl = bowls[id];
	w = BOWL_WIDTH;
	h = BOWL_HEIGHT;
	sx = _sx;
	sy = _sy;
	tileSize = _tsize;
	_loginfo("set vbowl %d at (%d,%d), tilesize=%d\n",id,sx,sy,tileSize);
}

/** Render bowl by drawing pieces, preview, hold, score, ... */
void VBowl::render() {
	/* simple background */
	SDL_Rect rect = {sx,sy,w*tileSize,h*tileSize};
	SDL_SetRenderDrawColor(mrc, 0, 0, 0, 200);
	SDL_SetRenderDrawBlendMode(mrc, SDL_BLENDMODE_BLEND);
	SDL_RenderFillRect(mrc,&rect);
	SDL_SetRenderDrawBlendMode(mrc, SDL_BLENDMODE_NONE);

	/* bowl content */
	for (int j = 0; j < h; j++)
		for (int i = 0; i < w; i++) {
			if (bowl->contents[i][j] == -1)
				continue;
			int osize = theme.blocks.getHeight();
			int pid = bowl->contents[i][j];
			int x = sx + i*tileSize;
			int y = sy + j*tileSize;
			theme.blocks.copy(osize*pid,0,osize,osize,x,y,tileSize,tileSize);
		}

	SDL_RenderPresent(mrc);
}

/** Update bowl according to passed time @ms in milliseconds and input. */
void VBowl::update(uint ms) {
	BowlControls bc;
	memset(&bc,0,sizeof(bc));
	bowl_update(bowl, ms, &bc, 0);
}
