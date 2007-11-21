/*

    File: intrf.h

    Copyright (C) 1998-2007 Christophe GRENIER <grenier@cgsecurity.org>
  
    This software is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
  
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
  
    You should have received a copy of the GNU General Public License along
    with this program; if not, write the Free Software Foundation, Inc., 51
    Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */

enum buffer_cmd {BUFFER_RESET, BUFFER_ADD, BUFFER_WRITE,BUFFER_SHOW};
typedef enum buffer_cmd buffer_cmd_t;

struct MenuItem
{
    const int key; /* Keyboard shortcut; if zero, then there is no more items in the menu item table */
    const char *name; /* Item name, should be eight characters with current implementation */
    const char *desc; /* Item description to be printed when item is selected */
};
#define MAX_LINES		200
#define LINE_LENGTH 		80
#define BUFFER_LINE_LENGTH 	4*LINE_LENGTH
#define MAXIMUM_PARTS 		60
#define COL_ID_WIDTH 		25
#define WARNING_START 		23
#define COLUMNS 		80


#define DUMP_X			0
#define DUMP_Y			5 + 2
#define DUMP_MAX_LINES		14
#define INTER_DUMP_X		DUMP_X
#define INTER_DUMP_Y		DUMP_Y+DUMP_MAX_LINES+1
#define INTER_ANALYSE_X		0
#define INTER_ANALYSE_Y 	8
#define INTER_MAX_LINES 	13
#define INTER_ANALYSE_MENU_X 	0
#define INTER_ANALYSE_MENU_Y 	23
#define INTER_OPTION_X  	0
#define INTER_OPTION_Y		10
#define INTER_PARTITION_X  	0
#define INTER_PARTITION_Y	8
#define INTER_MAIN_X		0
#define INTER_MAIN_Y		18
#define INTER_GEOM_X		0
#define INTER_GEOM_Y  		18
/* Constants for menuType parameter of menuSelect function */
#define MENU_HORIZ 		1
#define MENU_VERT 		2
#define MENU_ACCEPT_OTHERS 	4
#define MENU_BUTTON 		8
#define MENU_VERT_WARN		16
#define MENU_VERT_ARROW2VALID	32
/* Miscellenous constants */
#define MENU_SPACING 		2
#define MENU_MAX_ITEMS 		256 /* for simpleMenu function */
#define key_CR 			'\015'
#define key_ESC 		'\033'
#define key_DEL 		'\177'
#define key_BELL 		'\007'
/* '\014' == ^L */
#define key_REDRAWKEY 		'\014'


/* int aff_buffer(const int cmd, const char *_format, ...) __attribute__ ((format (printf, 2, 3))); */
int aff_buffer(const buffer_cmd_t cmd, const char *_format, ...);
void aff_CHS(const CHS_t * CHS);
void aff_CHS_buffer(const CHS_t * CHS);
void aff_LBA2CHS(const disk_t *disk_car, const unsigned long int pos_LBA);
void log_CHS_from_LBA(const disk_t *disk_car, const unsigned long int pos_LBA);
const char *aff_part_aux(const aff_part_type_t newline, const disk_t *disk_car, const partition_t *partition);
void aff_part_buffer(const aff_part_type_t newline,const disk_t *disk_car,const partition_t *partition);

int ask_confirmation(const char*_format, ...) __attribute__ ((format (printf, 1, 2)));
unsigned long long int ask_number(const unsigned long long int val_cur, const unsigned long long int val_min, const unsigned long long int val_max, const char * _format, ...) __attribute__ ((format (printf, 4, 5)));
unsigned long long int ask_number_cli(char **current_cmd, const unsigned long long int val_cur, const unsigned long long int val_min, const unsigned long long int val_max, const char * _format, ...) __attribute__ ((format (printf, 5, 6)));
int display_message_aux(const char*_format,...) __attribute__ ((format (printf, 1, 2)));
int display_message(const char*msg);
int get_string(char *str, int len, char *def);
void not_implemented(const char *msg);
void screen_buffer_to_log(void);
int interface_partition_type(disk_t *disk_car, const int verbose, char**current_cmd);
void intrf_no_disk(const char *prog_name);
char *ask_log_location(const char*filename);
int ask_log_creation(void);
char *ask_location(const char*msg, const char *src_dir);
void dump_ncurses(const void *nom_dump, unsigned int lng);