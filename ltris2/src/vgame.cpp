/*
 * vgame.cpp
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

#include "../libgame/sdl.h"
#include "../libgame/tetris.h"
#include "vgame.h"

VGame::VGame() {
	init_sdl(0); // needed to create dummy sdl.screen
	tetris_create(); // loads figures and inits block masks
}

VGame::~VGame() {
	quit_sdl();
	tetris_delete();
}
