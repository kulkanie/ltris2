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

/** Wrapper for libgame's bowl.c */
class VBowl {
	Bowl *bowl; /* merely a pointer, memory stuff is handled by VGame */
public:
	VBowl() : bowl(NULL) {};
	void connect(Bowl *b);
};

/** Wrapper for libgame's tetris.c stuff. */
class VGame {
	VBowl vbowls[MAXNUMPLAYERS];
public:
	VGame();
	~VGame();
	void init(VConfig &vcfg, bool demo);
};

#endif

