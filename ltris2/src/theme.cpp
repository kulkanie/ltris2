/*
 * theme.cpp
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

/** Load resources and scale if necessary using bricks screen height.
 * Whatever is missing: Fall back to Standard theme. */
void Theme::load(string name, Renderer &r)
{
	string path, fpath;
	vector<string> fnames;

	path = string(DATADIR) + "/themes/" + name;

	if (!fileExists(path))
		_logerr("CRITICAL ERROR: theme %s not found. We continue but most likely it will crash...\n",path.c_str());
	_loginfo("Loading theme %s\n",path.c_str());

	/* load theme.ini */
	if (fileExists(path + "/theme.ini")) {
		FileParser fp(path + "/theme.ini");
		fp.get("numWallPapers",numWallpapers);
	}

	Texture::setRenderScaleQuality(1);

	readDir(path, RD_FILES, fnames);

	/* fonts */
	fSmall.load(testRc(path,"f_normal.otf"), r.ry2sy(0.028));
	fNormal.load(testRc(path,"f_bold.otf"), r.ry2sy(0.037));
	fNormalHighlighted.load(testRc(path,"f_bold.otf"), r.ry2sy(0.037));
	fNormalHighlighted.setColor(255,220,0,255);

	/* menu */
	menuX = r.rx2sx(0.16);
	menuY = r.ry2sy(0.63);
	menuItemWidth = r.rx2sx(0.22);
	menuItemHeight = r.ry2sy(0.037);
	menuBackground.load(testRc(path,"menu.png"));
	fMenuNormal.load(testRc(path,"f_normal.otf"), r.ry2sy(0.033));
	fMenuNormal.setColor({255,255,255,255});
	fMenuFocus.load(testRc(path,"f_bold.otf"), r.ry2sy(0.037));
	fMenuFocus.setColor({255,220,0,255});

	/* sounds */
	sMenuClick.load(testRc(path,"s_menuclick.wav"));
	sMenuMotion.load(testRc(path,"s_menumotion.wav"));

	/* wallpapers */
	for (uint i = 0; i < numWallpapers; i++) {
		wallpapers[i].load(path + "/wallpaper" + to_string(i+1) + ".jpg");
		wallpapers[i].setBlendMode(0);
	}
	if (numWallpapers == 0) {
		wallpapers[0].load(stdPath + "/wallpaper1.jpg");
		wallpapers[0].setBlendMode(0);
		numWallpapers = 1;
	}

	/* blocks */
	blocks.load(testRc(path,"blocks.png"));
}
