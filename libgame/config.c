/***************************************************************************
                          config.c  -  description
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "sdl.h"
#include "config.h"
#include "parser.h"

Config config;

/* game type names. hacky place to define them but who cares? */
char gametype_names[GAME_TYPENUM][64];
char gametype_ids[GAME_TYPENUM][64];

/* check if config directory exists; if not create it and set config_dir */
void config_check_dir()
{
#ifdef DISABLE_INSTALL
    sprintf( config.dir_name, "." );
#else    
    snprintf( config.dir_name, sizeof(config.dir_name)-1, "%s/%s", getenv( "HOME" ), CONFIG_DIR_NAME );
#endif
    if ( opendir( config.dir_name ) == 0 ) {
        fprintf( stderr, "couldn't find/open config directory '%s'\n", config.dir_name );
        fprintf( stderr, "attempting to create it... " );
#ifdef WIN32
        mkdir( config.dir_name );
#else
        mkdir( config.dir_name, S_IRWXU );
#endif
        if ( opendir( config.dir_name ) == 0 )
            fprintf( stderr, "failed\n" );
        else
            fprintf( stderr, "ok\n" );
    }
}

/* set config to default */
void config_reset()
{
    /* game options */
    config.gametype = GAME_CLASSIC;
    config.starting_level = 0;
    config.preview = 1;
    strcpy( config.player1.name, "Michael" );
    strcpy( config.player2.name, "Sabine" );
    strcpy( config.player3.name, "Thomas" );
    config.expert = 0;
    config.modern = 1;
    /* multiplayer */
    config.holes = 1;
    config.rand_holes = 0;
    config.send_all = 0;
    config.send_tetris = 1;
    /* cpu */
    config.cpu_style = CS_NORMAL;
    config.cpu_delay = 500;
    config.cpu_rot_delay = 100;
    config.cpu_sfactor = 100;
    /* controls */
    config.as_delay = 170;
    config.as_speed = 50;
    config.vert_delay= 3;
    config.hyper_das = 0;
    config.pause_key = SDLK_p;
    config.player1.controls.left = SDLK_LEFT;
    config.player1.controls.right = SDLK_RIGHT;
    config.player1.controls.rot_left = SDLK_UP;
    config.player1.controls.rot_right = SDLK_PAGEDOWN;
    config.player1.controls.down = SDLK_DOWN;
    config.player1.controls.drop = SDLK_END;
    config.player1.controls.hold = SDLK_DELETE;
    config.player2.controls.left = 'a';
    config.player2.controls.right = 'd';
    config.player2.controls.rot_left = 'w';
    config.player2.controls.rot_right = 'e';
    config.player2.controls.down = 's';
    config.player2.controls.drop = 'y';
    config.player2.controls.hold = 'q';
    config.player3.controls.left = SDLK_KP_1;
    config.player3.controls.right = SDLK_KP_3;
    config.player3.controls.rot_left = SDLK_KP_5;
    config.player3.controls.rot_right = SDLK_KP_6;
    config.player3.controls.down = SDLK_KP_2;
    config.player3.controls.drop = SDLK_KP_8;
    config.player3.controls.hold = SDLK_KP_7;
    config.gp_enabled = 1;
    config.gp_lrot = 3;
    config.gp_rrot = 0;
    config.gp_hdrop = 2;
    config.gp_pause = 9;
    config.gp_hold = 1;
    /* sound */
    config.sound = 1;
    config.volume = 6; /* 1 - 8 */
    config.shiftsound = 1;
    /* graphics */
    config.anim = 1;
    config.fullscreen = 0;
    config.fade = 1;
    config.fps = 0; /* frames per second: 0 - 60Hz, 1 - 50Hz */
    config.show_fps = 0;
    config.bkgnd = 0;
    config.block_by_block = 0;
    /* lbreakout2 event data */
    config.rel_motion = 0;
    config.motion_mod = 100;
    config.invert = 0;
    config.grab = 0;
    /* various */
    config.quick_help = 1;
    config.visualize = 0;
    config.keep_bkgnd = 0;
}

/* load config */
static void parse_player( PData *pd, Player *player )
{
    char *str;
    PData *sub;
    if ( parser_get_value( pd, "name", &str, 0 ) )
        strcpy( player->name, str );
    if ( parser_get_pdata( pd, "controls", &sub ) ) {
        parser_get_int( sub, "left", &player->controls.left );
        parser_get_int( sub, "right", &player->controls.right );
        parser_get_int( sub, "rot_left", &player->controls.rot_left );
        parser_get_int( sub, "rot_right", &player->controls.rot_right );
        parser_get_int( sub, "down", &player->controls.down );
        parser_get_int( sub, "drop", &player->controls.drop );
        parser_get_int( sub, "hold", &player->controls.hold );
    }
}
void config_load( )
{
    char file_name[512];
    PData *pd, *sub; 
    /* set to defaults */
    config_check_dir();
    config_reset();
    /* load config */
    sprintf( file_name, "%s/%s", config.dir_name, CONFIG_FILE_NAME );
    if ( ( pd = parser_read_file( "config", file_name ) ) == 0 ) {
        fprintf( stderr, "%s\n", parser_get_error() );
        return;
    }
    /* parse config */
    parser_get_int( pd, "gametype", &config.gametype );
    parser_get_int( pd, "starting_level", &config.starting_level );
    if (config.starting_level > 19 || config.starting_level < 0) {
	    config.starting_level = 0;
    }
    parser_get_int( pd, "preview", &config.preview );
    parser_get_int( pd, "modern", &config.modern );
    parser_get_int( pd, "holes", &config.holes );
    parser_get_int( pd, "rand_holes", &config.rand_holes );
    parser_get_int( pd, "send_all", &config.send_all );
    parser_get_int( pd, "send_tetris", &config.send_tetris );
    if ( parser_get_pdata( pd, "player1", &sub ) )
        parse_player( sub, &config.player1 );
    if ( parser_get_pdata( pd, "player2", &sub ) )
        parse_player( sub, &config.player2 );
    if ( parser_get_pdata( pd, "player3", &sub ) )
        parse_player( sub, &config.player3 );

    parser_get_int( pd, "gp_enabled", &config.gp_enabled );
    parser_get_int( pd, "gp_lrot", &config.gp_lrot );
    parser_get_int( pd, "gp_rrot", &config.gp_rrot );
    parser_get_int( pd, "gp_hdrop", &config.gp_hdrop );
    parser_get_int( pd, "gp_pause", &config.gp_pause );
    parser_get_int( pd, "gp_hold", &config.gp_hold );

    parser_get_int( pd, "cpu_style", &config.cpu_style );
    parser_get_int( pd, "cpu_delay", &config.cpu_delay );
    parser_get_int( pd, "cpu_rot_delay", &config.cpu_rot_delay );
    parser_get_int( pd, "cpu_sfactor", &config.cpu_sfactor );
    parser_get_int( pd, "sound", &config.sound );
    parser_get_int( pd, "volume", &config.volume );
    parser_get_int( pd, "shiftsound", &config.shiftsound );
    parser_get_int( pd, "animations", &config.anim );
    parser_get_int( pd, "fullscreen", &config.fullscreen );
    parser_get_int( pd, "fading", &config.fade );
    parser_get_int( pd, "fps", &config.fps );
    parser_get_int( pd, "show_fps", &config.show_fps );
    parser_get_int( pd, "background", &config.bkgnd );
    parser_get_int( pd, "static_background", &config.keep_bkgnd );
    parser_get_int( pd, "as_delay", &config.as_delay );
    parser_get_int( pd, "as_speed", &config.as_speed );
    parser_get_int( pd, "vert_delay", &config.vert_delay );
    parser_get_int( pd, "pause_key", &config.pause_key );
    parser_get_int( pd, "block_by_block", &config.block_by_block );
    parser_get_int( pd, "motion_mod", &config.motion_mod );
    parser_get_int( pd, "relative_motion", &config.rel_motion );
    parser_get_int( pd, "grap_input", &config.grab );
    parser_get_int( pd, "invert_mouse", &config.invert );
    parser_get_int( pd, "quick_help", &config.quick_help );
    parser_get_int( pd, "hyper_das", &config.hyper_das );
    parser_free( &pd );
}

/* save config */
static void print_player( FILE *file, int i, Player *player )
{
    fprintf( file, "<player%i\n", i );
    fprintf( file, "name=%s\n", player->name );
    fprintf( file, "<controls\n" );
    fprintf( file, "left=%i\n", player->controls.left );
    fprintf( file, "right=%i\n", player->controls.right );
    fprintf( file, "rot_left=%i\n", player->controls.rot_left );
    fprintf( file, "rot_right=%i\n", player->controls.rot_right );
    fprintf( file, "down=%i\n", player->controls.down );
    fprintf( file, "drop=%i\n", player->controls.drop );
    fprintf( file, "hold=%i\n", player->controls.hold );
    fprintf( file, ">\n" );
    fprintf( file, ">\n" );
}
void config_save( )
{
    FILE *file = 0;
    char file_name[512];

    sprintf( file_name, "%s/%s", config.dir_name, CONFIG_FILE_NAME );
    if ( ( file = fopen( file_name, "w" ) ) == 0 )
        fprintf( stderr, "Cannot access config file '%s' to save settings\n", file_name );
    else {
        fprintf( file, "@\n" );
        fprintf( file, "gametype=%i\n", config.gametype );
        fprintf( file, "starting_level=%i\n", config.starting_level );
        fprintf( file, "preview=%i\n", config.preview );
        fprintf( file, "modern=%i\n", config.modern );
        fprintf( file, "holes=%i\n", config.holes );
        fprintf( file, "rand_holes=%i\n", config.rand_holes );
        fprintf( file, "send_all=%i\n", config.send_all );
        fprintf( file, "send_tetris=%i\n", config.send_tetris );
        print_player( file, 1, &config.player1 );
        print_player( file, 2, &config.player2 );
        print_player( file, 3, &config.player3 );

        fprintf( file, "gp_enabled=%d\n", config.gp_enabled );
        fprintf( file, "gp_lrot=%d\n", config.gp_lrot );
        fprintf( file, "gp_rrot=%d\n", config.gp_rrot );
        fprintf( file, "gp_hdrop=%d\n", config.gp_hdrop );
        fprintf( file, "gp_pause=%d\n", config.gp_pause );
        fprintf( file, "gp_hold=%d\n", config.gp_hold );

        fprintf( file, "cpu_style=%i\n", config.cpu_style );
        fprintf( file, "cpu_delay=%i\n", config.cpu_delay );
        fprintf( file, "cpu_rot_delay=%i\n", config.cpu_rot_delay );
        fprintf( file, "cpu_sfactor=%d\n", config.cpu_sfactor );
        fprintf( file, "sound=%i\n", config.sound );
        fprintf( file, "volume=%i\n", config.volume );
        fprintf( file, "shiftsound=%i\n", config.shiftsound );
        fprintf( file, "animations=%i\n", config.anim );
        fprintf( file, "fullscreen=%i\n", config.fullscreen );
        fprintf( file, "fading=%i\n", config.fade );
        fprintf( file, "fps=%i\n", config.fps );
        fprintf( file, "show_fps=%i\n", config.show_fps );
        fprintf( file, "background=%i\n", config.bkgnd );
        fprintf( file, "static_background=%i\n", config.keep_bkgnd );
        fprintf( file, "as_delay=%i\n", config.as_delay );
        fprintf( file, "as_speed=%i\n", config.as_speed );
        fprintf( file, "vert_delay=%i\n", config.vert_delay );
        fprintf( file, "pause_key=%i\n", config.pause_key );
        fprintf( file, "block_by_block=%i\n", config.block_by_block );
        fprintf( file, "motion_mod=%i\n", config.motion_mod );
        fprintf( file, "relative_motion=%i\n", config.rel_motion );
        fprintf( file, "grab_input=%i\n", config.grab );
        fprintf( file, "invert_mouse=%i\n", config.invert );
        fprintf( file, "quick_help=%i\n", config.quick_help );
        fprintf( file, "hyper_das=%i\n", config.hyper_das );
    }
}
