/*
 * sdl.cpp
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

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include "sdl.h"

SDL_Renderer *mrc = NULL; /* main window render context, got only one */

/** Main application window */

int Renderer::create(const string &title, int _w, int _h, int _full)
{
	int ret = 1;

	/* destroy old contexts if any */
	destroy();

	if (_w <= 0 || _h <= 0) { /* no width or height, use desktop setting */
		int idx = getDisplayId();
		SDL_DisplayMode mode;
		SDL_GetCurrentDisplayMode(idx,&mode);
		_w = mode.w;
		_h = mode.h;
		_full = 1;
	}
	w = _w;
	h = _h;

	_loginfo("Creating window with size %dx%d (%s)\n",w,h,_full?"fullscreen":"window");
 	if (_full) {
		mw = SDL_CreateWindow(title.c_str(), 0,0,0,0,0);
		if (mw)
			SDL_SetWindowFullscreen(mw, SDL_WINDOW_FULLSCREEN_DESKTOP);
	} else
		mw = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
					SDL_WINDOWPOS_CENTERED, w, h, 0);
	if(mw == NULL) {
		_logsdlerr();
		ret = 0;
	}
	if ((mr = SDL_CreateRenderer(mw, -1, SDL_RENDERER_ACCELERATED)) == NULL) {
		_logsdlerr();
		ret = 0;
	}
	mrc = mr;

	/* back to black */
	SDL_SetRenderDrawColor(mr,0,0,0,SDL_ALPHA_OPAQUE);
	SDL_RenderClear(mr);

	return ret;
}

void Renderer::setTarget(Texture &t) {
	SDL_SetRenderTarget(mr, t.get());
}

/** Return index of display current window is running on or will be created
 * on. Default to 0 on errors.
 */
int Renderer::getDisplayId()
{
	int idx = 0; /* default to display 0 */

	/* get current display index or default to 0 */
	if (mw) {
		idx = SDL_GetWindowDisplayIndex(mw);
	} else if (SDL_GetNumVideoDisplays() > 1) {
		/* XXX first time we open the window, so to properly determine the
		 * current display we open a small window for multiple monitors */
		SDL_Window *testwindow;
		_loginfo("Multiple displays detected... ");
		if((testwindow = SDL_CreateWindow("LBreakoutHD",
					SDL_WINDOWPOS_CENTERED,
					SDL_WINDOWPOS_CENTERED,
					320, 200, 0)) == NULL) {
			_logerr("\n  Could not open display test window: %s\n",
								SDL_GetError());
			_loginfo("\n  Falling back to settings of display 0\n");
		} else {
			idx = SDL_GetWindowDisplayIndex(testwindow);
			SDL_DestroyWindow(testwindow);
			_loginfo("using display %d\n",idx);
		}
	}
	return idx;
}


/** Image */

int Texture::create(int w, int h)
{
	if (tex) {
		SDL_DestroyTexture(tex);
		tex = NULL;
	}

	this->w = w;
	this->h = h;
	if ((tex = SDL_CreateTexture(mrc,SDL_PIXELFORMAT_RGBA8888,
				SDL_TEXTUREACCESS_TARGET,w, h)) == NULL) {
		_logsdlerr();
		return 0;
	}
	fill(0,0,0,0);
	setBlendMode(1);
	return 1;
}

int Texture::createFromScreen()
{
	int w, h;

	SDL_GetRendererOutputSize(mrc,&w,&h);
	SDL_Surface *sshot = SDL_CreateRGBSurface(0, w, h, 32,
			0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
	SDL_RenderReadPixels(mrc, NULL, SDL_PIXELFORMAT_ARGB8888,
					sshot->pixels, sshot->pitch);
	int ret = load(sshot);
	SDL_FreeSurface(sshot);
	return ret;
}

int Texture::load(const string& fname)
{
	if (tex) {
		SDL_DestroyTexture(tex);
		tex = NULL;
	}

	SDL_Surface *surf = IMG_Load(fname.c_str());
	if (surf == NULL) {
		_logsdlerr();
		return 0;
	}
	if ((tex = SDL_CreateTextureFromSurface(mrc, surf)) == NULL) {
		_logsdlerr();
		return 0;
	}
	w = surf->w;
	h = surf->h;
	SDL_FreeSurface(surf);
	return 1;
}
int Texture::load(SDL_Surface *s)
{
	if (tex) {
		SDL_DestroyTexture(tex);
		tex = NULL;
	}

	if ((tex = SDL_CreateTextureFromSurface(mrc,s)) == NULL) {
		_logsdlerr();
		return 0;
	}
	w = s->w;
	h = s->h;
	return 1;
}
int Texture::load(Texture *s, int x, int y, int w, int h)
{
	if (tex) {
		SDL_DestroyTexture(tex);
		tex = NULL;
	}

	SDL_Rect srect = {x, y, w, h};
	SDL_Rect drect = {0, 0, w, h};

	this->w = w;
	this->h = h;
	if ((tex = SDL_CreateTexture(mrc,SDL_PIXELFORMAT_RGBA8888,
				SDL_TEXTUREACCESS_TARGET,w, h)) == NULL) {
		_logsdlerr();
		return 0;
	}
	SDL_SetRenderTarget(mrc, tex);
	SDL_RenderCopy(mrc, s->get(), &srect, &drect);
	SDL_SetRenderTarget(mrc, NULL);
	return 1;
}

SDL_Texture *Texture::get()
{
	return tex;
}

void Texture::copy() /* full scale */
{
	if (tex == NULL)
		return;

	SDL_RenderCopy(mrc, tex, NULL, NULL);
}
void Texture::copy(int dx, int dy)
{
	if (tex == NULL)
		return;

	SDL_Rect drect = {dx , dy , w, h};
	SDL_RenderCopy(mrc, tex, NULL, &drect);
}
void Texture::copy(int dx, int dy, int dw, int dh)
{
	if (tex == NULL)
		return;

	SDL_Rect drect = {dx , dy , dw, dh};
	SDL_RenderCopy(mrc, tex, NULL, &drect);
}
void Texture::copy(int sx, int sy, int sw, int sh, int dx, int dy) {
	if (tex == NULL)
		return;

	SDL_Rect srect = {sx, sy, sw, sh};
	SDL_Rect drect = {dx , dy , sw, sh};
	SDL_RenderCopy(mrc, tex, &srect, &drect);
}

void Texture::copy(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh) {
	if (tex == NULL)
		return;

	SDL_Rect srect = {sx, sy, sw, sh};
	SDL_Rect drect = {dx , dy , dw, dh};
	SDL_RenderCopy(mrc, tex, &srect, &drect);
}

void Texture::fill(int x, int y, int w, int h,
		Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	SDL_Rect rect = {x,y,w,h};
	SDL_Texture *old = SDL_GetRenderTarget(mrc);
	SDL_SetRenderTarget(mrc,tex);
	SDL_SetRenderDrawColor(mrc,r,g,b,a);
	SDL_RenderFillRect(mrc,&rect);
	SDL_SetRenderTarget(mrc,old);
}

void Texture::scale(int nw, int nh)
{
	if (tex == NULL)
		return;
	if (nw == w && nh == h)
		return; /* already ok */

	SDL_Texture *newtex = 0;
	if ((newtex = SDL_CreateTexture(mrc,SDL_PIXELFORMAT_RGBA8888,
					SDL_TEXTUREACCESS_TARGET,nw,nh)) == NULL) {
		_logsdlerr();
		return;
	}
	SDL_Texture *oldTarget = SDL_GetRenderTarget(mrc);
	SDL_SetRenderTarget(mrc, newtex);
	SDL_SetTextureBlendMode(newtex, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(mrc,0,0,0,0);
	SDL_RenderClear(mrc);
	SDL_RenderCopy(mrc, tex, NULL, NULL);
	SDL_SetRenderTarget(mrc, oldTarget);

	SDL_DestroyTexture(tex);
	tex = newtex;
	w = nw;
	h = nh;
}

/** Create shadow image by clearing all r,g,b to 0 and cutting alpha in half. */
int Texture::createShadow(Texture &img)
{
	/* duplicate first */
	create(img.getWidth(),img.getHeight());
	SDL_Texture *oldTarget = SDL_GetRenderTarget(mrc);
	SDL_SetRenderTarget(mrc, tex);
	img.copy(0,0);

	/* change and apply */
	Uint32 pixels[w*h];
	int pitch = w*sizeof(Uint32);
	Uint8 r,g,b,a;
	Uint32 pft = 0;
	SDL_QueryTexture(tex, &pft, 0, 0, 0);
	SDL_PixelFormat* pf = SDL_AllocFormat(pft);
	if (SDL_RenderReadPixels(mrc, NULL, pft, pixels, pitch) < 0) {
		_logsdlerr();
		return 0;
	}
	for (int i = 0; i < w * h; i++) {
		SDL_GetRGBA(pixels[i],pf,&r,&g,&b,&a);
		pixels[i] = SDL_MapRGBA(pf,0,0,0,a/2);
	}
	SDL_UpdateTexture(tex, NULL, pixels, pitch);
	SDL_FreeFormat(pf);
	SDL_SetRenderTarget(mrc, oldTarget);

	return 1;
}


/** Font */

void Font::load(const string& fname, int sz) {
	if (font) {
		TTF_CloseFont(font);
		font = 0;
		size = 0;
	}

	if ((font = TTF_OpenFont(fname.c_str(), sz)) == NULL) {
		_logsdlerr();
		return;
	}
	size = sz;
	setColor(255,255,255,255);
	setAlign(ALIGN_X_LEFT | ALIGN_Y_TOP);
}

void Font::setColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	SDL_Color c = {r, g, b, a};
	clr = c;
}
void Font::setColor(SDL_Color c) {
	clr = c;
}
void Font::setAlign(int a) {
	align = a;
}
void Font::write(int x, int y, const string& _str, int alpha) {
	if (font == 0)
		return;

	/* XXX doesn't look good, why no rendering
	 * into texture directly available? */
	SDL_Surface *surf;
	SDL_Texture *tex;
	SDL_Rect drect;
	const char *str = _str.c_str();

	if (strlen(str) == 0)
		return;

	if ((surf = TTF_RenderUTF8_Blended(font, str, clr)) == NULL)
		_logsdlerr();
	if ((tex = SDL_CreateTextureFromSurface(mrc, surf)) == NULL)
		_logsdlerr();
	if (align & ALIGN_X_LEFT)
		drect.x = x;
	else if (align & ALIGN_X_RIGHT)
		drect.x = x - surf->w;
	else
		drect.x = x - surf->w/2; /* center */
	if (align & ALIGN_Y_TOP)
		drect.y = y;
	else if (align & ALIGN_Y_BOTTOM)
		drect.y = y - surf->h;
	else
		drect.y = y - surf->h/2;
	drect.w = surf->w;
	drect.h = surf->h;
	if (alpha < 255)
		SDL_SetTextureAlphaMod(tex, alpha);

	/* do a shadow first */
	if (1) {
		SDL_Rect drect2 = drect;
		SDL_Surface *surf2;
		SDL_Texture *tex2;
		SDL_Color clr2 = { 0,0,0,255 };
		if ((surf2 = TTF_RenderUTF8_Blended(font, str, clr2)) == NULL)
			_logsdlerr();
		if ((tex2 = SDL_CreateTextureFromSurface(mrc, surf2)) == NULL)
			_logsdlerr();
		drect2.x += size/10;
		drect2.y += size/10;
		SDL_SetTextureAlphaMod(tex2, alpha/2);
		SDL_RenderCopy(mrc, tex2, NULL, &drect2);
		SDL_FreeSurface(surf2);
		SDL_DestroyTexture(tex2);
	}

	SDL_RenderCopy(mrc, tex, NULL, &drect);
	SDL_FreeSurface(surf);
	SDL_DestroyTexture(tex);
}
void Font::writeText(int x, int y, const string& _text, int wrapwidth, int alpha)
{
	if (font == 0)
		return;

	/* XXX doesn't look good, why no rendering
	 * into texture directly available? */
	SDL_Surface *surf;
	SDL_Texture *tex;
	SDL_Rect drect;
	const char *text = _text.c_str();

	if (strlen(text) == 0)
		return;

	if ((surf = TTF_RenderUTF8_Blended_Wrapped(font, text, clr, wrapwidth)) == NULL)
		_logsdlerr();
	if ((tex = SDL_CreateTextureFromSurface(mrc, surf)) == NULL)
		_logsdlerr();
	drect.x = x;
	drect.y = y;
	drect.w = surf->w;
	drect.h = surf->h;
	if (alpha < 255)
		SDL_SetTextureAlphaMod(tex, alpha);
	SDL_RenderCopy(mrc, tex, NULL, &drect);
	SDL_FreeSurface(surf);
	SDL_DestroyTexture(tex);
}

void Label::setText(Font &font, const string &str, uint maxw)
{
	if (str == "") {
		empty = true;
		return;
	}

	int b = border;
	if (b == -1)
		b = 20 * font.getSize() / 100;
	int w = 0, h = 0;

	/* get size */
	if (maxw == 0) /* single centered line */
		font.getTextSize(str, &w, &h);
	else
		font.getWrappedTextSize(str, maxw, &w, &h);

	if (w == 0 || h == 0)
		return;

	img.create(w + 4*b, h + 2*b);
	img.fill(bgColor);
	SDL_Texture *old = SDL_GetRenderTarget(mrc);
	SDL_SetRenderTarget(mrc,img.get());
	if (maxw == 0) {
		font.setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
		font.write(img.getWidth()/2,img.getHeight()/2,str);
	} else
		font.writeText(2*b, b, str, maxw);
	SDL_SetRenderTarget(mrc,old);
	empty = false;
}

/** Try to find and open a joystick (game controller) */
void Gamepad::open()
{
	if (js)
		close(); /* make sure none is opened yet */

	if (SDL_NumJoysticks() == 0) {
		_loginfo("No game controller found...\n");
		return;
	}

	if ((js = SDL_JoystickOpen(0)) == NULL) {
		_logerr("Couldn't open game controller: %s\n",SDL_GetError());
		return;
	}

	_loginfo("Opened game controller 0\n");
	_logdebug(1,"  num axes: %d, num buttons: %d, num balls: %d\n",
			SDL_JoystickNumAxes(js),SDL_JoystickNumButtons(js),
			SDL_JoystickNumBalls(js));

	numbuttons = SDL_JoystickNumButtons(js);
	if (numbuttons > 10)
		numbuttons = 10;
}

/** Update a single input state (@id). @active is the current SDL state. */
void Gamepad::updateInput(uint id, bool active)
{
	if (id >= GPAD_LAST1)
		return;

	if (active) {
		if (oldstate[id]==GPBS_DOWN || oldstate[id]==GPBS_PRESSED)
			state[id] = GPBS_PRESSED;
		else
			state[id] = GPBS_DOWN;
	} else {
		if (oldstate[id]==GPBS_DOWN || oldstate[id]==GPBS_PRESSED)
			state[id] = GPBS_UP;
	}
}

/** Update joystick state and return state array.
 * (0=released,1=permanently pressed,2=down,3=up) */
const Uint8 *Gamepad::update() {
	SDL_JoystickUpdate();

	memcpy(oldstate,state,sizeof(state));
	memset(state,GPBS_RELEASED,sizeof(state));

	if (js == NULL)
		return state;

	if (SDL_JoystickGetAxis(js,0) < -3200)
		updateInput(GPAD_LEFT, true);
	else
		updateInput(GPAD_LEFT, false);
	if (SDL_JoystickGetAxis(js,0) > 3200)
		updateInput(GPAD_RIGHT, true);
	else
		updateInput(GPAD_RIGHT, false);
	if (SDL_JoystickGetAxis(js,1) < -3200)
		updateInput(GPAD_UP, true);
	else
		updateInput(GPAD_UP, false);
	if (SDL_JoystickGetAxis(js,1) > 3200)
		updateInput(GPAD_DOWN, true);
	else
		updateInput(GPAD_DOWN, false);

	for (uint i = 0; i < numbuttons; i++)
		updateInput(GPAD_BUTTON0 + i, SDL_JoystickGetButton(js,i));

	return state;
}

/** Update key states and return state array.
 * (0=released,1=permanently pressed,2=down,3=up) */
const Uint8 *Keystate::update()
{
	int numkeys;
	const Uint8 *ks = SDL_GetKeyboardState(&numkeys);

	if (numkeys > SDL_NUM_SCANCODES)
		numkeys = SDL_NUM_SCANCODES; /* should not happen, for safety */

	memcpy(oldstate,curstate,sizeof(curstate));
	memset(curstate,GPBS_RELEASED,sizeof(curstate));

	for (int id = 0; id < numkeys; id++) {
		if (ks[id]) {
			if (oldstate[id]==KS_DOWN || oldstate[id]==KS_PRESSED)
				curstate[id] = KS_PRESSED;
			else
				curstate[id] = GPBS_DOWN;

		} else {
			if (oldstate[id]==KS_DOWN || oldstate[id]==KS_PRESSED)
				curstate[id] = KS_UP;
		}
	}

	return curstate;
}
