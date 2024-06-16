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

/** Wrapper for libgame's bowl.c */
class VBowl {
	Bowl *bowl; /* merely a pointer, memory stuff is handled by VGame */
	int w, h; /* width/height of bowl (tiles) */
	int tileSize; /* size of a single tile */
	int sx, sy; /* screen position of bowl */
	int px, py; /* screen preview piece position */
	int hx, hy; /* screen hold piece position */
public:
	VBowl();
	void init(uint id, int _sx, int _sy, int tsize);
};

#endif
