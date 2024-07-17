/*
 * vbowl.h
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

#ifndef __VBOWL_H_
#define __VBOWL_H_

class View;
class VGame;

/** Wrapper for libgame's bowl.c */
class VBowl {
	friend View;
	friend VGame;

	Bowl *bowl; /* merely a pointer, memory stuff is handled by VGame */
	int w, h; /* width/height of bowl (tiles) */
	int tileSize; /* screen size of a single tile */
	SDL_Rect rBowl; /* screen region for bowl content */
	SDL_Rect rPreview; /* screen region for preview pieces */
	SDL_Rect rHold; /* screen region for hold piece */
	SDL_Rect rScore; /* screen region for score */
	bool compactInfo; /* display compact version of player score info */

	void renderStatLine(const string &cap, int val, int &y);
	void renderStats();
public:
	VBowl();
	void init(uint id, uint tsize, SDL_Rect &rb, SDL_Rect &rp, SDL_Rect &rh, SDL_Rect &rs, bool compact=false);
	bool initialized() { return (bowl != NULL); }
	void render();
	void update(uint ms, BowlControls &bc);
};

#endif
