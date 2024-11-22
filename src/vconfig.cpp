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

VConfig::VConfig() // @suppress("Class members should be properly initialized")
{
	/* game */
	gametype = 0; /* normal, figures, ... */
	modern = 1; /* ghost piece, 3-piece-preview, ... */
	startinglevel = 0;
	playernames[0] = "Mike";
	playernames[1] = "Tom";
	playernames[2] = "Chris";

	/* controls */
	controls[0].lshift = SDL_SCANCODE_LEFT;
	controls[0].rshift = SDL_SCANCODE_RIGHT;
	controls[0].lrot = SDL_SCANCODE_1;
	controls[0].rrot = SDL_SCANCODE_2;
	controls[0].sdrop = SDL_SCANCODE_DOWN;
	controls[0].hdrop = SDL_SCANCODE_END;
	controls[0].hold = SDL_SCANCODE_DELETE;
	controls[1].lshift = SDL_SCANCODE_A;
	controls[1].rshift = SDL_SCANCODE_D;
	controls[1].lrot = SDL_SCANCODE_W;
	controls[1].rrot = SDL_SCANCODE_E;
	controls[1].sdrop = SDL_SCANCODE_S;
	controls[1].hdrop = SDL_SCANCODE_Y;
	controls[1].hold = SDL_SCANCODE_Q;
	controls[2].lshift = SDL_SCANCODE_KP_1;
	controls[2].rshift = SDL_SCANCODE_KP_3;
	controls[2].lrot = SDL_SCANCODE_KP_5;
	controls[2].rrot = SDL_SCANCODE_KP_6;
	controls[2].sdrop = SDL_SCANCODE_KP_2;
	controls[2].hdrop = SDL_SCANCODE_KP_8;
	controls[2].hold = SDL_SCANCODE_KP_7;

	/* gamepad */
	gp_enabled = 1;
	gp_lrot = 3;
	gp_rrot = 0;
	gp_hdrop = 2;
	gp_pause = 9;
	gp_hold = 1;

	/* multiplayer */
	mp_numholes = 1;
	mp_randholes = 0;

	/* cpu */
	cpu_style = 1; /* normal */
	cpu_delay = 700;
	cpu_sfactor = 100;

	/* controls */
	as_delay = 170;
	as_speed = 70;
	keystatefix = 0;

	/* sounds */
	sound = 1;
	volume = 50;
	audiobuffersize = 1024;
	channels = 16;

	/* graphics */
	animations = 1;
	windowmode = WM_FULLSCREEN;
	fps = 1;
	showfps = 0;
	smoothdrop = 1;

	/* various */
	themeid = 0;
	themecount = 1;

	/* directory and file name */
	dname = CONFIGDIR;
	if (dname[0] == '~')
		dname = getHomeDir() + "/" + dname.substr(1);
	fname = dname + "/ltris2.conf";
	if (!dirExists(dname)) {
		_loginfo("Config directory %s not found, creating it...\n",dname.c_str());
		makeDir(dname);
	}

	/* load */
	if (fileExists(fname)) {
		_loginfo("Loading configuration %s\n",fname.c_str());
		FileParser fp(fname);

		fp.get("gametype", gametype);
		fp.get("modern", modern);
		fp.get("startinglevel", startinglevel);

		for (int i = 0; i < 3; i++) {
			fp.get("player"+to_string(i+1)+".name",playernames[i]);
			fp.get("player"+to_string(i+1)+".lshift",controls[i].lshift);
			fp.get("player"+to_string(i+1)+".rshift",controls[i].rshift);
			fp.get("player"+to_string(i+1)+".lrot",controls[i].lrot);
			fp.get("player"+to_string(i+1)+".rrot",controls[i].rrot);
			fp.get("player"+to_string(i+1)+".sdrop",controls[i].sdrop);
			fp.get("player"+to_string(i+1)+".hdrop",controls[i].hdrop);
			fp.get("player"+to_string(i+1)+".hold",controls[i].hold);
		}

		fp.get("gp_enabled",gp_enabled);
		fp.get("gp_lrot",gp_lrot);
		fp.get("gp_rrot",gp_rrot);
		fp.get("gp_hdrop",gp_hdrop);
		fp.get("gp_pause",gp_pause);
		fp.get("gp_hold",gp_hold);

		fp.get("mp_numholes",mp_numholes);
		fp.get("mp_randholes",mp_randholes);

		fp.get("cpu_style",cpu_style);
		fp.get("cpu_delay",cpu_delay);
		fp.get("cpu_sfactor",cpu_sfactor);

		/* fix non 10% setting for cpu_sfactor since we changed the step */
		if ((cpu_sfactor % 10) != 0)
			cpu_sfactor = 100;

		fp.get("as_delay",as_delay);
		fp.get("as_speed",as_speed);
		fp.get("keystatefix",keystatefix);

		fp.get("sound", sound);
		fp.get("volume", volume);
		fp.get("audiobuffersize", audiobuffersize);
		fp.get("channels", channels);

		fp.get("animations", animations);
		fp.get("windowmode", windowmode);
		fp.get("fps", fps);
		fp.get("showfps", showfps);
		fp.get("smoothdrop", smoothdrop);

		fp.get("themeid", themeid);
		fp.get("themecount", themecount);
	}
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

	for (int i = 0; i < 3; i++) {
		ofs << "player" << to_string(i+1) << " {\n";
		ofs << "  name=" << playernames[i] << "\n";
		ofs << "  lshift=" << controls[i].lshift << "\n";
		ofs << "  rshift=" << controls[i].rshift << "\n";
		ofs << "  lrot=" << controls[i].lrot << "\n";
		ofs << "  rrot=" << controls[i].rrot << "\n";
		ofs << "  sdrop=" << controls[i].sdrop << "\n";
		ofs << "  hdrop=" << controls[i].hdrop << "\n";
		ofs << "  hold=" << controls[i].hold << "\n";
		ofs << "}\n";
	}

	ofs << "gp_enabled=" << gp_enabled << "\n";
	ofs << "gp_lrot=" << gp_lrot << "\n";
	ofs << "gp_rrot=" << gp_rrot << "\n";
	ofs << "gp_hdrop=" << gp_hdrop << "\n";
	ofs << "gp_pause=" << gp_pause << "\n";
	ofs << "gp_hold=" << gp_hold << "\n";

	ofs << "mp_numholes=" << mp_numholes << "\n";
	ofs << "mp_randholes=" << mp_randholes << "\n";

	ofs << "cpu_style=" << cpu_style << "\n";
	ofs << "cpu_delay=" << cpu_delay << "\n";
	ofs << "cpu_sfactor=" << cpu_sfactor << "\n";

	ofs << "as_delay=" << as_delay << "\n";
	ofs << "as_speed=" << as_speed << "\n";
	ofs << "keystatefix=" << keystatefix << "\n";

	ofs << "sound=" << sound << "\n";
	ofs << "volume=" << volume << "\n";
	ofs << "audiobuffersize=" << audiobuffersize << "\n";
	ofs << "channels=" << channels << "\n";

	ofs << "animations=" << animations << "\n";
	ofs << "windowmode=" << windowmode << "\n";
	ofs << "fps=" << fps << "\n";
	ofs << "showfps=" << showfps << "\n";
	ofs << "smoothdrop=" << smoothdrop << "\n";

	ofs << "themeid=" << themeid << "\n";
	ofs << "themecount=" << themecount << "\n";

	ofs.close();
	_loginfo("Configuration saved to %s\n",fname.c_str());
}


