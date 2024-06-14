/*
 * config.h
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

#ifndef SRC_VCONFIG_H_
#define SRC_VCONFIG_H_

enum {
	GAME_DEMO = 0,
	GAME_CLASSIC,
	GAME_FIGURES,
	GAME_VS_HUMAN,
	GAME_VS_CPU,
	GAME_VS_HUMAN_HUMAN,
	GAME_VS_HUMAN_CPU,
	GAME_VS_CPU_CPU,
	GAME_TRAINING,
	GAME_TYPENUM,

	CS_DEFENSIVE = 0,
	CS_NORMAL,
	CS_AGGRESSIVE,

	MAXCONFIGPLAYERNAMES = 3,

	FPS_50 = 0,
	FPS_60,
	FPS_200
};

class VConfig {
public:
	string dname;
	string fname;

	/* game */
	int gametype; /* demo, classic, ... */
	int modern; /* ghost piece, 3-piece-preview, ... */
	int startinglevel;
	string playernames[MAXCONFIGPLAYERNAMES];
	//TODO Controls controls[MAXCONFIGPLAYERNAMES]; from libgame/config.h

	/* sound */
	int sound;
	int volume; /* 1 - 8 */
	int audiobuffersize;
	int channels; /* number of mix channels */

	/* graphics */
	int animations;
	int fullscreen; /* 0 = window, 1 = fullscreen */
	int fps; /* frames per second: 0 - no limit, 1 - 100, 2 - 200 */
	int showfps;

	/* various */
	int themeid; /* 0 == default theme */
	int themecount; /* to check and properly reset id if number of themes changed */

	VConfig();
	~VConfig() { save(); }
	void save();
};

#endif /* SRC_VCONFIG_H_ */
