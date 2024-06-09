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

#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_

enum {
	MAXCONFIGPLAYERNAMES = 4
};

class Config {
public:
	string dname;
	string fname;

	/* game */
	int playercount;
	string playernames[MAXCONFIGPLAYERNAMES];
	int gamemode;
	int setsize;
	int matchsize;
	int closedelay;
	int motifcaption;

	/* sound */
	int sound;
	int volume; /* 1 - 8 */
	int speech; /* enable speech? */
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

	Config();
	~Config() { save(); }
	void save();
};

#endif /* SRC_CONFIG_H_ */
