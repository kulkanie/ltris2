/*
 * theme.h
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

#ifndef SRC_THEME_H_
#define SRC_THEME_H_

class View;
class VGame;
class VBowl;
class Menu;

enum {
	MAXWALLPAPERS = 10,
	NUMBLOCKS = 12,
	NUMPIECES = 7,
	LOCKDELAYBLOCKID = 10
};

class Theme {
	friend View;
	friend VGame;
	friend VBowl;
	friend Menu;

	string path; /* resource path for this theme */
	string stdPath; /* path to standard theme for fallbacks */

	Texture menuBackground;
	uint menuX, menuY, menuItemWidth, menuItemHeight;
	Font fMenuNormal, fMenuFocus, fMenuCaption, fTooltip;
	Sound sMenuClick, sMenuMotion;
	SDL_Color fontColorNormal;
	SDL_Color fontColorHighlight;
	Texture cursor;

	Texture wallpapers[MAXWALLPAPERS];
	uint numWallpapers;
	Texture blocks; /* for pieces */
	uint numBlocks;
	uint blockSize;
	Sound sShift, sInsert, sExplosion, sNextLevel, sTetris;

	/* XXX vbowl assets, set by vgame.init(). having a vbowl assets
	 * class does not work. for some reason textures are empty and
	 * I cannot figure out why. instructions are exactly the same...
	 * so we just do it this way. */
	Texture vbaBlocks[NUMBLOCKS];
	Texture vbaPreviews[NUMPIECES];
	Font vbaFontNormal, vbaFontBold, vbaFontSmall, vbaFontTiny;

	const string &testRc(const string &path, const string &fname) {
		static string fpath; /* not thread safe */
		if (fileExists(path + "/" + fname))
			fpath = path + "/" + fname;
		else
			fpath = stdPath + "/" + fname;
		return fpath;
	}
public:
	Theme() : menuX(0), menuY(0), menuItemWidth(0), menuItemHeight(0),
			numWallpapers(0), numBlocks(0), blockSize(0)
	{
		stdPath = string(DATADIR) + "/themes/Standard";
	}
	void load(string name, Renderer &r);
	void vbaLoadFonts(uint bsize);
};

#endif /* SRC_THEME_H_ */
