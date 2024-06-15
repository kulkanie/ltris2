/*
 * vcharts.h
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

#include "../libgame/chart.h"
#include "vcharts.h"

/** Create chart wrapper and load all highscore charts. */
VCharts::VCharts() {
	chart_load();
}

/** Destroy chart wrapper and save all highscore charts. */
VCharts::~VCharts() {
	chart_save();
}
