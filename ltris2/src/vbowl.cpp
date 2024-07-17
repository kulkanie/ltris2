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
 * and piece tile size @_tsize. If @compact is true render compact player info. */
void VBowl::init(uint id, uint tsize, SDL_Rect &rb, SDL_Rect &rp,
				SDL_Rect &rh, SDL_Rect &rs, bool compact)
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
	compactInfo = compact;

	_loginfo("  set vbowl %d at (%d,%d), tilesize=%d\n",id,rBowl.x,rBowl.y,tileSize);
	if (!bowl->preview)
		_loginfo("    preview disabled\n");
	if (!bowl->hold_active)
		_loginfo("    hold disabled\n");

	/* TEST for sprites
	for (int i = 0; i < 9; i++) {
		bowl->contents[i][19] = 1;
		bowl->contents[i][18] = 2;
		bowl->contents[i][17] = 3;
		bowl->contents[i][16] = 4;
	} */
}

/** Render bowl by drawing pieces, preview, hold, score, ... */
void VBowl::render() {
	int pid, x, y;
	string strLines;

	if (bowl == NULL)
		return;

	/* player info */
	int ix = rScore.x + rScore.w/2, iy = rScore.y + tileSize/2;
	theme.vbaFontNormal.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	theme.vbaFontBold.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	theme.vbaFontSmall.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	/* first line is always name, second is score */
	theme.vbaFontBold.write(ix,iy,bowl->name);
	iy += tileSize;
	theme.vbaFontNormal.write(ix,iy,to_string((int)counter_get_approach(bowl->score)));
	/* add limit to line info if no transition yet */
	if (bowl->firstlevelup_lines > 10 && bowl->lines < bowl->firstlevelup_lines)
		strLines = to_string(bowl->lines)+"/"+to_string(bowl->firstlevelup_lines);
	else
		strLines = to_string(bowl->lines);
	if (compactInfo) {
		/* have level and lines in one line */
		string str;
		iy += tileSize;
		theme.vbaFontSmall.setAlign(ALIGN_X_LEFT | ALIGN_Y_CENTER);
		str = string(_("Lines"))+": "+strLines;
		theme.vbaFontSmall.write(rScore.x,iy,str);
		theme.vbaFontSmall.setAlign(ALIGN_X_RIGHT | ALIGN_Y_CENTER);
		str = string(_("Level"))+": "+to_string(bowl->level);
		theme.vbaFontSmall.write(rScore.x+rScore.w,iy,str);
	} else {
		/* level and lines in separate lines */
		iy += 1.5*tileSize;
		theme.vbaFontBold.write(ix,iy,_("Level"));
		iy += tileSize;
		theme.vbaFontNormal.write(ix,iy,to_string(bowl->level));
		iy += 1.5*tileSize;
		theme.vbaFontBold.write(ix,iy,_("Lines"));
		iy += tileSize;
		theme.vbaFontNormal.write(ix,iy,strLines);
	}

	/* show nothing more but stats for pause or game over */
	if (bowl->paused || bowl->game_over) {
		string msg = bowl->game_over?_("Game Over"):_("Paused");

		theme.vbaFontBold.write(rBowl.x + rBowl.w/2,
					rBowl.y + 3*tileSize, msg);
		renderStats();
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
	if (rPreview.w > 0 && bowl->preview && bowl->next_block_id >= 0) {
		theme.vbaPreviews[bowl->next_block_id].copy(rPreview.x,rPreview.y+1.5*tileSize);

		/* only for modern: show next two pieces of piece bag */
		for (int i = 0; i < bowl->preview-1; i++) {
			pid = next_blocks[bowl->next_blocks_pos+i];
			theme.vbaPreviews[pid].copy(rPreview.x,
					rPreview.y+4.5*tileSize+i*3*tileSize);
		}
	}

	/* hold piece */
	if (rHold.w > 0 && bowl->hold_active && bowl->hold_id != -1)
		theme.vbaPreviews[bowl->hold_id].copy(rHold.x,rHold.y+1.5*tileSize);

}

/** Update bowl according to passed time @ms in milliseconds and input. */
void VBowl::update(uint ms, BowlControls &bc) {
	if (bowl == NULL)
		return;

	if (!bowl->paused)
		bowl_update(bowl, ms, &bc, 0);
}


/** Render a single line into bowl at y-pos @y and update @y to next line.
 * Caption @cap is left and value @val right aligned. If @val < 0 show a minus
 * for "not set". */
void VBowl::renderStatLine(const string &cap, int val, int &y)
{
	int cx = rBowl.x + tileSize;
	int vx = rBowl.x + rBowl.w - tileSize;

	theme.vbaFontSmall.setAlign(ALIGN_X_LEFT | ALIGN_Y_TOP);
	theme.vbaFontSmall.write(cx, y, cap+":");
	theme.vbaFontSmall.setAlign(ALIGN_X_RIGHT | ALIGN_Y_TOP);
	if (val < 0)
		theme.vbaFontSmall.write(vx, y, "-");
	else
		theme.vbaFontSmall.write(vx, y, to_string(val));

	y += theme.vbaFontSmall.getLineHeight();
}

/** Render stats inside of bowl. */
void VBowl::renderStats()
{
	BowlStats *s = &bowl->stats;
	int sy = rBowl.y + 5.5*tileSize;

	renderStatLine(_("Pieces Placed"), s->pieces, sy);
	renderStatLine(_("I-Pieces"), s->i_pieces, sy);
	sy += theme.vbaFontSmall.getLineHeight();

	renderStatLine(_("Singles"), s->cleared[0], sy);
	renderStatLine(_("Doubles"), s->cleared[1], sy);
	renderStatLine(_("Triples"), s->cleared[2], sy);
	renderStatLine(_("Tetrises"), s->cleared[3], sy);
	sy += theme.vbaFontSmall.getLineHeight();

	/* tetris rate: how many line where cleared by tetris */
	int trate = s->cleared[0] + s->cleared[1]*2 +
			s->cleared[2]*3 + s->cleared[3]*4;
	if (trate > 0)
		trate = ((1000 * (s->cleared[3]*4) / trate) + 5) / 10; /* round up */
	else
		trate = -1;
	renderStatLine(_("Tetris Rate"), trate, sy);
	sy += theme.vbaFontSmall.getLineHeight();


	renderStatLine(_("Droughts"), s->droughts, sy);
	renderStatLine(_("Drought Max"), s->max_drought, sy);
	int avg = -1;
	if (s->droughts > 0)
		avg = ((10 * s->sum_droughts / s->droughts) + 5) / 10;
	renderStatLine(_("Drought Avg"), avg, sy);
	int dc = -1;
	if (bowl->drought > 12)
		dc = bowl->drought;
	renderStatLine(_("Current Drought"), dc, sy);
}
