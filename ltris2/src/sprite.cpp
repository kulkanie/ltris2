/*
 * sprite.cpp
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
#include "sprite.h"

Shrapnell::Shrapnell(Texture &t, Vector &p, Vector &v, Vector &g, uint a, double amod) :
	texture(t), w(t.getWidth()), h(t.getHeight()), pos(p), vel(v), grav(g)
{
	alpha.init(SCT_ONCE, a, SDL_ALPHA_TRANSPARENT, amod);
}
