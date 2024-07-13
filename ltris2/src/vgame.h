/*
 * vgame.h
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

#ifndef __VGAME_H_
#define __VGAME_H_

enum {
	VGS_NOINIT = -1,
	VGS_RUNNING,
	VGS_PAUSED,
	VGS_GAMEOVER
};

class View;

/** Wrapper for libgame's tetris.c stuff. */
class VGame {
	friend View;

	VBowl vbowls[MAXNUMPLAYERS];
	int state;
	Texture background;

	void setBowlControls(BowlControls &bc, SDL_Event &ev, PControls &pctrl);
	void setBowlControlsCPU(BowlControls &bc, VBowl &bowl);
	void addFrame(SDL_Rect inner, int padding = 0, int border = 0);
public:
	VGame();
	~VGame();
	void init(bool demo);
	void render();
	void update(uint ms, SDL_Event &ev);
};

#endif

