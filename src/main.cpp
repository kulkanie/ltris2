/*
 * main.cpp
 * (C) 2024 by Michael Speck
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

using namespace std;

#include "sdl.h"
#include "tools.h"
#include "mixer.h"
#include "theme.h"
#include "sprite.h"
#include "menu.h"
#include "vconfig.h"
#include "hiscores.h"
#include "../libgame/bowl.h"
#include "vbowl.h"
#include "vgame.h"
#include "view.h"

/* global resources for easier access across all objects */
Renderer renderer;
VConfig vconfig;
Theme theme;

int main(int argc, char **argv)
{
	/* i18n */
#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	printf("---\n");
	printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
	printf("Copyright 2024 Michael Speck\n");
	printf("Published under GNU GPL\n");
	printf("---\n");

	srand(time(NULL));

	View view;
	view.run();
	return 0;
}
