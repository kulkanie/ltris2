/***************************************************************************
                          cpu.h  -  description
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

#ifndef __CPU_H
#define __CPU_H

/* set with scores for all evaluated criteria */
typedef struct {
	int lines; /* score for completed lines */
	int slope; /* score for profile of bowl content */
	int holes; /* score for new holes */
	int block; /* score for new piece tiles blocking existing holes */
	int abyss; /* score for deep gaps */
	int clear; /* used to built gap for triple/tetris clears */
} CPU_ScoreSet;

/* evaluation result for a piece */
typedef struct {
	int id, x, y, rot; /* tested piece info */
	int score; /* evaluation score (total of values in score_set) */
	CPU_ScoreSet score_set; /* single evaluations (for debugging) */
} CPU_Eval;

/* CPU Data contains the original bowl and piece to be evaluated and internal
 * data to do so. After testing and evaluating all positions of the piece in
 * the bowl, the best position is set in dest_*.
 */
typedef struct {
	/* current piece+bowl information set by bowl_quick_game()
	 * before calling cpu_analyze_data() */
	int style; /* defensive, normal, aggressive */
	int bowl_w, bowl_h; /* must be BOWL_WIDTH and BOWL_HEIGHT */
	int piece_id, preview_id, hold_active, hold_id;
	int original_bowl[BOWL_WIDTH][BOWL_HEIGHT]; /* original bowl content: 0 - empty, 1 - tile */

	CPU_ScoreSet base_scores; /* basic multipliers for evaluated criteria */

	/* internal stuff for analysis */
	int bowl[BOWL_WIDTH][BOWL_HEIGHT]; /* this bowl is used to actually compute stuff */
	Block_Mask *piece; /* actual piece tested (pointer to block_masks) */

	/* result */
	int use_hold; /* 1 if result belongs to hold (or preview if empty) piece */
	CPU_Eval result;
} CPU_Data;

/** Analyze cpu_data.original_bowl and cpu_data.original_piece and find
 * best position and rotation and store in cpu_data.result. */
void cpu_analyze_data(CPU_Data *cpu_data);

#endif
