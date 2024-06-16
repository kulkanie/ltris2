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
#include "sdl.h"
#include "mixer.h"
#include "tools.h"
#include "vconfig.h"
#include "theme.h"
#include "vbowl.h"
#include "vgame.h"

extern Renderer renderer;
extern VConfig vconfig;
extern Theme theme;
extern SDL_Renderer *mrc;
extern Config config;

VGame::VGame() : state(VGS_NOINIT) {
	init_sdl(0); // needed to create dummy sdl.screen
	tetris_create(); // loads figures and init block masks
}

VGame::~VGame() {
	if (state != VGS_NOINIT)
		tetris_clear(); // clear any open game
	tetris_delete();
	quit_sdl();
}

/** Initialize a new game. If @demo is true, start a demo game. */
void VGame::init(bool demo) {
	/* clear game context */
	if (state != VGS_NOINIT)
		tetris_clear();

	/* set default config values */
	config_reset();

	/* XXX only allow single player for now */
	vconfig.gametype = GAME_CLASSIC;

	/* transfer existing vconfig settings */
	if (demo)
		config.gametype = GAME_DEMO;
	else
		config.gametype = vconfig.gametype;
	config.modern = vconfig.modern;
	config.starting_level = vconfig.startinglevel;
	snprintf(config.player1.name,32,"%s",vconfig.playernames[0].c_str());

	/* initialize actual game context */
	tetris_init();

	/* XXX initialize single vbowl wrapper, TODO do all game types */
	int tsize = (int)(renderer.getHeight() / 50)*2; /* get nearest even value to 25 tiles */
	int x = (renderer.getWidth() - (tsize*BOWL_WIDTH))/2;
	vbowls[0].init(0,x,0,tsize);

	state = VGS_RUNNING;
}

/** Render all bowls and animations. */
void VGame::render() {
	if (state == VGS_NOINIT)
		return;

	for (auto b : vbowls)
		if (b.initialized())
			b.render();
}

/** Update bowls according to passed time @ms in milliseconds and input. */
void VGame::update(uint ms) {
	if (state == VGS_NOINIT)
		return;

	for (auto b : vbowls)
		if (b.initialized())
			b.update(ms);
}
