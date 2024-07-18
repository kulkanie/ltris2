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
void VGame::setBowlControls(uint bid, BowlControls &bc, SDL_Event &ev,
				const Uint8 *gpstate, PControls &pctrl)
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

	/* allow gamepad for bowl 0 */
	if (bid == 0 && vconfig.gp_enabled) {
		if (gpstate[GPAD_DOWN] == CS_PRESSED || gpstate[GPAD_DOWN] == CS_DOWN)
			bc.sdrop = CS_PRESSED;
		else if (gpstate[GPAD_LEFT] == CS_PRESSED)
			bc.lshift = CS_PRESSED;
		else if (gpstate[GPAD_LEFT] == CS_DOWN)
			bc.lshift = CS_DOWN;
		else if (gpstate[GPAD_RIGHT] == CS_PRESSED)
			bc.rshift = CS_PRESSED;
		else if (gpstate[GPAD_RIGHT] == CS_DOWN)
			bc.rshift = CS_DOWN;

		if (ev.type == SDL_JOYBUTTONDOWN) {
			if (ev.jbutton.button == vconfig.gp_lrot)
				bc.lrot = CS_DOWN;
			if (ev.jbutton.button == vconfig.gp_rrot)
				bc.rrot = CS_DOWN;
			if (ev.jbutton.button == vconfig.gp_hdrop)
				bc.hdrop = CS_DOWN;
			if (ev.jbutton.button == vconfig.gp_hold)
				bc.hold = CS_DOWN;
		}
	}
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


VGame::VGame() : type(-1), state(VGS_NOINIT) {
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
	if (state != VGS_NOINIT) {
		tetris_clear();
		for (auto &vb : vbowls)
			vb.bowl = NULL;
		state = VGS_NOINIT;
	}

	/* set default config values */
	config_reset();

	/* set game type, vconfig.gametype is different and must be translated! */
	if (demo) {
		type = -1;
		config.gametype = GAME_DEMO;
	} else {
		switch (vconfig.gametype) {
		case GT_NORMAL: config.gametype = GAME_CLASSIC; break;
		case GT_FIGURES: config.gametype = GAME_FIGURES; break;
		case GT_VSHUMAN: config.gametype = GAME_VS_HUMAN; break;
		case GT_VSCPU: config.gametype = GAME_VS_CPU; break;
		case GT_VSCPU2: config.gametype = GAME_VS_CPU_CPU; break;
		default: config.gametype = GAME_DEMO; break; /* shouldn't happen */
		}
		type = vconfig.gametype;
	}

	/* transfer existing vconfig settings */
	config.modern = vconfig.modern;
	config.starting_level = vconfig.startinglevel;
	snprintf(config.player1.name,32,"%s",vconfig.playernames[0].c_str());
	config.holes = vconfig.mp_numholes;
	config.rand_holes = vconfig.mp_randholes;
	config.cpu_style = vconfig.cpu_style;
	config.cpu_delay = vconfig.cpu_delay;
	config.cpu_sfactor = vconfig.cpu_sfactor;
	config.as_delay = vconfig.as_delay;
	config.as_speed = vconfig.as_speed;

	_loginfo("Initializing game (type=%d)\n",config.gametype);

	/* initialize actual game context */
	tetris_init();

	/* create background with wallpaper */
	background.create(renderer.getWidth(),renderer.getHeight());
	background.setBlendMode(0);
	renderer.setTarget(background);
	theme.wallpapers[0].copy();
	renderer.clearTarget();

	/* initialize vbowls depending on game type */
	int tsize; /* actual tile size for bowls */
	int padding, border; /* for frames: may vary on game type */
	SDL_Rect rBowl[3], rPreview[3], rHold[3], rScore[3];
	rHiscores = {0,0,0,0};
	if (demo || type == GT_NORMAL || type == GT_FIGURES) {
		/* initialize a single bowl with big score info and hiscores */
		tsize = (int)(renderer.getHeight() / 42)*2; /* get nearest even value to 21 tiles */
		int bx = (renderer.getWidth() - (tsize*BOWL_WIDTH))/2;
		int panelw = bx; /* space left/right of bowl */

		rBowl[0] = {bx, 0, tsize*BOWL_WIDTH, tsize*BOWL_HEIGHT};
		if (config.modern)
			rPreview[0] = {rBowl[0].x + rBowl[0].w + (panelw - 4*tsize)/2,
					2*tsize,4*tsize,10*tsize};
		else
			rPreview[0] = {rBowl[0].x + rBowl[0].w + (panelw - 4*tsize)/2,
					5*tsize,4*tsize,4*tsize};
		rHold[0] = {rPreview[0].x, 14*tsize, 4*tsize, 4*tsize};
		rScore[0] = {(panelw - 6*tsize)/2, 2*tsize, 6*tsize, 7*tsize};

		vbowls[0].init(0,tsize,rBowl[0],rPreview[0],rHold[0],rScore[0]);

		padding = tsize/4;
		border = tsize/3;

		/* hiscores is game level not bowl level (single player only) */
		if (!demo) {
			rHiscores = {(panelw - 6*tsize)/2, 11*tsize, 6*tsize, 8*tsize};
			addFrame(rHiscores,tsize/4,tsize/3);
		}
	} else if (type == GT_VSHUMAN || type == GT_VSCPU) {
		/* initialize two bowls with small score info below it */
		tsize = (int)(renderer.getHeight() / 48)*2; /* get nearest even value to 24 tiles */
		padding = tsize/6;
		border = tsize/4;
		/* get total width of bowl and attached preview and divide rest of
		 * screen width in three gaps of same size. */
		int bw = tsize*BOWL_WIDTH + border*2 + 2*padding + 4*tsize;
		int bgap = (renderer.getWidth() - 2*bw) / 3;

		rBowl[0] = {bgap, 0, tsize*BOWL_WIDTH, tsize*BOWL_HEIGHT};
		rBowl[1] = {bgap*2+bw, 0, tsize*BOWL_WIDTH, tsize*BOWL_HEIGHT};

		for (int i = 0; i < 2; i++) {
			if (config.modern)
				rPreview[i] = {rBowl[i].x + rBowl[i].w + border*2 + padding,
						rBowl[i].y + 2*tsize,
						4*tsize,10*tsize};
			else
				rPreview[i] = {rBowl[i].x + rBowl[i].w + border*2 + padding,
						rBowl[i].y + 5*tsize,
						4*tsize,4*tsize};
			rHold[i] = {rPreview[i].x, rBowl[i].y + 14*tsize,
					4*tsize, 4*tsize};
			rScore[i] = {rBowl[i].x + tsize,
					rBowl[i].y + rBowl[i].h + border*2 + padding,
					rBowl[i].w - tsize*2, tsize*3};
		}

		for (int i = 0; i < 2; i++)
			vbowls[i].init(i,tsize,rBowl[i],rPreview[i],rHold[i],rScore[i],true);
	} else if (type == GT_VSCPU2) {
		/* have 3 bowls with no preview/hold active */
		tsize = (int)(renderer.getHeight() / 48)*2; /* get nearest even value to 24 tiles */
		padding = tsize/6;
		border = tsize/4;
		int bw = tsize*BOWL_WIDTH + border*2;
		int bgap = (renderer.getWidth() - 3*bw) / 4;

		rBowl[0] = {bgap+border, 0, tsize*BOWL_WIDTH, tsize*BOWL_HEIGHT};
		rBowl[1] = {bgap*2+bw+border, 0, tsize*BOWL_WIDTH, tsize*BOWL_HEIGHT};
		rBowl[2] = {bgap*3+2*bw+border, 0, tsize*BOWL_WIDTH, tsize*BOWL_HEIGHT};

		for (int i = 0; i < 3; i++) {
			rPreview[i] = {0,0,0,0};
			rHold[i] = {0,0,0,0};
			rScore[i] = {rBowl[i].x + tsize,
					rBowl[i].y + rBowl[i].h + border*2 + padding,
					rBowl[i].w - tsize*2, tsize*3};
		}

		for (int i = 0; i < 3; i++)
			vbowls[i].init(i,tsize,rBowl[i],rPreview[i],rHold[i],rScore[i],true);
	}

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

	/* add frames and fixed text to background */
	for (int i = 0; i < MAXNUMPLAYERS; i++)
		if (vbowls[i].initialized()) {
			addFrame(vbowls[i].rBowl,0,border);
			if (vbowls[i].rPreview.w > 0)
				addFrame(vbowls[i].rPreview,padding,border);
			if (vbowls[i].rHold.w > 0)
				addFrame(vbowls[i].rHold,padding,border);
			addFrame(vbowls[i].rScore,padding,border);

			renderer.setTarget(background);
			theme.vbaFontNormal.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
			if (vbowls[i].rPreview.w > 0)
				theme.vbaFontNormal.write(vbowls[i].rPreview.x+vbowls[i].rPreview.w/2,
						vbowls[i].rPreview.y+tsize/2,_("Next"));
			if (vbowls[i].rHold.w > 0)
				theme.vbaFontNormal.write(vbowls[i].rHold.x+vbowls[i].rHold.w/2,
						vbowls[i].rHold.y+tsize/2,_("Hold"));
			renderer.clearTarget();
		}

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

/** Update bowls according to passed time @ms in milliseconds and input.
 * Return 1 if state switches to game over, 0 otherwise. */
bool VGame::update(uint ms, SDL_Event &ev, const Uint8 *gpstate) {
	BowlControls bc;

	if (state != VGS_RUNNING)
		return false;

	for (int i = 0; i < MAXNUMPLAYERS; i++)
		if (vbowls[i].initialized()) {
			if (vbowls[i].bowl->cpu_player)
				setBowlControlsCPU(bc,vbowls[i]);
			else
				setBowlControls(i, bc, ev, gpstate, vconfig.controls[i]);
			vbowls[i].update(ms,bc);
		}

	/* check for game over */
	int gameover = 0;
	int numbowls = 0;
	for (auto &vb : vbowls)
		if (vb.initialized()) {
			numbowls++;
			if (vb.bowl->game_over)
				gameover++;
		}
	if ((numbowls == 1 && gameover) || (numbowls > 1 && numbowls - gameover <= 1))
		gameover = 1;
	else
		gameover = 0;
	if (gameover) {
		if (numbowls > 1)
			for (auto &vb : vbowls)
				if (vb.initialized() && !vb.bowl->game_over) {
					bowl_finish_game(vb.bowl, 1);
					vb.winner = true;
				}
		state = VGS_GAMEOVER;
		return true;
	}
	return false;
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
