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
extern Block_Mask block_masks[BLOCK_COUNT];

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

/* Render a frame around @inner region to background. @padding is distance
 * between inner content and border. @border is width of frame. */
void VGame::addFrame(SDL_Rect inner, int padding, int border)
{
	int x1, y1, x2, y2;
	SDL_Rect outer = {inner.x - padding, inner.y - padding,
				inner.w + padding*2, inner.h + padding*2};
	SDL_Color clr = {0,0,0,128};


	/* since we don't want to set alpha but apply color to texture with it
	 * we need to enable blending */
	SDL_SetRenderDrawBlendMode(mrc, SDL_BLENDMODE_BLEND);

	background.fill(outer,clr);

	SDL_SetRenderTarget(mrc, background.get());
	for (int i = 0; i < border; i++) {
		/* alpha ranges from outer 96 to inner 192 */
		SDL_SetRenderDrawColor(mrc,0,0,0,96+96*(border-i-1)/(border-1));

		/* top line */
		x1 = outer.x - 1 - i;
		y1 = outer.y - 1 - i;
		x2 = outer.x + outer.w + i;
		y2 = outer.y - 1 - i;
		SDL_RenderDrawLine(mrc, x1, y1, x2, y2);

		/* left line */
		x1 = outer.x - 1 - i;
		y1 = outer.y - i; /* no -1 to prevent overlap with top line */
		x2 = outer.x - 1 - i;
		y2 = outer.y + outer.h + i;
		SDL_RenderDrawLine(mrc, x1, y1, x2, y2);

		SDL_SetRenderDrawColor(mrc,255,255,255,64+96*(border-i-1)/(border-1));

		/* right line */
		x1 = outer.x + outer.w + i;
		y1 = outer.y - 1 - i;
		x2 = outer.x + outer.w + i;
		y2 = outer.y + outer.h + i - 1; /* -1 to prevent overlap with bottom line */
		SDL_RenderDrawLine(mrc, x1, y1, x2, y2);

		/* bottom line */
		x1 = outer.x - 1 - i;
		y1 = outer.y + outer.h + i;
		x2 = outer.x + outer.w + i;
		y2 = outer.y + outer.h + i;
		SDL_RenderDrawLine(mrc, x1, y1, x2, y2);
	}
	SDL_SetRenderTarget(mrc, NULL);

	SDL_SetRenderDrawBlendMode(mrc, SDL_BLENDMODE_NONE);
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

	/* create background with wallpaper */
	background.create(renderer.getWidth(),renderer.getHeight());
	background.setBlendMode(0);
	renderer.setTarget(background);
	theme.wallpapers[0].copy();
	renderer.clearTarget();

	/* XXX initialize single vbowl wrapper, TODO do all game types */
	int tsize = (int)(renderer.getHeight() / 42)*2; /* get nearest even value to 22 tiles */
	int x = (renderer.getWidth() - (tsize*BOWL_WIDTH))/2;
	int panelw = x; /* space left/right of bowl */
	SDL_Rect rBowl, rPreview, rHold, rScore;

	rBowl = {x, 0, tsize*BOWL_WIDTH, tsize*BOWL_HEIGHT};
	if (config.modern)
		rPreview = {rBowl.x + rBowl.w + (panelw - 4*tsize)/2,
				2*tsize,4*tsize,10*tsize};
	else
		rPreview = {rBowl.x + rBowl.w + (panelw - 4*tsize)/2,
				5*tsize,4*tsize,4*tsize};
	rHold = {rPreview.x, 14*tsize, 4*tsize, 4*tsize};
	rScore = {(panelw - 6*tsize)/2, 3*tsize, 6*tsize, 7*tsize};

	vbowls[0].init(0,tsize,rBowl,rPreview,rHold,rScore);

	/* XXX store bowl assets in theme as using a VBowlAssets class in
	 * bowl does not work. textures get created but are not displayed
	 * and I can't figure out why... */
	/* create tiles for pieces */
	for (int i = 0; i < NUMTILES; i++) {
		theme.vbaTiles[i].create(tsize,tsize);
		renderer.setTarget(theme.vbaTiles[i]);
		theme.tiles.copy(i*theme.tileSize,0,theme.tileSize,theme.tileSize,
				0,0,tsize,tsize);
	}
	/* create previews for all pieces */
	double pxoff[7] = {-0.5,-0.5,-0.5,0,-0.5,-0.5,0};
	double pyoff[7] = {-2,-2,-2,-2,-2,-2,-1.5};
	for (int k = 0; k < NUMPIECES; k++) {
		int pid = block_masks[k].blockid;
		theme.vbaPreviews[k].create(4*tsize,2*tsize);
		renderer.setTarget(theme.vbaPreviews[k]);
		for (int j = 0; j < 4; j++)
			for (int i = 0; i < 4; i++) {
				if (!block_masks[k].mask[block_masks[k].rstart][i][j])
					continue;
				theme.vbaTiles[pid].copy(pxoff[k]*tsize + i*tsize,
							pyoff[k]*tsize + j*tsize);
			}
	}
	renderer.clearTarget();
	theme.vbaLoadFonts(tsize);

	/* and frames to background XXX for all game types, too! */
	addFrame(rBowl,0,tsize/3);
	addFrame(rPreview,tsize/4,tsize/3);
	addFrame(rHold,tsize/4,tsize/3);
	addFrame(rScore,tsize/4,tsize/3);

	/* write fixed text */
	renderer.setTarget(background);
	theme.vbaFontNormal.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	theme.vbaFontNormal.write(rPreview.x+rPreview.w/2,rPreview.y+tsize/2,_("Next"));
	theme.vbaFontNormal.write(rHold.x+rHold.w/2,rHold.y+tsize/2,_("Hold"));
	renderer.clearTarget();

	state = VGS_RUNNING;
}

/** Render all bowls and animations. */
void VGame::render() {
	if (state == VGS_NOINIT)
		return;

	/* background */
	background.copy();

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

/** Pause or unpause game. */
void VGame::pause(bool p)
{
	if (state == VGS_NOINIT || state == VGS_GAMEOVER)
		return;

	for (auto &vb : vbowls) {
		if (!vb.initialized())
			continue;
		if (vb.bowl->game_over)
			continue;

		if (p)
			vb.bowl->paused = 1;
		else
			vb.bowl->paused = 0;
	}

	if (p)
		state = VGS_PAUSED;
	else
		state = VGS_RUNNING;
}

bool VGame::isDemo()
{
	return (config.gametype == GAME_DEMO);
}
