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
	VS_IDLE = 0,

	/* waitForKey types */
	WT_ANYKEY = 0,
	WT_YESNO,
	WT_PAUSE
};

class View {
	/* general */
	Mixer mixer;
	Gamepad gamepad;

	/* menu */
	bool menuActive;
	unique_ptr<Menu> rootMenu;
	Menu *curMenu, *graphicsMenu;
	vector<string> themeNames;
	Label lblCredits1, lblCredits2;
	bool noGameYet;
	bool changingKey;
	SmoothCounter gpDelay;

	/* game */
	vector<string> gameTypeNames;
	Hiscores hiscores;
	VGame game;  // wrapper for libgame/tetris.c
	int state;
	bool quitReceived;
	uint curWallpaperId;
	list<unique_ptr<Sprite>> sprites;

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
	bool checkMenuGamepadEvent(int ms, const Uint8 *gpadstate, SDL_Event &ev);
	bool handleMenuEvent(SDL_Event &ev);
	void createShrapnells(VBowl &vb);
	void setShrapnellVelGrav(VBowl &vb, int type, int xid, Vector &v, Vector &g);
	void renderHiscore(int x, int y, int w, int h, bool detailed);
public:
	View();
	void init(string t, uint f, bool reinit=false);
	void run();
	void render();
};

#endif /* VIEW_H_ */
