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
#include "menu.h"
#include "vconfig.h"
#include "vcharts.h"
#include "../libgame/bowl.h"
#include "vgame.h"
#include "view.h"

int main(int argc, char **argv)
{
	/* i18n */
#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
	printf("Copyright 2024 Michael Speck\n");
	printf("Published under GNU GPL\n");
	printf("---\n");

	srand(time(NULL));

	Renderer renderer;
	View view(renderer);
	view.run();
	return 0;
}
