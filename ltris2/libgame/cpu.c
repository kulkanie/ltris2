/***************************************************************************
                          cpu.c  -  description
                             -------------------
    begin                : Sun Jan 6 2002
    copyright            : (C) 2001 by Michael Speck
    email                : kulkanie@gmx.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ltris.h"
#include "list.h"
#include "cpu.h"

extern Block_Mask block_masks[BLOCK_COUNT];
extern Config config;

/** Reset bowl to original content (0 = empty, 1 = tile). */
static void cpu_reset_bowl(CPU_Data *cpu_data)
{
	int i, j;
	for (i = 0; i < cpu_data->bowl_w; i++)
		for (j = 0; j < cpu_data->bowl_h; j++)
			cpu_data->bowl[i][j] = cpu_data->original_bowl[i][j];
}

/** Check if cpu_data->piece fits at this position.
 * Return 0 if invalid or 1 if valid. */
static int cpu_validate_piece_pos(CPU_Data *cpu_data, int x, int y, int rot)
{
	int i, j;
	for (j = 3; j >= 0; j--)
		for (i = 3; i >= 0; i--)
			if (cpu_data->piece->mask[rot][i][j]) {
				if (x + i < 0 || x + i >= cpu_data->bowl_w)
					return 0;
				if (y + j >= cpu_data->bowl_h)
					return 0;
				if (y + j < 0)
					continue;
				if (cpu_data->bowl[x + i][y + j])
					return 0;
			}
	return 1;
}

/** Drop and insert piece to bowl if possible with tile id 2.
 * Return 1 on success, 0 otherwise. */
static int cpu_insert_piece(CPU_Data *cpu_data, int x, int rot, int *y)
{
	int i, j;

	/* check if out of bowl at the sides*/
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++)
			if (cpu_data->piece->mask[rot][i][j])
				if (x + i < 0 || x + i >= cpu_data->bowl_w)
					return 0;
	}

	/* drop tile down */
	*y = -3;
	while (cpu_validate_piece_pos(cpu_data, x, *y + 1, rot))
		(*y)++;

	/* insert block */
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++)
			if (cpu_data->piece->mask[rot][i][j])
				cpu_data->bowl[x + i][(*y) + j] = 2;
	}

	return 1;
}

/** Count number of completed lines and set tile id to 3 for these */
static int cpu_count_completed_lines(CPU_Data *cpu_data)
{
	int i, j, line_count;
	line_count = 0;
	for (j = 0; j < cpu_data->bowl_h; j++) {
		for (i = 0; i < cpu_data->bowl_w; i++)
			if (!cpu_data->bowl[i][j]) break;
		if (i == cpu_data->bowl_w) {
			for (i = 0; i < cpu_data->bowl_w; i++)
				cpu_data->bowl[i][j] = 3;
			line_count++;
		}
	}
	return line_count;
}

/** Remove completed lines. */
static void cpu_remove_completed_lines(CPU_Data *cpu_data)
{
	int i, j, l;
	int line_count = 0;
	int line_y[4];

	/* count lines and get pos */
	for (j = 0; j < cpu_data->bowl_h; j++) {
		if (cpu_data->bowl[0][j] == 3) {
			line_y[line_count++] = j;
		}
	}

	/* remove */
	for (j = 0; j < line_count; j++)
		for (i = 0; i < cpu_data->bowl_w; i++) {
			for (l = line_y[j]; l > 0; l--)
				cpu_data->bowl[i][l] = cpu_data->bowl[i][l - 1];
			cpu_data->bowl[i][0] = 0;
		}
}

/** Get height of column. */
static int cpu_get_column_height(CPU_Data *cpu_data, int col)
{
	int j;

	/* "outer columns" -1 and w have height of bowl. */
	if (col == -1 || col == cpu_data->bowl_w)
		return cpu_data->bowl_h;

	if (col < 0 || col >= cpu_data->bowl_w) {
		printf("%s: illegal column index %d",__FILE__,col);
		return 0; /* illegal column */
	}

	for (j = 0; j < cpu_data->bowl_h; j++)
		if (cpu_data->bowl[col][j] != 0)
			break;

	return cpu_data->bowl_h - j;
}

/* Get y-value for first tile in column. bowl_h means no tiles in column. */
static int cpu_get_column_starty(CPU_Data *cpu_data, int col) {
	int h = cpu_get_column_height(cpu_data, col);
	return cpu_data->bowl_h - h;
}

/** Get bowl height by checking all columns. */
static int cpu_get_bowl_height(CPU_Data *cpu_data)
{
	int height = 0;
	for (int i = 0; i < cpu_data->bowl_w; i++) {
		if (cpu_get_column_height(cpu_data, i) > height)
			height = cpu_get_column_height(cpu_data, i);
	}
	return height;
}

/** Weigh a value by building sum, so each +1 gets more and more significance */
static int get_wval(int v) {
	int wv = 0;
	for (int i = 1; i <= v; i++)
		wv += i;
	return wv;
}

/** Get height difference with right column. >0 increasing, <0 decreasing. */
static int cpu_get_height_diff_r(CPU_Data *cpu_data, int col)
{
	if (col < 0 || col >= cpu_data->bowl_w-1) {
		printf("%s: illegal column index %d",__FILE__,col);
		return 0;
	}
	return cpu_get_column_height(cpu_data, col+1) -
				cpu_get_column_height(cpu_data, col);
}

/** Get height difference with left column. >0 increasing, <0 decreasing. */
static int cpu_get_height_diff_l(CPU_Data *cpu_data, int col)
{
	if (col < 1 || col >= cpu_data->bowl_w) {
		printf("%s: illegal column index %d",__FILE__,col);
		return 0;
	}
	return cpu_get_column_height(cpu_data, col-1) -
				cpu_get_column_height(cpu_data, col);
}

/** Get first gap y (from top) for column col. If not found (= no gap)
 * return height of bowl as illegal position. */
static int cpu_column_get_first_gapy(CPU_Data *cpu_data, int col)
{
	if (col < 0 || col >= cpu_data->bowl_w) {
		printf("%s: illegal column index %d",__FILE__,col);
		return 0;
	}

	int y = cpu_get_column_starty(cpu_data, col);
	for (int i = y; i < cpu_data->bowl_h; i++)
		if (cpu_data->bowl[col][i] == 0)
			return i;
	return cpu_data->bowl_h;
}

/** This is the main analyze function. cpu_data's bowl already contains
 * the positioned piece (id=2) and we have to evaluate the placement and
 * return the result in eval->score. */
static void cpu_analyze_bowl(CPU_Data *cpu_data, CPU_Eval *eval)
{
	int i, j;
	int line_count;
	int bheight = 0;
	int line_score;
	int abyss_depth;
	int newtiles = 0;
	CPU_ScoreSet *bscores = &cpu_data->base_scores;

	/* get bowl height */
	bheight = cpu_get_bowl_height(cpu_data);

	/* count tiles of newly placed piece in last column for "clear" eval */
	if (bheight <= cpu_data->bowl_h/3) {
		for (i = 0; i < cpu_data->bowl_h; i++)
			if (cpu_data->bowl[cpu_data->bowl_w-1][i] == 2)
				newtiles++;
	}

	/* count completed lines -- lines that will be removed are marked as 3 */
	line_count = cpu_count_completed_lines(cpu_data);
	cpu_remove_completed_lines(cpu_data);

	/* evaluate */

	/* lines: the higher the current bowl content the more valuable is removing lines */
	line_score = bscores->lines + (bscores->lines * bheight / cpu_data->bowl_h);
	if (line_count == 1) {
		/* punish single clears for low bowl content */
		if (bheight <= 5)
			eval->score_set.lines = -line_score;
		else if (bheight <= 12)
			eval->score_set.lines = -(line_score/2);
	} else
		eval->score_set.lines = line_score * get_wval(line_count);

	/* clear: special evaluation to encourage clearing more that two lines
	 * by having a gap for an i-piece at the right hand side. does not work
	 * for classic since bowl content gets too unbalanced too quickly due
	 * to random pieces. */
	if (config.modern && cpu_data->style == CS_AGGRESSIVE) {
		eval->score_set.clear = 0;

		/* punish piece placement in last column if it does not result
		 * in three or more cleared lines to encourage tetrises. */
		if (newtiles > 0 && line_count < 3)
			eval->score_set.clear -= bscores->clear * newtiles;

		/* give bonus for abyss up to depth 6 in last column if bowl
		 * content is not too high, this also automatically punishes
		 * building higher than the left neighbor column because for
		 * d<0 we decrease the eval score. */
		if (bheight <= cpu_data->bowl_h/3) {
			int d = cpu_get_height_diff_l(cpu_data, cpu_data->bowl_w-1);
			if (d > 6)
				d = 6; /* limit wanted depth */
			eval->score_set.clear += d * bscores->clear/2;
		}
	}

	/* holes: each gives some (negative) score, new holes appear by placed piece,
	 * old holes might disappear on clearing lines, so we need the total balance */
	for (i = 0; i < cpu_data->bowl_w; i++) {
		int sy = cpu_get_column_starty(cpu_data, i);
		for (j = sy; j < cpu_data->bowl_h; j++)
			if (!cpu_data->bowl[i][j])
				eval->score_set.holes += bscores->holes;
	}

	/* slope: height difference to adjacent columns. the outer columns are only
	 * measured once this way, since it is ok to have more difference there, e.g.
	 * putting an i-piece at a far end is ok or having a gap for an i-piece there
	 * is actually good. so we try to keep the middle of the bowl balanced. */
	for (i = 1; i < cpu_data->bowl_w - 1; i++) {
		eval->score_set.slope += bscores->slope *
				abs(cpu_get_height_diff_r(cpu_data, i));
		eval->score_set.slope += bscores->slope *
				abs(cpu_get_height_diff_l(cpu_data, i));
	}

	/* abyss: punish deep single column gaps */
	for (i = 0; i < cpu_data->bowl_w; i++) {
		/* don't punish abyss in last column if bowl is not too full
		 * to allow using i-piece for tetris if playing aggressive */
		if (cpu_data->style >= CS_NORMAL &&
					i == cpu_data->bowl_w-1 &&
					bheight < cpu_data->bowl_h/3)
			continue;

		/* get lowest neighbor height */
		int nh = cpu_get_column_height(cpu_data, i - 1);
		if (cpu_get_column_height(cpu_data, i + 1) < nh)
			nh = cpu_get_column_height(cpu_data, i + 1);

		abyss_depth = nh - cpu_get_column_height(cpu_data, i);

		if (abyss_depth < 0)
			continue; /* column is higher than its neighbors */

		if (abyss_depth >= 2) /* small gaps for j and l pieces are ok */
			eval->score_set.abyss += bscores->abyss * abyss_depth;
	}

	/* block: to keep the bowl down we need to complete lines and
	 * therefore keep the upper holes in reach. so we punish each
	 * new blocking tile if there is a hole at max one tile below
	 * new piece tiles in a column for aggressive play. this leads
	 * to more "towers" in the middle of the bowl but less holes
	 * and more multiple line clears. however, if the bowl content
	 * gets to high we switch to a more balanced play style. */
	if (cpu_data->style >= CS_NORMAL && bheight < cpu_data->bowl_h/2) {
		for (i = 0; i < cpu_data->bowl_w; i++) {
			int gy = cpu_column_get_first_gapy(cpu_data, i);
			if (gy == cpu_data->bowl_h)
				continue; /* no gap */
			int y = cpu_get_column_starty(cpu_data, i);
			newtiles = 0;
			for (j = y; j < cpu_data->bowl_h; j++) {
				if (cpu_data->bowl[i][j] == 2)
					newtiles++;
				else
					break;
			}
			if (newtiles && gy - j <= 1)
				eval->score_set.block += newtiles *  bscores->block;
		}
	} else {
		/* XXX old broken block evaluation... which just counts
		 * remaining tiles of a new piece. this rewards using a piece
		 * for line clears and shouldn't do much actually. except it
		 * does give a HUGE edge for balanced long play ... I absolutely
		 * don't know why but I'll take it. */
		int debris = 0;
		for (i = 0; i < cpu_data->bowl_w; i++)
			for (j = 0; j < cpu_data->bowl_h; j++)
				if (cpu_data->bowl[i][j] == 2)
					debris++;
		eval->score_set.block = debris * bscores->block;
	}

	/* total score */
	eval->score = eval->score_set.holes + eval->score_set.lines +
			eval->score_set.abyss + eval->score_set.slope +
			eval->score_set.block + eval->score_set.clear;
}

/** Analyze cpu_data's original bowl and piece by testing all possible
 * piece placements. Return the best placement as cpu_data.result. */
void cpu_analyze_data(CPU_Data *cpu_data)
{
	int x, rot, y;
	CPU_Eval cur_eval;
	int first_eval = 1;
	int pid = 0;

	cpu_data->use_hold = 0;

	/* get and analyze valid positions of block */
	for (int i = 0; i < 2; i++) {
		if (i == 0) {/* test current piece */
			pid = cpu_data->piece_id;
		} else if (cpu_data->hold_active) {
			/* if hold allowed, test hold piece or preview piece if none yet */
			if (cpu_data->hold_id != -1) {
				pid = cpu_data->hold_id;
			} else {
				pid = cpu_data->preview_id;
			}
		} else
			break;

		cpu_data->piece = &block_masks[pid];
		for (rot = 0; rot < 4; rot++) {
			for (x = -4; x < 14; x++) {
				cpu_reset_bowl(cpu_data);
				if (!cpu_insert_piece(cpu_data, x, rot, &y))
					continue;

				memset(&cur_eval,0,sizeof(CPU_Eval));
				cur_eval.id = pid;
				cur_eval.x = x;
				cur_eval.y = y;
				cur_eval.rot = rot;
				cpu_analyze_bowl(cpu_data, &cur_eval);

				/* better than current result? replace! */
				if (first_eval || cur_eval.score > cpu_data->result.score ||
						(cur_eval.score == cpu_data->result.score &&
								cur_eval.y > cpu_data->result.y)) {
					cpu_data->result = cur_eval;
					if (i == 1) /* testing hold option */
						cpu_data->use_hold = 1;
				}
				first_eval = 0;
			}
		}
	}
}
