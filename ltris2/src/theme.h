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
class Menu;

enum {
	MAXWALLPAPERS = 10
};

class Theme {
	friend View;
	friend Menu;

	string stdPath; /* path to standard theme for fallbacks */

	Texture menuBackground;
	uint menuX, menuY, menuItemWidth, menuItemHeight;
	Font fMenuNormal, fMenuFocus;

	Texture wallpapers[MAXWALLPAPERS];
	uint numWallpapers;
	Font fSmall, fNormal, fNormalHighlighted;

	Sound sMenuClick, sMenuMotion;

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
			numWallpapers(0)
	{
		stdPath = string(DATADIR) + "/themes/Standard";
	}
	void load(string name, Renderer &r);
};

#endif /* SRC_THEME_H_ */