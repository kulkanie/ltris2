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
#include "menu.h"
#include "vcharts.h"
#include "vgame.h"
#include "view.h"

extern SDL_Renderer *mrc;

View::View(Renderer &r)
	: renderer(r), menuActive(true),
	  curMenu(NULL), graphicsMenu(NULL),
	  noGameYet(true),
	  state(VS_IDLE),
	  quitReceived(false),
	  fpsCycles(0), fpsStart(0), fps(0)
{
	mixer.open(vconfig.channels, vconfig.audiobuffersize);
	mixer.setVolume(vconfig.volume);
	mixer.setMute(!vconfig.sound);

	/* load theme names */
	readDir(string(DATADIR)+"/themes", RD_FOLDERS, themeNames);
	if ((uint)vconfig.themeid >= themeNames.size())
		vconfig.themeid = 0;
	vconfig.themecount = themeNames.size();

	init(themeNames[vconfig.themeid], vconfig.fullscreen);
}

/** (Re)Initialize window, theme and menu.
 * t is theme name, f is fullscreen. */
void View::init(string t, uint f)
{
	_loginfo("Initializing View (Theme=%s, Fullscreen=%d)\n",t.c_str(),f);

	/* determine resolution */
	int sw = 640, sh = 480;
	SDL_DisplayMode mode;
	SDL_GetCurrentDisplayMode(0,&mode);
	if (f) {
		sw = mode.w;
		sh = mode.h;
		_loginfo("Using fullscreen resolution %dx%d\n",mode.w,mode.h);
	} else {
		sw = mode.w / 2;
		sh = 3 * sw / 4;
		_loginfo("Using window resolution %dx%d\n",sw,sh);
	}

	/* (re)create main window */
	renderer.create("LPairs2", sw, sh, f );

	/* load theme */
	theme.load(t,renderer);

	/* create menu structure */
	createMenus();

	/* set label stuff */
	lblCredits1.setText(theme.fSmall, "http://lgames.sf.net");
	lblCredits2.setText(theme.fSmall, string("v")+PACKAGE_VERSION);

	/* create render images and positions */
	background.create(sw,sh);
	background.setBlendMode(0);

	/* game */
	if (noGameYet) {
		changeWallpaper();
		// TODO init demo game here
	} else
		;// TODO start regular game
}

/** Main game loop. Handle events, update game and render view.
 *  Handle menu overlay if active. */
void View::run()
{
	SDL_Event ev;
	int flags;
	Ticks ticks;
	Ticks renderTicks;
	int maxDelay, delay = 0;
	Uint32 ms;
	vector<string> text;
	string str;
	int button = 0; /* pressed button */
	int buttonX = 0, buttonY = 0; /* position if pressed */

	state = VS_IDLE;

	fpsStart = SDL_GetTicks();
	fpsCycles = 0;

	if (vconfig.fps == FPS_50)
		maxDelay = 20;
	else if (vconfig.fps == FPS_60)
		maxDelay = 17;
	else
		maxDelay = 5;

	render();

	while (!quitReceived) {
		renderTicks.reset();

		/* handle events */
		button = 0; /* none pressed */
		if (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT)
				quitReceived = true;
			else if (ev.type == SDL_KEYUP) {
				switch (ev.key.keysym.scancode) {
				case SDL_SCANCODE_F:
					if (!menuActive)
						vconfig.showfps = !vconfig.showfps;
					break;
				case SDL_SCANCODE_R:
					if (!menuActive)
						; // TODO restart game
					break;
				case SDL_SCANCODE_ESCAPE:
					if (!noGameYet)
						menuActive = !menuActive;
					break;
				default:
					break;
				}
			}
			if (menuActive)
				handleMenuEvent(ev);
			else if (ev.type == SDL_MOUSEBUTTONUP && state == VS_IDLE) {
				button = ev.button.button;
				buttonX = ev.button.x;
				buttonY = ev.button.y;
			}
		}

		/* get key state */
		const Uint8 *keystate = SDL_GetKeyboardState(NULL);

		/* get passed time */
		ms = ticks.get();

		/* update menu */
		if (menuActive)
			curMenu->update(ms);

		/* TODO update game */

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
	/* background */
	background.copy();

	/* menu */
	if (menuActive) {
		theme.menuBackground.copy();
		lblCredits1.copy(renderer.getWidth()-2,renderer.getHeight(),
					ALIGN_X_RIGHT | ALIGN_Y_BOTTOM);
		lblCredits2.copy(renderer.getWidth()-2,renderer.getHeight() - theme.fSmall.getLineHeight(),
					ALIGN_X_RIGHT | ALIGN_Y_BOTTOM);
		curMenu->render();
	}

	/* stats */
	if (vconfig.showfps) {
		theme.fSmall.setAlign(ALIGN_X_LEFT | ALIGN_Y_TOP);
		theme.fSmall.write(0,0,to_string((int)fps));
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
	Font &font = theme.fSmall;
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
	Menu *mNewGame, *mAudio, *mGraphics;
	const char *fpsLimitNames[] = {_("50 FPS"),_("60 FPS"),_("200 FPS") } ;
	const int bufSizes[] = { 256, 512, 1024, 2048, 4096 };
	const int channelNums[] = { 8, 16, 32 };
	const char *typeNames[] = {_("Demo"), _("Normal"), _("Figures"), _("Vs CPU")};

	/* XXX too lazy to set fonts for each and every item...
	 * use static pointers instead */
	MenuItem::fNormal = &theme.fMenuNormal;
	MenuItem::fFocus = &theme.fMenuFocus;
	MenuItem::fTooltip = &theme.fSmall;
	MenuItem::tooltipWidth = 0.4 * renderer.w;

	rootMenu.reset(); /* delete any old menu ... */

	rootMenu = unique_ptr<Menu>(new Menu(theme)); /* .. or is assigning a new object doing it? */
	mNewGame = new Menu(theme);
	//mOptions = new Menu(theme);
	mAudio = new Menu(theme);
	mGraphics = new Menu(theme);
	graphicsMenu = mGraphics; /* needed to return after mode/theme change */

	mNewGame->add(new MenuItem(_("Start Game"),"",AID_STARTGAME));
	mNewGame->add(new MenuItemSep());
	mNewGame->add(new MenuItemList(_("Game Type"),
			_("You can play alone or against a human or CPU opponent."),
			AID_NONE,vconfig.gametype,typeNames,4));
	mNewGame->add(new MenuItemList(_("Game Style"),
			_("Modern or classic."),
			AID_NONE,vconfig.modern,_("Classic"),_("Modern")));
	mNewGame->add(new MenuItemRange(_("Starting Level"),
			"Starting level between 0 and 19.",
			AID_NONE,vconfig.startinglevel,0,19,1));
	mNewGame->add(new MenuItemEdit(_("Player Name"),vconfig.playernames[0]));

	mNewGame->add(new MenuItemSep());
/*	mNewGame->add(new MenuItemRange(_("Players"),
			_("Number and names of players. Players alternate whenever a life is lost."),
			AID_NONE,config.playercount,1,MAX_PLAYERS,1));
	mNewGame->add(new MenuItemEdit(_("1st"),config.playernames[0]));
	mNewGame->add(new MenuItemEdit(_("2nd"),config.playernames[1]));
	mNewGame->add(new MenuItemEdit(_("3rd"),config.playernames[2]));
	mNewGame->add(new MenuItemEdit(_("4th"),config.playernames[3]));
	mNewGame->add(new MenuItemSep());*/
	mNewGame->add(new MenuItemBack(rootMenu.get()));

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
	mGraphics->add(new MenuItemList(_("Frame Limit"),
			"Maximum number of frames per second.",
			AID_NONE,vconfig.fps,fpsLimitNames,3));
	mGraphics->add(new MenuItemSep());
	mGraphics->add(new MenuItemBack(rootMenu.get()));

	mAudio->add(new MenuItemSwitch(_("Sound"),"",AID_SOUND,vconfig.sound));
	mAudio->add(new MenuItemRange(_("Volume"),"",AID_VOLUME,vconfig.volume,0,100,10));
	//mAudio->add(new MenuItemSwitch(_("Speech"),"",AID_NONE,config.speech));
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

	/* mOptions->add(new MenuItemSub(_("Graphics"),mGraphics));
	mOptions->add(new MenuItemSub(_("Audio"),mAudio));
	mOptions->add(new MenuItemBack(rootMenu.get())); */

	rootMenu->add(new MenuItemSub(_("New Game"), mNewGame));
	rootMenu->add(new MenuItemSep());
	rootMenu->add(new MenuItemSub(_("Graphics"),mGraphics));
	rootMenu->add(new MenuItemSub(_("Audio"),mAudio));
	//rootMenu->add(new MenuItemSub(_("Settings"), mOptions));
	//rootMenu->add(new MenuItem(_("Help"), "", AID_HELP));
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

void View::handleMenuEvent(SDL_Event &ev)
{
	int aid = AID_NONE;
	MenuItemSub *subItem;
	MenuItemBack *backItem;

	if (ev.type == SDL_QUIT)
		quitReceived = true;
	else if (ev.type == SDL_KEYDOWN &&
			ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
		Menu *lm = curMenu->getLastMenu();
		if (lm)
			curMenu = lm;
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
			init(themeNames[vconfig.themeid],vconfig.fullscreen);
			curMenu = graphicsMenu;
			break;
		case AID_HELP:
			//showHelp();
			break;
		case AID_STARTGAME:
			// TODO start game
			menuActive = false;
			waitForInputRelease();
			break;
		}
	}
}

void View::changeWallpaper()
{
	curWallpaperId = rand() % theme.numWallpapers;
	renderer.setTarget(background);
	theme.wallpapers[curWallpaperId].copy();
	renderer.clearTarget();
}
