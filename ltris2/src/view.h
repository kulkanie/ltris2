/*
 * view.h
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

#ifndef VIEW_H_
#define VIEW_H_

enum {
	/* general */
	MAX_PLAYERS = 4,

	/* game states */
	VS_IDLE = 0,

	/* mixer */
	MIX_CHANNELNUM = 16,
	MIX_CUNKSIZE = 2048,

	/* waitForKey types */
	WT_ANYKEY = 0,
	WT_YESNO,
	WT_PAUSE
};

class View {
	/* general */
	Renderer &renderer;
	VConfig &config;
	Theme theme;
	Mixer mixer;

	/* menu */
	bool menuActive;
	unique_ptr<Menu> rootMenu;
	Menu *curMenu, *graphicsMenu;
	vector<string> themeNames;
	Label lblCredits1, lblCredits2;
	bool noGameYet;

	/* game */
	int state;
	bool quitReceived;
	uint curWallpaperId;
	Texture imgBackground;

	/* stats */
	Uint32 fpsCycles, fpsStart;
	double fps;

	void createMenus();
	void waitForInputRelease();
	int waitForKey(int type);
	void darkenScreen(int alpha = 32);
	bool showInfo(const string &line, int type);
	bool showInfo(const vector<string> &text, int type);
	void dim();
	void handleMenuEvent(SDL_Event &ev);
	void changeWallpaper();
public:
	View(Renderer &r, VConfig &cfg);
	void init(string t, uint f);
	void run();
	void render();
};

#endif /* VIEW_H_ */
