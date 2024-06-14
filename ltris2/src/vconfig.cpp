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
	playercount = 1;
	playernames[0] = "Michael";
	playernames[1] = "Mr.X";
	playernames[2] = "Mr.Y";
	playernames[3] = "Mr.Z";
	gamemode = 0;
	setsize = 1;
	matchsize = 2;
	closedelay = 3;
	motifcaption = 1; /* on shift is default */

	/* sounds */
	sound = 1;
	volume = 50;
	speech = 1;
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

	fp.get( "playercount", playercount );
	fp.get( "player0", playernames[0] );
	fp.get( "player1", playernames[1] );
	fp.get( "player2", playernames[2] );
	fp.get( "player3", playernames[3] );
	fp.get( "gamemode", gamemode );
	fp.get( "setsize", setsize );
	fp.get( "matchsize", matchsize );
	fp.get( "closedelay", closedelay );
	fp.get( "motifcaption", motifcaption );
	fp.get( "sound", sound );
	fp.get( "volume", volume );
	fp.get( "speech", speech );
	fp.get( "audiobuffersize", audiobuffersize );
	fp.get( "channels", channels );
	fp.get( "animations", animations );
	fp.get( "fullscreen", fullscreen );
	fp.get( "fps", fps );
	fp.get( "showfps", showfps );
	fp.get( "themeid", themeid );
	fp.get( "themecount", themecount );
}

void VConfig::save()
{
	ofstream ofs(fname);
	if (!ofs.is_open()) {
		_logerr("Could not open config file %s\n",fname.c_str());
		return;
	}

	ofs << "playercount=" << playercount << "\n";
	ofs << "player0=" << playernames[0] << "\n";
	ofs << "player1=" << playernames[1] << "\n";
	ofs << "player2=" << playernames[2] << "\n";
	ofs << "player3=" << playernames[3] << "\n";
	ofs << "gamemode=" << gamemode << "\n";
	ofs << "setsize=" << setsize << "\n";
	ofs << "matchsize=" << matchsize << "\n";
	ofs << "closedelay=" << closedelay << "\n";
	ofs << "motifcaption=" << motifcaption << "\n";
	ofs << "sound=" << sound << "\n";
	ofs << "volume=" << volume << "\n";
	ofs << "speech=" << speech << "\n";
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


