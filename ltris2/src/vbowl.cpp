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
#include "tools.h"
#include "vconfig.h"
#include "vbowl.h"

extern Renderer renderer;
extern VConfig vconfig;
extern Bowl *bowls[];

VBowl::VBowl() {
	bowl = NULL;
	w = h = tileSize = sx = sy = px = py = hx = hy = 0;
}

void VBowl::init(uint id, int _sx, int _sy, int _tsize) {
	if (id >= MAXNUMPLAYERS || !bowls[id]) {
		_logerr("VBowl init: libgame bowl %d does not exist\n",id);
		return;
	}
	bowl = bowls[id];
	sx = _sx;
	sy = _sy;
	tileSize = _tsize;
	_loginfo("set vbowl %d at (%d,%d), tilesize=%d\n",id,sx,sy,tileSize);
}

