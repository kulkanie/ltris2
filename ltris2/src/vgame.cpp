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

/** Translate current event/input states into bowl controls for human player. */
void VGame::setBowlControls(BowlControls &bc, SDL_Event &ev, PControls &pctrl)
{
	/* get key state */
	const Uint8 *keystate = SDL_GetKeyboardState(NULL);

	/* clear input state */
	memset(&bc,0,sizeof(bc));

	if (keystate[pctrl.lshift])
		bc.lshift = CS_PRESSED;
	if (keystate[pctrl.rshift])
		bc.rshift = CS_PRESSED;
	if (keystate[pctrl.sdrop])
		bc.sdrop = CS_PRESSED;

	if (ev.type == SDL_KEYDOWN) {
		if (ev.key.keysym.scancode == pctrl.lshift)
			bc.lshift = CS_DOWN;
		if (ev.key.keysym.scancode == pctrl.rshift)
			bc.rshift = CS_DOWN;
		if (ev.key.keysym.scancode == pctrl.lrot)
			bc.lrot = CS_DOWN;
		if (ev.key.keysym.scancode == pctrl.rrot)
			bc.rrot = CS_DOWN;
		if (ev.key.keysym.scancode == pctrl.hdrop)
			bc.hdrop = CS_DOWN;
		if (ev.key.keysym.scancode == pctrl.hold)
			bc.hold = CS_DOWN;
	}

	/* XXX fix gamepad support
	 * allow gamepad for bowl 0
	if (i == 0 && config.gp_enabled) {
		if (gamepad_ctrl_isdown(GPAD_LEFT))
			bc.lshift = CS_DOWN;
		else if (gamepad_ctrl_ispressed(GPAD_LEFT))
			bc.lshift = CS_PRESSED;
		if (gamepad_ctrl_isdown(GPAD_RIGHT))
			bc.rshift = CS_DOWN;
		else if (gamepad_ctrl_ispressed(GPAD_RIGHT))
			bc.rshift = CS_PRESSED;
		if (gamepad_ctrl_isactive(GPAD_DOWN))
			bc.sdrop = CS_PRESSED;

		if (ev.type == SDL_JOYBUTTONDOWN) {
			if (ev.jbutton.button == config.gp_lrot)
				bc.lrot = CS_DOWN;
			if (ev.jbutton.button == config.gp_rrot)
				bc.rrot = CS_DOWN;
			if (ev.jbutton.button == config.gp_hdrop)
				bc.hdrop = CS_DOWN;
			if (ev.jbutton.button == config.gp_hold)
				bc.hold = CS_DOWN;
		}
	} */
}

/** Fake bowl controls for a CPU player. */
void VGame::setBowlControlsCPU(BowlControls &bc, VBowl &bowl)
{
	Bowl *b = bowl.bowl;

	/* clear input state */
	memset(&bc,0,sizeof(bc));

	/* left/right shift */
	if (b->cpu_dest_x > b->block.x)
		bc.rshift = CS_PRESSED;
	else if (b->cpu_dest_x < b->block.x)
		bc.lshift = CS_PRESSED;

	/* rotation */
	if (b->cpu_dest_rot != b->block.rot_id)
		bc.lrot = CS_DOWN;
}

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

	/* transfer existing vconfig settings */
	if (demo)
		config.gametype = GAME_DEMO;
	else
		config.gametype = vconfig.gametype;
	config.modern = vconfig.modern;
	config.starting_level = vconfig.startinglevel;
	snprintf(config.player1.name,32,"%s",vconfig.playernames[0].c_str());

	_loginfo("Initializing game (type=%d)\n",config.gametype);

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
void VGame::update(uint ms, SDL_Event &ev) {
	BowlControls bc;

	if (state == VGS_NOINIT)
		return;

	for (int i = 0; i < MAXNUMPLAYERS; i++)
		if (vbowls[i].initialized()) {
			if (vbowls[i].bowl->cpu_player)
				setBowlControlsCPU(bc,vbowls[i]);
			else
				setBowlControls(bc, ev, vconfig.controls[i]);
			vbowls[i].update(ms,bc);
		}
}
