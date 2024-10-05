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

extern Renderer renderer;

/** Load resources and scale if necessary using bricks screen height.
 * Whatever is missing: Fall back to Standard theme. */
void Theme::load(string name, Renderer &r)
{
	string fpath;
	vector<string> fnames;

	path = string(DATADIR) + "/themes/" + name;

	if (!fileExists(path))
		_logerr("CRITICAL ERROR: theme %s not found. We continue but most likely it will crash...\n",path.c_str());
	_loginfo("Loading theme %s\n",path.c_str());

	/* load theme.ini */
	if (fileExists(path + "/theme.ini")) {
		FileParser fp(path + "/theme.ini");
		fp.get("numWallPapers",numWallpapers);
		fp.get("blockSize",blockSize);
	}

	Texture::setRenderScaleQuality(1);

	readDir(path, RD_FILES, fnames);

	/* menu */
	fontColorNormal = {255,255,255,255};
	fontColorHighlight = {255,220,0,255};
	menuX = r.rx2sx(0.16);
	menuY = r.ry2sy(0.63);
	menuItemWidth = r.rx2sx(0.22);
	menuItemHeight = r.ry2sy(0.037);
	menuBackground.load(testRc(path,"menu.png"));
	fMenuNormal.load(testRc(path,"f_normal.otf"), r.ry2sy(0.033));
	fMenuNormal.setColor(fontColorNormal);
	fMenuFocus.load(testRc(path,"f_bold.otf"), r.ry2sy(0.037));
	fMenuFocus.setColor(fontColorHighlight);
	fMenuCaption.load(testRc(path,"f_bold.otf"), r.ry2sy(0.037));
	fMenuCaption.setColor(fontColorNormal);
	fTooltip.load(testRc(path,"f_normal.otf"), r.ry2sy(0.028));
	sMenuClick.load(testRc(path,"s_menuclick.wav"));
	sMenuMotion.load(testRc(path,"s_menumotion.wav"));

	/* cursor - scale to 0.036 of screen height */
	cursor.load(testRc(path,"cursor.png"));
	uint ch = r.ry2sy(0.036);
	uint cw = cursor.getWidth() * ch / cursor.getHeight();
	cursor.scale(cw,ch);

	/* wallpapers */
	numWallpapers = 0;
	string wfname = path + "/wallpaper0.jpg";
	while (fileExists(wfname)) {
		wallpapers[numWallpapers].load(wfname);
		wallpapers[numWallpapers].setBlendMode(0);
		numWallpapers++;
		if (numWallpapers == MAXWALLPAPERS)
			break;
		wfname = path + "/wallpaper" + to_string(numWallpapers) + ".jpg";
	}
	if (numWallpapers == 0) {
		wallpapers[0].load(stdPath + "/wallpaper0.jpg");
		wallpapers[0].setBlendMode(0);
		numWallpapers = 1;
	}

	/* blocks -- each valid set is in a row */
	blocks.load(testRc(path,"blocks.png"));
	numBlocks = blocks.getWidth() / blockSize;
	if (numBlocks > NUMBLOCKS)
		numBlocks = NUMBLOCKS;
	if (numBlocks < NUMBLOCKS)
		_loginfo("Theme provides only %d blocks instead of %d...\n",numBlocks,NUMBLOCKS);

	/* ingame sounds */
	sShift.load(testRc(path,"s_shift.wav"));
	sInsert.load(testRc(path,"s_insert.wav"));
	sExplosion.load(testRc(path,"s_explosion.wav"));
	sNextLevel.load(testRc(path,"s_nextlevel.wav"));
	sTetris.load(testRc(path,"s_tetris.wav"));
}

/** Load fonts for bowl assets. @bsize is single block size. */
void Theme::vbaLoadFonts(uint bsize)
{
	vbaFontNormal.load(testRc(path,"f_normal.otf"), 9*bsize/10);
	vbaFontNormal.setColor(fontColorNormal);
	vbaFontBold.load(testRc(path,"f_bold.otf"), 9*bsize/10);
	vbaFontBold.setColor(fontColorNormal);
	vbaFontSmall.load(testRc(path,"f_normal.otf"), 7*bsize/10);
	vbaFontSmall.setColor(fontColorNormal);
	vbaFontTiny.load(testRc(path,"f_normal.otf"), 6*bsize/10);
	vbaFontTiny.setColor(fontColorNormal);
}
