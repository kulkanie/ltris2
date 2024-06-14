/*
 * tools.h
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

#ifndef SRC_TOOLS_H_
#define SRC_TOOLS_H_

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
  #include <time.h>
  #include <stdint.h>
  typedef uint32_t uint;
#endif
#include <math.h>
#include <dirent.h>
#include <list>
#include <string>
#include <vector>
#include <memory>
#include <cstdio>
#include <random>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "vconfig.h"

/* i18n */
#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#include "../gettext.h"
#if ENABLE_NLS
#define _(str) gettext (str)
#else
#define _(str) (str)
#endif

#ifndef DEBUGLEVEL
  #define DEBUGLEVEL 0
#endif

#define _logerr(...) do { \
		fprintf(stderr,"ERROR: %s:%d: %s(): ", __FILE__, __LINE__, __FUNCTION__); \
		fprintf (stderr, __VA_ARGS__); \
	} while (0)
#define _loginfo(...) fprintf (stdout, __VA_ARGS__)
#define _logdebug(level,...) \
	if (level <= DEBUGLEVEL) \
		fprintf (stderr, __VA_ARGS__);

class Timeout {
	uint cur, max;
public:
	Timeout() : cur(0), max(0) {}
	Timeout(int ms) : cur(ms), max(ms) {}
	void set(int ms) { cur = max = ms; }
	void add(int ms) { cur += ms; max += ms; }
	void clear() { set(0); }
	void reset() { cur = max; }
	uint getMax() { return max; }
	uint get() { return cur; }
	bool running() { return max > 0 && cur > 0; }
	bool expired() { return max > 0 && cur == 0; }
	bool isSet() { return max > 0; }
	bool update(uint ms) {
		if (ms < cur)
			cur -= ms;
		else
			cur = 0;
		return cur == 0;
	}
};

typedef struct {
	string key;
	string value;
} ParserEntry;

class FileParser {
	list<ParserEntry> entries;
	string prefix;
public:
	FileParser(const string&  fname);
	int get(const string&  k, string &v);
	int get(const string&  k, int &v);
	int get(const string&  k, uint &v);
	int get(const string&  k, uint8_t &v);
	int get(const string&  k, double &v);
};

bool dirExists(const string& name);
bool makeDir(const string &name);
bool fileExists(const string& name);

/** Count continuously from start to end. Delay is in milliseconds for
 * changing counter by one (e.g. delay=1000 means it takes one second per step. */
enum {
	SCT_ONCE = 0,
	SCT_REPEAT,
	SCT_UPDOWN
};
class SmoothCounter {
	int type;
	double cpms; /* change per millisecond */
	double cur, min, max;
	bool running;
public:
	void init(int t, double start, double end, double delay) {
		type = t;
		cpms = 1.0 / delay;
		if (end > start) {
			min = cur = start;
			max = end;
		} else {
			max = cur = start;
			min = end;
			cpms *= -1;
		}
		running = true;
	}
	int update(int ms) {
		if (cpms == 0 && !running)
			return 0;

		int ret = 0;
		cur += cpms * ms;
		if (cpms > 0 && cur >= max) {
			if (type == SCT_REPEAT)
				cur = min;
			else if (type == SCT_ONCE)
				cpms = 0;
			else if (type == SCT_UPDOWN) {
				cur = max;
				cpms *= -1;
			}
			ret = 1;
		} else if (cpms < 0 && cur <= min) {
			if (type == SCT_REPEAT)
				cur = max;
			else if (type == SCT_ONCE)
				cpms = 0;
			else if (type == SCT_UPDOWN) {
				cur = min;
				cpms *= -1;
			}
			ret = 1;
		}
		if (type == SCT_ONCE && ret)
			running = false;
		return ret;
	}
	double get() { return cur; }
	bool isRunning() { return running; }
};

class FrameCounter : public SmoothCounter {
public:
	void init(uint max, uint delay) {
		SmoothCounter::init(SCT_REPEAT, 0, -0.01 + max, delay);
	}
	int get() { return SmoothCounter::get(); }
};

void strprintf(string& str, const char *fmt, ... );

/** Simple vector object. There is already struct Vector in libgame
 * so we call it just Vec. */
class Vector {
	double x,y;
public:
	Vector(double _x, double _y) : x(_x), y(_y) {}
	double getX() { return x; }
	double getY() { return y; }
	void normalize() {
		if ( x == 0 && y == 0 )
			return;
		double l = sqrt(x*x+y*y);
		x /= l;
		y /= l;
	}
	double getLength() {
		return sqrt(x*x + y*y);
	}
	void setLength(double l) {
		normalize();
		x *= l;
		y *= l;
	}
	void add(double s, Vector &v) {
		x += s * v.getX();
		y += s * v.getY();
	}
};

enum {
	RD_FILES = 0,
	RD_FOLDERS
};
int readDir(const string &dname, int type, vector<string> &fnames);

string getHomeDir();
const string &getFullLevelsetPath(const string &n);

string trimString(const string& str);

#endif /* SRC_TOOLS_H_ */
