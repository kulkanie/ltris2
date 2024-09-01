/***************************************************************************
                          config.h  -  description
                             -------------------
    begin                : Tue Feb 13 2001
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

#ifndef __CONFIG_H
#define __CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_FILE_NAME "ltris.conf"

/* game type ids */
enum {
    GAME_DEMO = 0,
    GAME_CLASSIC,
    GAME_FIGURES,
    GAME_VS_HUMAN,
    GAME_VS_CPU,
    GAME_VS_HUMAN_HUMAN,
    GAME_VS_HUMAN_CPU,
    GAME_VS_CPU_CPU,
    GAME_TRAINING,
    GAME_TYPENUM,

    CS_DEFENSIVE = 0,
    CS_NORMAL,
    CS_AGGRESSIVE
};

typedef struct {
    int left;
    int right;
    int rot_left;
    int rot_right;
    int down;
    int drop;
    int hold;
} Controls;

typedef struct {
    char        name[32];
    Controls    controls;
} Player;

/* configure struct */
typedef struct {
    /* directory to save config and saved games */
    char dir_name[512];
    /* game options */
    int gametype;
    int starting_level;
    int preview;
    int modern;
    int expert;
    Player player1;
    Player player2;
    Player player3;
    /* gamepad */
    int gp_enabled;
    int gp_lrot;
    int gp_rrot;
    int gp_hdrop;
    int gp_pause;
    int gp_hold;
    /* multiplayer */
    int holes;
    int rand_holes;
    int send_all;
    int send_tetris;
    /* cpu */
    int cpu_style; /* cpu style */
    int cpu_delay; /* delay in ms before CPU soft drops */
    int cpu_rot_delay; /* delay between rotation steps */
    int cpu_sfactor; /* multiplier for dropping speed in 0.25 steps */
    /* controls */
    int	as_delay;
    int	as_speed;
    int vert_delay;
    int pause_key;
    int hyper_das;
    /* sound */
    int sound;
    int volume; /* 1 - 8 */
    int shiftsound;
    /* graphics */
    int anim;
    int fullscreen;
    int fade;
    int fps; /* frames per second: 0 - no limit, 1 - 50, 2 - 100, 3 - 200 */
    int show_fps;
    int bkgnd;
    int block_by_block;
    /* lbreakout2 event data */
    int motion_mod;
    int rel_motion;
    int grab;
    int invert;
    /* various */
    int quick_help;
    int visualize; /* compute stats hidden? */
    int keep_bkgnd;
} Config;

/* set config to default */
void config_reset();

/* load config */
void config_load( );

/* save config */
void config_save( );

#ifdef __cplusplus
};
#endif

#endif
