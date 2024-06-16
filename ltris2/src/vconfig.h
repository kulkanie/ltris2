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
	MAXNUMPLAYERS = 3,

	FPS_50 = 0,
	FPS_60,
	FPS_200
};

typedef struct {
    int lshift; /* left shift */
    int rshift; /* right shift */
    int lrot; /* left rotation */
    int rrot; /* right rotation */
    int sdrop; /* soft drop */
    int hdrop; /* hard drop */
    int hold; /* switch hold piece */
} PControls;

class VConfig {
public:
	string dname;
	string fname;

	/* game */
	int gametype; /* demo, classic, ... */
	int modern; /* ghost piece, 3-piece-preview, ... */
	int startinglevel;
	string playernames[MAXNUMPLAYERS];
	PControls controls[MAXNUMPLAYERS];

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
