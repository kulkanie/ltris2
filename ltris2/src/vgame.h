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

/** Wrapper for libgame's tetris.c stuff. */
class VGame {
	VBowl vbowls[MAXNUMPLAYERS];
	int state;

	void setBowlControls(BowlControls &bc, SDL_Event &ev, PControls &pctrl);
	void setBowlControlsCPU(BowlControls &bc, VBowl &bowl);
public:
	VGame();
	~VGame();
	void init(bool demo);
	void render();
	void update(uint ms, SDL_Event &ev);
};

#endif

