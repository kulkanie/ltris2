/*
 * sdl.h
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

#ifndef SDL_H_
#define SDL_H_

#include <string>
#include <SDL_image.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include "tools.h"

class Texture;

class Renderer {
public:
	SDL_Window* mw;
	SDL_Renderer* mr;
	int w, h;

	Renderer() : mw(NULL), mr(NULL), w(0), h(0) {
		_loginfo("Initializing SDL\n");
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
				SDL_Log("SDL_Init failed: %s\n", SDL_GetError());
		if (TTF_Init() < 0)
		 		SDL_Log("TTF_Init failed: %s\n", SDL_GetError());
	}

	~Renderer() {
		destroy();

		_loginfo("Finalizing SDL\n");
		TTF_Quit();
		SDL_Quit();
	}

	int create(const string &title, int _w, int _h, int _full = 0);
	void destroy() {
		if (mr)
			SDL_DestroyRenderer(mr);
		if (mw)
			SDL_DestroyWindow(mw);
		mr = NULL;
		mw = NULL;
	}

	SDL_Renderer* get() {
		return mr;
	}
	int getWidth() {
		return w;
	}
	int getHeight() {
		return h;
	}

	void refresh() {
		SDL_RenderPresent(mr);
	}

	int rx2sx(double x) {
		return x * w;
	}
	int ry2sy(double y) {
		return y * h;
	}

	void setTarget(Texture &t);
	void clearTarget() {
		SDL_SetRenderTarget(mr, NULL);
	}

	int getDisplayId();
	bool isWidescreen() {
		return (w > 4*h/3);
	}
};

class Texture {
protected:
	SDL_Texture *tex;
	int w, h;
public:
	static int getHeight(const string& fname) {
		Texture img;
		if (!img.load(fname))
			return -1;
		return img.getHeight();
	}
	static int getWidth(const string& fname) {
		Texture img;
		if (!img.load(fname))
			return -1;
		return img.getWidth();
	}
	static Uint32 getSurfacePixel(const SDL_Surface *src, int sx, int sy) {
		Uint32 pixel = 0;
		memcpy( &pixel, (char*)src->pixels + sy * src->pitch +
				sx * src->format->BytesPerPixel,
				src->format->BytesPerPixel );
		return pixel;
	}
	static void setRenderScaleQuality(int level) {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,to_string(level).c_str());
	}

	Texture() : tex(NULL), w(0), h(0) {}
	Texture(int _w, int _h) : tex(NULL), w(0), h(0) {
		create(_w,_h);
	}
	~Texture() {
		if (tex)
			SDL_DestroyTexture(tex);
	}
	int create(int w, int h);
	int createFromScreen();
	int load(const string& fname);
	int load(SDL_Surface *s);
	int load(Texture *s, int x, int y, int w, int h);
	int duplicate(Texture &s) {
		return load(&s,0,0,s.getWidth(),s.getHeight());
	}
	SDL_Texture *get();
	void copy();
	void copy(int dx, int dy);
	void copy(int dx, int dy, int dw, int dh);
	void copy(int sx, int sy, int sw, int sh, int dx, int dy);
	void copy(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh);
	int getWidth() {return w;}
	int getHeight() {return h;}
	void setAlpha(int alpha) { SDL_SetTextureAlphaMod(tex, alpha); }
	void clearAlpha() { setAlpha(255); }
	void setBlendMode(int on) {
		SDL_SetTextureBlendMode(tex,
				on?SDL_BLENDMODE_BLEND:SDL_BLENDMODE_NONE);
	}
	void fill(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
		fill(0,0,w,h,r,g,b,a);
	}
	void fill(const SDL_Color &c) {
		fill(0,0,w,h,c.r,c.g,c.b,c.a);
	}
	void fill(SDL_Rect &r, SDL_Color &c) {
		fill(r.x,r.y,r.w,r.h,c.r,c.g,c.b,c.a);
	}
	void fill(int x, int y, int w, int h,
			Uint8 r, Uint8 g, Uint8 b, Uint8 a);
	void scale(int nw, int nh);
	int createShadow(Texture &img);
};

#ifndef ALIGN_X_LEFT
	#define ALIGN_X_LEFT	1
	#define ALIGN_X_CENTER	2
	#define ALIGN_X_RIGHT	4
	#define ALIGN_Y_TOP	8
	#define ALIGN_Y_CENTER	16
	#define ALIGN_Y_BOTTOM	32
#endif

class Font {
protected:
	TTF_Font *font;
	SDL_Color clr;
	int align;
	int size;
public:
	Font() : font(0), align(0), size(0) {}
	~Font() {
		if (font)
			TTF_CloseFont(font);
	}
	void free() {
		if (font) {
			TTF_CloseFont(font);
			font = 0;
		}
	}
	void load(const string& fname, int size);
	void setColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
	void setColor(SDL_Color c);
	void setAlign(int a);
	int getSize() {return size;};
	int getLineHeight() {
		if (font)
			return TTF_FontLineSkip(font);
		return 0;
	}
	int getTextSize(const string& str, int *w, int *h) {
		*w = 0;
		*h = 0;
		if (font)
			return TTF_SizeText(font,str.c_str(),w,h);
		return 0;
	};
	int getWrappedTextSize(const string& str, uint maxw, int *w, int *h) {
		*w = *h = 0;
		SDL_Surface *surf = TTF_RenderUTF8_Blended_Wrapped(
						font, str.c_str(), clr, maxw);
		if (surf) { /* any way to do this without rendering? */
			*w = surf->w;
			*h = surf->h;
			SDL_FreeSurface(surf);
			return 1;
		}
		return 0;
	}
	int getCharWidth(char c) {
		if (!font)
			return 0;
		int w, h;
		char str[2] = {c, 0};
		TTF_SizeText(font,str,&w,&h);
		return w;
	}
	void write(int x, int y, const string &str, int alpha = 255);
	void writeText(int x, int y, const string &text, int width, int alpha = 255);
};

/** easy log sdl error */
#define _logsdlerr() \
	fprintf(stderr,"ERROR: %s:%d: %s(): %s\n", \
			__FILE__, __LINE__, __FUNCTION__, SDL_GetError())

class Ticks {
	Uint32 now, last;
public:
	Ticks() { now = last = SDL_GetTicks(); }
	void reset() { now = last = SDL_GetTicks(); }
	Uint32 get(bool zeroOK = false) {
		now = SDL_GetTicks();
		Uint32 ms = now - last;
		if (ms == 0 && !zeroOK)
			ms = 1;
		last = now;
		return ms;
	}
};

class Label {
	bool empty;
	Texture img;
	int border; /* -1 for default 20% font size border */
	SDL_Color bgColor;
public:
	Label(bool transparent=false) : empty(true), border(-1) {
		bgColor.r = bgColor.g = bgColor.b = 0;
		if (transparent) {
			bgColor.a = 0;
			border = 0;
		} else
			bgColor.a = 224;
	}
	void setBgColor(const SDL_Color &c) {
		bgColor = c;
	}
	void setBorder(int b) {
		if (b < 0)
			b = -1;
		border = b;
	}
	void setText(Font &f, const string &str, uint max = 0);
	void clearText() {
		empty = true;
	}
	void copy(int x, int y, int align = ALIGN_X_CENTER | ALIGN_Y_CENTER) {
		if (empty)
			return;
		if (align & ALIGN_X_CENTER)
			x -= img.getWidth() / 2;
		else if (align & ALIGN_X_RIGHT)
			x -= img.getWidth();
		if (align & ALIGN_Y_CENTER)
			y -= img.getHeight() / 2;
		else if (align & ALIGN_Y_BOTTOM)
			y -= img.getHeight();
		img.copy(x,y);
	}
	void setAlpha(int a) {
		img.setAlpha(a);
	}
	uint getHeight() {
		if (empty)
			return 0;
		return img.getHeight();
	}
	uint getWidth() {
		if (empty)
			return 0;
		return img.getWidth();
	}
};

/** Wrapper for SDL_Joystick. Yes, it should be gamecontroller
 * but I cannot open a valid gamepad as controller but as joystick
 * so we go with what works. */
enum {
	/* gamepad inputs */
	GPAD_NONE = 0,
	GPAD_LEFT,
	GPAD_RIGHT,
	GPAD_UP,
	GPAD_DOWN,
	GPAD_BUTTON0,
	GPAD_BUTTON1,
	GPAD_BUTTON2,
	GPAD_BUTTON3,
	GPAD_BUTTON4,
	GPAD_BUTTON5,
	GPAD_BUTTON6,
	GPAD_BUTTON7,
	GPAD_BUTTON8,
	GPAD_BUTTON9,
	GPAD_LAST1,

	/* gamepad button states */
	GPBS_RELEASED = 0,
	GPBS_PRESSED,
	GPBS_DOWN,
	GPBS_UP
};
class Gamepad {
	SDL_Joystick *js;
	uint numbuttons;
	Uint8 state[GPAD_LAST1];
	Uint8 oldstate[GPAD_LAST1];

	void updateInput(uint id, bool active);
public:
	Gamepad() : js(NULL), numbuttons(0) {}
	~Gamepad() {
		close();
	}
	void open();
	int opened() {
		return (js != NULL);
	}
	void close() {
		if (js) {
			SDL_JoystickClose(js);
			_loginfo("Closed game controller 0\n");
			js = NULL;
		}
	}
	const Uint8 *update();
};

/** Similiar to gamepad we need an object properly keystates.
 * Using SDL_KEYDOWN for (first) DOWN event doesn't work as
 * it gets auto repeated... */
enum {
	KS_RELEASED = 0, /* not pressed */
	KS_PRESSED, /* continously pressed */
	KS_DOWN, /* freshly pressed in this cycle */
	KS_UP /* just released in this cycle */

};
class Keystate {
	Uint8 oldstate[SDL_NUM_SCANCODES];
	Uint8 curstate[SDL_NUM_SCANCODES];
public:
	Keystate() {
		reset();
	}
	void reset() {
		memset(oldstate,KS_RELEASED,sizeof(oldstate));
		memset(curstate,KS_RELEASED,sizeof(curstate));
	}
	const Uint8 *update();
	void forceKeyDown(int scancode) {
		if (scancode < 0 || scancode >= SDL_NUM_SCANCODES)
			return;
		if (curstate[scancode] == KS_RELEASED || curstate[scancode] == KS_UP) {
			oldstate[scancode] = KS_RELEASED;
			curstate[scancode] = KS_DOWN;
			_loginfo("Oops, need to force key down: %d\n",scancode);
		}
	}
};

#endif /* SDL_H_ */
