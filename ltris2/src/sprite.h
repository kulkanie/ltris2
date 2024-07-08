/*
 * sprite.h
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

#ifndef SRC_SPRITE_H_
#define SRC_SPRITE_H_

/* Graphical object (hail to the C64 :) that can be rendered and updated. */
class Sprite {
public:
	virtual ~Sprite() {};
	virtual int update(uint ms) = 0; /* return 1 if to be removed, 0 otherwise */
	virtual void render() = 0;
};

/* A fading tile with a position and movement vector influenced by gravity. */
class Shrapnell : public Sprite {
	Texture &texture; /* reference to an existing texture */
	int w, h; /* size of texture */
	Vector pos, vel, grav; /* position, velocity and vel change (gravity) */
	SmoothCounter alpha;
public:
	Shrapnell(Texture &t, Vector &p, Vector &v, Vector &g, uint a, double amod);
	int update(uint ms) {
		pos.add(ms, vel);
		vel.add(ms, grav);
		return alpha.update(ms);
	}
	void render() {
		texture.setAlpha(alpha.get());
		texture.copy(pos.getX(), pos.getY(), w, h);
		texture.clearAlpha();
	}
};

#endif /* SRC_SPRITE_H_ */
