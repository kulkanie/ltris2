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

#include "../libgame/bowl.h"
#include "../libgame/tetris.h"
#include "tools.h"
#include "vconfig.h"
#include "vgame.h"

extern Config config;
extern Bowl *bowls[];

void VBowl::connect(Bowl *b) {
	bowl = b;
	_loginfo("Connected bowl at %d,%d\n",b->sx,b->sy);
}

VGame::VGame() {
	init_sdl(0); // needed to create dummy sdl.screen
	tetris_create(); // loads figures and init block masks
}

VGame::~VGame() {
	tetris_clear(); // clear any open game
	tetris_delete();
	quit_sdl();
}

/** Initialize a new game. If @demo is true, start a demo game. */
void VGame::init(VConfig &vcfg, bool demo) {
	/* clear game context */
	tetris_clear();

	/* set default config values */
	config_reset();

	/* transfer existing vconfig settings */
	if (demo)
		config.gametype = GAME_DEMO;
	else
		config.gametype = vcfg.gametype;
	config.modern = vcfg.modern;
	config.starting_level = vcfg.startinglevel;
	snprintf(config.player1.name,32,"%s",vcfg.playernames[0].c_str());

	/* initialize actual game context */
	tetris_init();

	for (int i = 0; i < MAXNUMPLAYERS; i++)
		if (bowls[i])
			vbowls[i].connect(bowls[i]);
}
