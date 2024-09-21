/*
 * view.cpp
 * (C) 2018 by Michael Speck
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "tools.h"
#include "sdl.h"
#include "mixer.h"
#include "theme.h"
#include "sprite.h"
#include "menu.h"
#include "vconfig.h"
#include "hiscores.h"
#include "../libgame/bowl.h"
#include "vbowl.h"
#include "vgame.h"
#include "view.h"

extern SDL_Renderer *mrc;
extern Renderer renderer;
extern VConfig vconfig;
extern Theme theme;

View::View() : menuActive(true),
	  curMenu(NULL), graphicsMenu(NULL),
	  lblCredits1(true), lblCredits2(true),
	  noGameYet(true), changingKey(false),
	  state(VS_IDLE),
	  quitReceived(false),
	  fpsCycles(0), fpsStart(0), fps(0)
{
	mixer.open(vconfig.channels, vconfig.audiobuffersize);
	mixer.setVolume(vconfig.volume);
	mixer.setMute(!vconfig.sound);

	/* set game type names - needed for menu and hiscores */
	gameTypeNames.push_back(_("Normal"));
	gameTypeNames.push_back(_("Figures"));
	gameTypeNames.push_back(_("Vs Human"));
	gameTypeNames.push_back(_("Vs CPU"));
	gameTypeNames.push_back(_("Vs 2xCPU"));

	/* load theme names */
	readDir(string(DATADIR)+"/themes", RD_FOLDERS, themeNames);
	if ((uint)vconfig.themeid >= themeNames.size())
		vconfig.themeid = 0;
	vconfig.themecount = themeNames.size();

	/* gamepad */
	gamepad.open();
	if (gamepad.opened()) {
		printf("  NOTE: Gamepad cannot be configured via menu yet but you\n");
		printf("  can edit the gp_ entries in config file %s .\n",vconfig.fname.c_str());
		printf("  In case connection to gamepad gets lost, you can press F5 to reconnect.\n");
	}

	init(themeNames[vconfig.themeid], vconfig.fullscreen);
}

/** (Re)Initialize window, theme and menu.
 * t is theme name, f is fullscreen. */
void View::init(string t, uint f, bool reinit)
{
	_loginfo("%sInitializing view (theme=%s, fullscreen=%d)\n",
			reinit?_("(Re)"):"",t.c_str(),f);

	/* determine resolution */
	int idx = renderer.getDisplayId();
	int sw = 640, sh = 480;
	SDL_DisplayMode mode;
	SDL_GetCurrentDisplayMode(idx,&mode);
	if (f) {
		sw = mode.w;
		sh = mode.h;
	} else {
		/* window is half screen's width and 4:3 */
		sw = mode.w / 2;
		sh = 3 * sw / 4;
	}

	/* (re)create main window */
	renderer.create("LTris2", sw, sh, f );

	/* load theme */
	theme.load(t, renderer);

	/* create menu structure */
	createMenus();

	/* set label stuff */
	lblCredits1.setText(theme.fTooltip, "http://lgames.sf.net");
	lblCredits2.setText(theme.fTooltip, string("v")+PACKAGE_VERSION);

	/* start demo or reinitialize current game */
	if (reinit)
		game.init(GT_REINIT);
	else
		game.init(GT_DEMO);
	noGameYet = false;
}

/** Main game loop. Handle events, update game and render view.
 *  Handle menu overlay if active. */
void View::run()
{
	SDL_Event ev;
	Ticks ticks;
	Ticks renderTicks;
	int maxDelay, delay = 0;
	Uint32 ms;
	vector<string> text;
	string str;
	bool newEvent;

	state = VS_IDLE;
	sprites.clear();
	changingKey = false;
	keystate.reset();

	fpsStart = SDL_GetTicks();
	fpsCycles = 0;

	render();

	while (!quitReceived) {
		renderTicks.reset();

		/* handle global events */
		newEvent = false;
		if (SDL_PollEvent(&ev)) {
			newEvent = true;
			if (ev.type == SDL_QUIT)
				quitReceived = true;
			else if (ev.type == SDL_JOYBUTTONUP) {
				if (ev.jbutton.button == vconfig.gp_pause) {
					menuActive = !menuActive;
					if (!game.isDemo())
						game.pause(menuActive);
				}
			} else if (!changingKey && ev.type == SDL_KEYDOWN) {
				switch (ev.key.keysym.scancode) {
				case SDL_SCANCODE_F5:
					gamepad.close();
					gamepad.open();
					break;
				case SDL_SCANCODE_F6:
					if (!menuActive)
						vconfig.showfps = !vconfig.showfps;
					break;
				case SDL_SCANCODE_ESCAPE:
					if (!noGameYet && !menuActive) {
						menuActive = true;
						if (!game.isDemo())
							game.pause(true);
						/* clear event to not screw up
						 * handleMenuEvent below */
						ev.type = SDL_NOEVENT;
					}
					break;
				default:
					break;
				}
			}
		}

		/* get passed time */
		ms = ticks.get();

		/* update gamepad state */
		const Uint8 *gpadstate = gamepad.update();

		/* handle menu events */
		if (menuActive) {
			/* fake keydown event if gamepad is used
			 * XXX would be better to only check gamepad if no
			 * event happened but SDL_PollEvent sets ev.type
			 * to the internal event 0x7f00 SDL_EVENT_POLL_SENTINEL
			 * instead of leaving NOEVENT. since using this internal
			 * event might break something in the future we just
			 * always check for gamepad state and potentially
			 * overwrite another event. shouldn't hurt to bad. */
			if (checkMenuGamepadEvent(ms, gpadstate, ev))
				newEvent = true;

			if (newEvent && handleMenuEvent(ev)) {
				menuActive = false;
				if (!game.isDemo())
					game.pause(false);
			}
		}

		/* update menu */
		if (menuActive)
			curMenu->update(ms);

		/* update game */
		const Uint8 *ks = keystate.update();
		if (game.update(ms,ks,gpadstate)) {
			/* game over, save hiscores, only for single player */
			if (game.type == GT_NORMAL || game.type == GT_FIGURES) {
				HiscoreChart *hc = hiscores.get(gameTypeNames[game.type]);
				hc->add(game.vbowls[0].bowl->name,
						game.vbowls[0].bowl->level,
						game.vbowls[0].bowl->score.value);
			}
			hiscores.save();
		}

		/* update sprites */
		for (auto it = begin(sprites); it != end(sprites); ++it) {
			if ((*it).get()->update(ms))
				it = sprites.erase(it);
		}

		/* play sounds & create shrapnells */
		for (int i = 0; i < MAXNUMPLAYERS; i++)
			if (game.vbowls[i].initialized()) {
				if (game.vbowls[i].bowl->vbi.snd_shift)
					mixer.play(theme.sShift);
				if (game.vbowls[i].bowl->vbi.snd_insert)
					mixer.play(theme.sInsert);
				if (game.vbowls[i].bowl->vbi.snd_explosion)
					mixer.play(theme.sExplosion);
				if (game.vbowls[i].bowl->vbi.snd_nextlevel)
					mixer.play(theme.sNextLevel);
				if (game.vbowls[i].bowl->vbi.snd_tetris)
					mixer.play(theme.sTetris);

				/* create shrapnells */
				if (game.vbowls[i].bowl->vbi.cleared_line_count > 0)
					createShrapnells(game.vbowls[i]);

				/* clear view bowl info */
				memset(&game.vbowls[i].bowl->vbi,0,sizeof(ViewBowlInfo));
			}

		/* render */
		render();
		SDL_RenderPresent(mrc);

		/* stats */
		fpsCycles++;
		if (fpsStart < SDL_GetTicks())
			fps = 1000 * fpsCycles / (SDL_GetTicks() - fpsStart);
		if (fpsCycles > 100) {
			fpsCycles = 0;
			fpsStart = SDL_GetTicks();
		}

		/* get time for one cycle */
		if (vconfig.fps == FPS_50)
			maxDelay = 20;
		else if (vconfig.fps == FPS_60)
			maxDelay = 17;
		else
			maxDelay = 5; /* 200 fps */

		/* limit frame rate */
		delay = maxDelay - renderTicks.get(true);
		if (delay > 0)
			SDL_Delay(delay);
		SDL_FlushEvent(SDL_MOUSEMOTION); /* prevent event loop from dying */
	}

	if (!quitReceived)
		dim();
}

/** Render current game state. */
void View::render()
{
	/* game */
	game.render();

	/* render hiscores if region is set (single player only) */
	if (game.rHiscores.w > 0 && !menuActive)
		renderHiscore(game.rHiscores.x, game.rHiscores.y,
				game.rHiscores.w, game.rHiscores.h, renderer.isWidescreen());

	/* sprites */
	for (auto& s : sprites)
		s->render();

	/* menu */
	if (menuActive) {
		theme.menuBackground.copy();
		lblCredits1.copy(renderer.getWidth()-2,renderer.getHeight(),
					ALIGN_X_RIGHT | ALIGN_Y_BOTTOM);
		lblCredits2.copy(renderer.getWidth()-2,renderer.getHeight() - theme.fTooltip.getLineHeight(),
					ALIGN_X_RIGHT | ALIGN_Y_BOTTOM);
		curMenu->render();
	}

	/* stats */
	if (vconfig.showfps) {
		theme.fTooltip.setAlign(ALIGN_X_LEFT | ALIGN_Y_TOP);
		theme.fTooltip.write(0,0,to_string((int)fps));
	}
}

/* Dim screen to black. As game engine is already set for new state
 * we cannot use render() but need current screen state. */
void View::dim()
{
	Texture img;
	img.createFromScreen();

	for (uint8_t a = 250; a > 0; a -= 10) {
		SDL_SetRenderDrawColor(mrc,0,0,0,255);
		SDL_RenderClear(mrc);
		img.setAlpha(a);
		img.copy();
		SDL_RenderPresent(mrc);
		SDL_Delay(10);
	}

	/* make sure no input is given yet for next state */
	waitForInputRelease();
}

bool View::showInfo(const string &line, int type)
{
	vector<string> text;
	text.push_back(line);
	return showInfo(text, type);
}

/* Darken current screen content and show info text.
 * If confirm is false, wait for any key/click.
 * If confirm is true, wait for key y/n and return true false. */
bool View::showInfo(const vector<string> &text, int type)
{
	Font &font = theme.fTooltip;
	bool ret = true;
	uint h = text.size() * font.getLineHeight();
	int tx = renderer.getWidth()/2;
	int ty = (renderer.getHeight() - h)/2;

	darkenScreen();

	font.setAlign(ALIGN_X_CENTER | ALIGN_Y_TOP);
	for (uint i = 0; i < text.size(); i++) {
		font.write(tx,ty,text[i]);
		ty += font.getLineHeight();
	}

	SDL_RenderPresent(mrc);

	ret = waitForKey(type);
	return ret;
}

void View::createMenus()
{
	Menu *mNewGame, *mAudio, *mGraphics, *mMultiplayer;
	Menu *mControls, *mPlayer[2];
	const char *fpsLimitNames[] = {_("50 FPS"),_("60 FPS"),_("200 FPS") } ;
	const int bufSizes[] = { 256, 512, 1024, 2048, 4096 };
	const int channelNums[] = { 8, 16, 32 };
	const char *cpuStyleNames[] = {_("Defensive"), _("Normal"), _("Aggressive")};

	/* longer hints */
	string hStartingLevel = _("This is your starting level which will be ignored " \
			"for mode 'Figures' (always starts at level 0).\n\n"\
			"If not 0 the first level transition will require more lines "\
			"to be cleared (the higher the starting level the more lines).");
	string hGameMode = _("Normal: Starts with an empty bowl. Try to survive as long as possible.\n\n"\
			"Figures: Clear a different figure each level. Later on single blocks (level 7+) or lines (level 13+) will appear.\n\n"\
			"Vs Human/CPU/2xCPU: Play against another human or one or two CPU opponents.");
	string hGameStyle = _("Modern: Enables all that stuff that makes tetris "\
			"casual like ghost piece, wall-kicks, hold, "\
			"DAS charge during ARE (allows to shift next piece faster), "\
			"piece bag (at max 12 pieces before getting same piece again).\n\n"\
			"Classic: Doesn't give you any of this, making the game "\
			"really hard but also very interesting.");

	/* XXX too lazy to set fonts for each and every item...
	 * use static pointers instead */
	Menu::fCaption = &theme.fMenuCaption;
	MenuItem::fNormal = &theme.fMenuNormal;
	MenuItem::fFocus = &theme.fMenuFocus;
	MenuItem::fTooltip = &theme.fTooltip;
	MenuItem::tooltipWidth = 0.4 * renderer.w;

	rootMenu.reset(); /* delete any old menu ... */

	rootMenu = unique_ptr<Menu>(new Menu(theme)); /* .. or is assigning a new object doing it? */
	mNewGame = new Menu(theme,_("New Game"));
	mMultiplayer = new Menu(theme,_("Multiplayer Options"));
	mControls = new Menu(theme,_("Controls"));
	mPlayer[0] = new Menu(theme,_("Player 1"));
	mPlayer[1] = new Menu(theme,_("Player 2"));
	mAudio = new Menu(theme,_("Audio"));
	mGraphics = new Menu(theme,_("Graphics"));
	graphicsMenu = mGraphics; /* needed to return after mode/theme change */

	/* new game */
	mNewGame->add(new MenuItem(_("Start Game"),"",AID_STARTGAME));
	mNewGame->add(new MenuItemSep());
	mNewGame->add(new MenuItemEdit(_("Player 1"),vconfig.playernames[0]));
	mNewGame->add(new MenuItemEdit(_("Player 2"),vconfig.playernames[1]));
	//mNewGame->add(new MenuItemEdit(_("Player 3"),vconfig.playernames[2]));
	mNewGame->add(new MenuItemSep());
	mNewGame->add(new MenuItemList(_("Game Mode"),
			hGameMode,AID_NONE,vconfig.gametype,gameTypeNames));
	mNewGame->add(new MenuItemList(_("Game Style"),
			hGameStyle,AID_NONE,vconfig.modern,_("Classic"),_("Modern")));
	mNewGame->add(new MenuItemRange(_("Starting Level"),
			hStartingLevel,AID_NONE,vconfig.startinglevel,0,19,1));
	mNewGame->add(new MenuItemSep());
	mNewGame->add(new MenuItemSub(_("Multiplayer Options"),mMultiplayer));
	mNewGame->add(new MenuItemSep());
	mNewGame->add(new MenuItemBack(rootMenu.get()));

	/* multiplayer */
	mMultiplayer->add(new MenuItemRange(_("Holes"),
			_("Number of holes in sent lines."),
			AID_NONE,vconfig.mp_numholes,1,9,1));
	mMultiplayer->add(new MenuItemSwitch(_("Random Holes"),
			_("If multiple lines are sent, each line has different holes."),
			AID_NONE, vconfig.mp_randholes));
	mMultiplayer->add(new MenuItemSep());
	mMultiplayer->add(new MenuItemList(_("CPU Style"),
			_("Defensive: Clears any lines.\nNormal: Mostly goes for two lines.\nAggressive: Tries to complete three or four lines (on modern only).\n\nIn general: The more aggressive the style, the more priority is put on completing multiple lines at the expense of a balanced bowl contents."),
			AID_NONE,vconfig.cpu_style,cpuStyleNames,3));
	mMultiplayer->add(new MenuItemRange(_("CPU Drop Delay"),
			"Delay (in ms) before CPU starts dropping a piece.",
			AID_NONE,vconfig.cpu_delay,0,2000,100));
	mMultiplayer->add(new MenuItemRange(_("CPU Speed"),
			"Multiplier in percent for dropping speed of pieces, e.g.,\n50% = half the regular speed\n100% = regular speed\n200% = doubled speed\nCan range between 50% and 400%.",
			AID_NONE,vconfig.cpu_sfactor,50,400,25));
	mMultiplayer->add(new MenuItemSep());
	mMultiplayer->add(new MenuItemBack(mNewGame));

	/* controls */
	mControls->add(new MenuItemSub(_("Player 1"),mPlayer[0]));
	mControls->add(new MenuItemSub(_("Player 2"),mPlayer[1]));
	mControls->add(new MenuItemSep());
	mControls->add(new MenuItemRange(_("Auto-Shift Delay"),
			_("Initial delay before auto shift starts. Classic DAS has 270."),
			AID_NONE,vconfig.as_delay,20,300,10));
	mControls->add(new MenuItemRange(_("Auto-Shift Speed"),
			_("Delay between auto shift steps. Classic DAS has 100."),
			AID_NONE,vconfig.as_speed,20,200,10));
	mControls->add(new MenuItemSep());
	mControls->add(new MenuItemBack(rootMenu.get()));

	/* player keys */
	for (int i = 0; i < 2; i++) {
		string hCtrls = _("Left/Right: horizontal movement\n" \
				"Rotate Left/Right: piece rotation\n" \
				"Down: fast soft drop\n" \
				"Drop: INSTANT hard drop\n" \
				"Hold: put current piece to hold (only for modern style)");
		mPlayer[i]->add(new MenuItemKey(_("Left"),hCtrls, vconfig.controls[i].lshift));
		mPlayer[i]->add(new MenuItemKey(_("Right"),hCtrls, vconfig.controls[i].rshift));
		mPlayer[i]->add(new MenuItemKey(_("Rotate Left"),hCtrls, vconfig.controls[i].lrot));
		mPlayer[i]->add(new MenuItemKey(_("Rotate Right"),hCtrls, vconfig.controls[i].rrot));
		mPlayer[i]->add(new MenuItemKey(_("Down"),hCtrls, vconfig.controls[i].sdrop));
		mPlayer[i]->add(new MenuItemKey(_("Drop"),hCtrls, vconfig.controls[i].hdrop));
		mPlayer[i]->add(new MenuItemKey(_("Hold"),hCtrls, vconfig.controls[i].hold));
		mPlayer[i]->add(new MenuItemSep());
		mPlayer[i]->add(new MenuItemBack(mControls));
	}

	/* graphics */
	mGraphics->add(new MenuItemList(_("Theme"),
			_("Select theme. (not applied yet)"),
			AID_NONE,vconfig.themeid,themeNames));
	mGraphics->add(new MenuItemList(_("Mode"),
			_("Select mode. (not applied yet)"),
			AID_NONE,vconfig.fullscreen,_("Window"),_("Fullscreen")));
	mGraphics->add(new MenuItem(_("Apply Theme&Mode"),
			_("Apply the above settings."),AID_APPLYTHEMEMODE));
	mGraphics->add(new MenuItemSep());
	mGraphics->add(new MenuItemSwitch(_("Animations"),"",AID_NONE,
			vconfig.animations));
	mGraphics->add(new MenuItemSwitch(_("Smooth Drop"),
			_("Drop piece block-by-block or smoothly. Does not affect drop speed."),
			AID_NONE,vconfig.smoothdrop));
	mGraphics->add(new MenuItemList(_("Frame Limit"),
			"Maximum number of frames per second.",
			AID_NONE,vconfig.fps,fpsLimitNames,3));
	mGraphics->add(new MenuItemSep());
	mGraphics->add(new MenuItemBack(rootMenu.get()));

	mAudio->add(new MenuItemSwitch(_("Sound"),"",AID_SOUND,vconfig.sound));
	mAudio->add(new MenuItemRange(_("Volume"),"",AID_VOLUME,vconfig.volume,0,100,10));
	mAudio->add(new MenuItemSep());
	mAudio->add(new MenuItemIntList(_("Buffer Size"),
			_("Reduce buffer size if you experience sound delays. Might have more CPU impact though. (not applied yet)"),
			vconfig.audiobuffersize,bufSizes,5));
	mAudio->add(new MenuItemIntList(_("Channels"),
			_("More channels gives more sound variety, less channels less (not applied yet)"),vconfig.channels,
			channelNums,3));
	mAudio->add(new MenuItem(_("Apply Size&Channels"),
			_("Apply above settings"),AID_APPLYAUDIO));
	mAudio->add(new MenuItemSep());
	mAudio->add(new MenuItemBack(rootMenu.get()));

	rootMenu->add(new MenuItemSub(_("New Game"), mNewGame));
	rootMenu->add(new MenuItemSep());
	rootMenu->add(new MenuItemSub(_("Controls"), mControls));
	rootMenu->add(new MenuItemSub(_("Graphics"),mGraphics));
	rootMenu->add(new MenuItemSub(_("Audio"),mAudio));
	rootMenu->add(new MenuItemSep());
	rootMenu->add(new MenuItem(_("Quit"), "", AID_QUIT));

	rootMenu->adjust();

	curMenu = rootMenu.get();
}

int View::waitForKey(int type)
{
	SDL_Event ev;
	bool ret = true;
	bool leave = false;

	SDL_PumpEvents();
	SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
	while (!leave) {
		/* handle events */
		if (SDL_WaitEvent(&ev)) {
			if (ev.type == SDL_QUIT)
				quitReceived = leave = true;
			if (type == WT_ANYKEY && (ev.type == SDL_KEYUP ||
						ev.type == SDL_MOUSEBUTTONUP))
				leave = true;
			else if (ev.type == SDL_KEYUP) {
				int sc = ev.key.keysym.scancode;
				if (type == WT_YESNO) {
					if (sc == SDL_SCANCODE_Y || sc == SDL_SCANCODE_Z)
						ret = leave = true;
					if (sc == SDL_SCANCODE_N || sc == SDL_SCANCODE_ESCAPE) {
						ret = false;
						leave = true;
					}
				} else if (type == WT_PAUSE)
					if (sc == SDL_SCANCODE_P)
						ret = leave = true;
			}
		}
		SDL_FlushEvent(SDL_MOUSEMOTION);
	}
	SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
	return ret;
}

void View::darkenScreen(int alpha)
{
	Texture img;
	img.createFromScreen();
	SDL_SetRenderDrawColor(mrc,0,0,0,255);
	SDL_RenderClear(mrc);
	img.setAlpha(alpha);
	img.copy();
}

/* wait up to 1s for release of all buttons and keys.
 * Clear SDL event loop afterwards.
 */
void View::waitForInputRelease()
{
	SDL_Event ev;
	bool leave = false;
	int timeout = 500, numkeys;
	Ticks ticks;

	SDL_PumpEvents();
	SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
	while (!leave) {
		if (SDL_PollEvent(&ev) && ev.type == SDL_QUIT)
			quitReceived = leave = true;
		leave = true;
		if (SDL_GetMouseState(NULL, NULL))
			leave = false;
		const Uint8 *keystate = SDL_GetKeyboardState(&numkeys);
		for (int i = 0; i < numkeys; i++)
			if (keystate[i]) {
				leave = false;
				break;
			}
		timeout -= ticks.get();
		if (timeout <= 0)
			leave = true;
		SDL_Delay(10);
	}
	SDL_PumpEvents();
	SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
}

/** Check if gamepad is used to navigate menu and return as fake
 * key down event in @ev. */
bool View::checkMenuGamepadEvent(int ms, const Uint8 *gpadstate, SDL_Event &ev)
{
	int gp2sc[6][2] = {
		{GPAD_UP, SDL_SCANCODE_UP},
		{GPAD_DOWN, SDL_SCANCODE_DOWN},
		{GPAD_RIGHT, SDL_SCANCODE_RIGHT},
		{GPAD_LEFT, SDL_SCANCODE_LEFT},
		{GPAD_BUTTON0 + vconfig.gp_rrot, SDL_SCANCODE_RIGHT},
		{GPAD_BUTTON0 + vconfig.gp_lrot, SDL_SCANCODE_LEFT}
	};

	for (int i = 0; i < 6; i++)
		if (gpadstate[gp2sc[i][0]] == GPBS_DOWN) {
			/* initial button press */
			ev.type = SDL_KEYDOWN;
			ev.key.keysym.scancode = (SDL_Scancode)gp2sc[i][1];
			gpDelay.init(SCT_REPEAT, 0, 1, 250);
			return true;
		} else if (gpadstate[gp2sc[i][0]] == GPBS_PRESSED &&
							gpDelay.update(ms)) {
			/* button is continuously down, so have some delay */
			ev.type = SDL_KEYDOWN;
			ev.key.keysym.scancode = (SDL_Scancode)gp2sc[i][1];
			return true;
		}

	return false;
}


/** Handle menu event and update menu items and state.
 * Return true if leaving root menu requested, false otherwise. */
bool View::handleMenuEvent(SDL_Event &ev)
{
	bool ret = false;
	int aid = AID_NONE;
	MenuItemSub *subItem;
	MenuItemBack *backItem;

	if (changingKey) {
		/* changing a key is completely blocking */
		if (ev.type == SDL_KEYDOWN) {
			MenuItemKey *keyItem = dynamic_cast<MenuItemKey*>
			(curMenu->getCurItem());
			switch (ev.key.keysym.scancode) {
			case SDL_SCANCODE_ESCAPE:
				keyItem->cancelChange();
				changingKey = false;
				break;
			default:
				keyItem->setKey(ev.key.keysym.scancode);
				changingKey = false;
				break;
			}
		}
	} else if (ev.type == SDL_KEYDOWN &&
			ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
		Menu *lm = curMenu->getLastMenu();
		if (lm)
			curMenu = lm;
		else
			ret = true; /* no previous menu = root menu */
	} else if ((aid = curMenu->handleEvent(ev))) {
		if (aid != AID_FOCUSCHANGED)
			mixer.play(theme.sMenuClick);
		switch (aid) {
		case AID_FOCUSCHANGED:
			mixer.play(theme.sMenuMotion);
			break;
		case AID_QUIT:
			quitReceived = true;
			break;
		case AID_ENTERMENU:
			subItem = dynamic_cast<MenuItemSub*>(curMenu->getCurItem());
			if (subItem) { /* should never fail */
				curMenu = subItem->getSubMenu();
			} else
				_logerr("Oops, submenu not found...\n");
			break;
		case AID_LEAVEMENU:
			backItem = dynamic_cast<MenuItemBack*>(curMenu->getCurItem());
			if (backItem) { /* should never fail */
				curMenu = backItem->getLastMenu();
			} else
				_logerr("Oops, last menu not found...\n");
			break;
		case AID_CHANGEKEY:
			changingKey = true;
			break;
		case AID_SOUND:
			mixer.setMute(!vconfig.sound);
			break;
		case AID_VOLUME:
			mixer.setVolume(vconfig.volume);
			break;
		case AID_APPLYAUDIO:
			mixer.close();
			mixer.open(vconfig.channels, vconfig.audiobuffersize);
			break;
		case AID_APPLYTHEMEMODE:
			/* XXX workaround for SDL bug: clear event
			 * loop otherwise left mouse button event is
			 * screwed for the first click*/
			waitForInputRelease();
			init(themeNames[vconfig.themeid],vconfig.fullscreen,true);
			curMenu = graphicsMenu;
			break;
		case AID_HELP:
			//showHelp();
			break;
		case AID_STARTGAME:
			menuActive = false;
			waitForInputRelease();
			game.init(vconfig.gametype);
			keystate.reset();
			break;
		}
	}

	return ret;
}

/** Create shrapnells from view bowl info. */
void View::createShrapnells(VBowl &vb)
{
	Bowl *b = vb.bowl;
	int anim = rand() % 8;

	if (b == NULL)
		return;

	if (!vconfig.animations)
		anim = -1; /* only fading */

	for (int j = 0; j < b->vbi.cleared_line_count; j++) {
		int ly = b->vbi.cleared_line_y[j];
		Vector pos(vb.rBowl.x, vb.rBowl.y + vb.blockSize*ly);
		Vector vel, grav;

		for (int i = 0; i < vb.w; i++) {
			if (b->vbi.cleared_lines[j][i] == -1) {
				/* game over animation may have gaps */
				pos.setX(pos.getX() + vb.blockSize);
				continue;
			}

			setShrapnellVelGrav(vb, anim, i, vel, grav);

			Shrapnell *s = new Shrapnell(
				theme.vbaBlocks[b->vbi.cleared_lines[j][i]],
				pos, vel, grav, 192, 0.64);
			sprites.push_back(unique_ptr<Sprite>(s));

			pos.setX(pos.getX() + vb.blockSize);
		}
	}
}

/** Set velocity @v and gravity @g for animation @type and index @xid (0-9)
 * in a cleared line for VBowl @vb. */
void View::setShrapnellVelGrav(VBowl &vb, int type, int xid, Vector &v, Vector &g)
{
	/* set ratio to convert libgame 640x480 geometry to screen resolution
	 * we need height ratio only since that defines size of the bowl */
	double ratio = renderer.getHeight() / 480.0;

	v.set(0,0);
	g.set(0,0);

	/* if game over, this is the final animation. use special setting. */
	if (vb.bowl->game_over) {
		v.setY(ratio * -0.05);
		g.setY(ratio * 0.0002);
		return;
	}

	switch (type) {
	case 0:
		/* simple sideways animation:
		 * move blocks 0-4 to the left side and 5-9 to the right
		 * each block takes 300 ms to get to the side (px/ms = i*20/300)
		 */
		if (xid < vb.w/2)
			v.setX(ratio * (-xid) * 0.0667);
		else
			v.setX(ratio * (vb.w - 1 - xid) * 0.0667);
		break;
	case 1:
	case 2:
		/* move blocks up with in/decreasing speed and some gravity */
		if (type == 1)
			v.setY(ratio * (vb.w - xid) * -0.015);
		else
			v.setY(ratio * (xid + 1) * -0.015);
		g.set(0,ratio*0.0002);
		break;
	case 3:
	case 4:
		/* similiar to last effect but this time the fastest blocks are
		 * in the middle (4) or at the sides (5)
		 */
		if (type == 3) {
			if (xid < vb.w/2)
				v.setY(ratio * (xid+1) * -0.016);
			else
				v.setY(ratio * (vb.w-xid) * -0.016);
		} else {
			if (xid < vb.w/2)
				v.setY(ratio * (vb.w/2 - xid) *-0.016);
			else
				v.setY(ratio * (xid + 1 - vb.w/2) *-0.016);
		}
		g.set(0,ratio*0.0002);
		break;
	case 5:
		/* opposite of 1: move blocks horizontally to the middle */
		if (xid < vb.w/2)
			v.setX(ratio * (vb.w/2 - xid - 1) * 0.0667);
		else
			v.setX(ratio * (xid - vb.w/2) * -0.0667);
		break;
	case 6:
	case 7:
		if (type == 6) {
			v.setX(ratio * (xid+1) * -0.02);
			v.setY(ratio * (vb.w-xid) * -0.01);
		} else {
			v.setX(ratio * (vb.w-xid) * 0.02);
			v.setY(ratio * (xid+1) * -0.01);
		}
		g.set(0,ratio*0.0002);
		break;
	default:
		/* no animation, just fade out */
		break;
	}
}

/* Render hiscore at given screen region. */
/* TODO: pre-render on update and just copy texture here */
void View::renderHiscore(int x, int y, int w, int h, bool detailed)
{
	Font &fTitle = theme.vbaFontTiny;
	Font &fEntry = theme.vbaFontTiny;

	if (game.type != GT_NORMAL && game.type != GT_FIGURES)
		return;

	HiscoreChart *chart = hiscores.get(gameTypeNames[game.type]);

	int cy = y + (h - fTitle.getLineHeight() - 10*fEntry.getLineHeight())/2;

	string str = _("Hiscores");
	//if (detailed)
	//	str = str + " '" + chart->getName() + "'";
	fTitle.setAlign(ALIGN_X_CENTER | ALIGN_Y_TOP);
	fTitle.write(x + w/2, cy, str);
	cy += fTitle.getLineHeight();
	//if (detailed)
	//	cy += fTitle.getLineHeight();

	int cx_num = x;
	int cx_name = x + 3*fEntry.getCharWidth('0');
	int cx_level = x + 9*fEntry.getCharWidth('W');
	int cx_score = x + w;

	for (int i = 0; i < CHARTSIZE; i++) {
		const ChartEntry *entry = chart->get(i);
		if (entry->newEntry)
			fEntry.setColor(theme.fontColorHighlight);
		else
			fEntry.setColor(theme.fontColorNormal);

		fEntry.setAlign(ALIGN_X_LEFT | ALIGN_Y_TOP);

		/* number */
		str = to_string(i+1) + ".";
		fEntry.write(cx_num, cy, str);
		/* name */
		if (entry->name.length() > 8)
			fEntry.write(cx_name, cy, entry->name.substr(0,7)+"...");
		else
			fEntry.write(cx_name, cy, entry->name);
		/* level */
		if (detailed)
			fEntry.write(cx_level, cy, "L" + to_string(entry->level+1));
		/* score */
		fEntry.setAlign(ALIGN_X_RIGHT | ALIGN_Y_TOP);
		fEntry.write(cx_score, cy, to_string(entry->score));

		cy += fEntry.getLineHeight();
	}
	fEntry.setColor(theme.fontColorNormal);
}
