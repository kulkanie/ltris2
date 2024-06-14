/*
 * config.cpp
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

VConfig::VConfig()
{
	/* game */
	gametype = GAME_CLASSIC; /* demo, classic, ... */
	modern = 1; /* ghost piece, 3-piece-preview, ... */
	startinglevel = 0;
	playernames[0] = "Michael";
	playernames[1] = "Mr. X";
	playernames[2] = "Mr. Y";

	/* sounds */
	sound = 1;
	volume = 50;
	audiobuffersize = 1024;
	channels = 16;

	/* graphics */
	animations = 1;
	fullscreen = 1;
	fps = 1;
	showfps = 0;

	/* various */
	themeid = 0;
	themecount = 1;

	/* directory and file name */
	dname = CONFIGDIR;
	if (dname[0] == '~')
		dname = getHomeDir() + "/" + dname.substr(1);
	fname = dname + "/ltris2.conf";
	if (!dirExists(dname))
		makeDir(dname);

	/* load */
	_loginfo("Loading configuration %s\n",fname.c_str());
	FileParser fp(fname);

	fp.get("gametype", gametype);
	fp.get("modern", modern);
	fp.get("startinglevel", startinglevel);
	fp.get("player0", playernames[0]);
	fp.get("player1", playernames[1]);
	fp.get("player2", playernames[2]);
	fp.get("sound", sound);
	fp.get("volume", volume);
	fp.get("audiobuffersize", audiobuffersize);
	fp.get("channels", channels);
	fp.get("animations", animations);
	fp.get("fullscreen", fullscreen);
	fp.get("fps", fps);
	fp.get("showfps", showfps);
	fp.get("themeid", themeid);
	fp.get("themecount", themecount);
}

void VConfig::save()
{
	ofstream ofs(fname);
	if (!ofs.is_open()) {
		_logerr("Could not open config file %s\n",fname.c_str());
		return;
	}

	ofs << "gametype=" << gametype << "\n";
	ofs << "modern=" << modern << "\n";
	ofs << "startinglevel=" << startinglevel << "\n";
	ofs << "player0=" << playernames[0] << "\n";
	ofs << "player1=" << playernames[1] << "\n";
	ofs << "player2=" << playernames[2] << "\n";
	ofs << "sound=" << sound << "\n";
	ofs << "volume=" << volume << "\n";
	ofs << "audiobuffersize=" << audiobuffersize << "\n";
	ofs << "channels=" << channels << "\n";
	ofs << "animations=" << animations << "\n";
	ofs << "fullscreen=" << fullscreen << "\n";
	ofs << "fps=" << fps << "\n";
	ofs << "showfps=" << showfps << "\n";
	ofs << "themeid=" << themeid << "\n";
	ofs << "themecount=" << themecount << "\n";

	ofs.close();
	_loginfo("Configuration saved to %s\n",fname.c_str());
}


