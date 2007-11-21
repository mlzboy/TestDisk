/*

    File: testdisk.c

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_LOCALE_H
#include <locale.h>	/* setlocale */
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#include "types.h"
#include "common.h"
#include "testdisk.h"
#include "intrf.h"
#ifdef HAVE_NCURSES
#include "intrfn.h"
#else
#include <stdio.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "intrface.h"
#include "fnctdsk.h"
#include "dir.h"
#include "ext2_dir.h"
#include "rfs_dir.h"
#include "ntfs_dir.h"
#include "hdcache.h"
#include "ewf.h"
#include "log.h"
#include "hdaccess.h"

extern const arch_fnct_t arch_i386;
extern const arch_fnct_t arch_mac;
extern const arch_fnct_t arch_sun;

#ifdef HAVE_SIGACTION
void sighup_hdlr(int shup)
{
  log_critical("SIGHUP detected! TestDisk has been killed.\n");
  log_close();
  exit(1);
}
#endif

#ifdef HAVE_NCURSES
void aff_copy(WINDOW *window)
{
  wclear(window);
  keypad(window, TRUE); /* Need it to get arrow key */
  wmove(window,0,0);
  wprintw(window, "TestDisk %s, Data Recovery Utility, %s",VERSION,TESTDISKDATE);
  wmove(window,1,0);
  wprintw(window,"Christophe GRENIER <grenier@cgsecurity.org>");
  wmove(window,2,0);
  wprintw(window,"http://www.cgsecurity.org");
}
#endif

int main( int argc, char **argv )
{
  int i;
  int help=0, verbose=0, dump_ind=0;
  int create_log=0;     /* 0: no_log, 1: append, 2 create */
  int do_list=0;
  int write_used;
  int saveheader=0;
  int create_backup=0;
  int run_setlocale=1;
  int done=0;
  int safe=0;
  int testdisk_mode=TESTDISK_O_RDWR|TESTDISK_O_READAHEAD_8K;
  list_disk_t *list_disk=NULL;
  list_disk_t *element_disk;
  const char *cmd_device=NULL;
  char *cmd_run=NULL;
#ifdef TARGET_SOLARIS
  const arch_fnct_t *arch=&arch_sun;
#elif defined __APPLE__
  const arch_fnct_t *arch=&arch_mac;
#else
  const arch_fnct_t *arch=&arch_i386;
#endif
#ifdef HAVE_SIGACTION
  struct sigaction action;
#endif
  /* random (weak is ok) is need fot GPT */
  srand(time(NULL));
#ifdef HAVE_SIGACTION
  /* set up the signal handler for SIGHUP */
  action.sa_handler  = sighup_hdlr;
  action.sa_flags = 0;
  if(sigaction(SIGHUP, &action, NULL)==-1)
  {
    printf("Error on SIGACTION call\n");
    return -1;
  }
#endif
  for(i=1;i<argc;i++)
  {
    if((strcmp(argv[i],"/dump")==0) || (strcmp(argv[i],"-dump")==0))
      dump_ind=1;
    else if((strcmp(argv[i],"/log")==0) ||(strcmp(argv[i],"-log")==0))
    {
      if(create_log==0)
      {
        create_log=1;
        log_open("testdisk.log","a","TestDisk",argc,argv);
      }
    }
    else if((strcmp(argv[i],"/debug")==0) || (strcmp(argv[i],"-debug")==0))
    {
      verbose++;
      if(create_log==0)
      {
        create_log=1;
        log_open("testdisk.log","a","TestDisk",argc,argv);
      }
    }
    else if((strcmp(argv[i],"/all")==0) || (strcmp(argv[i],"-all")==0))
      testdisk_mode|=TESTDISK_O_ALL;
    else if((strcmp(argv[i],"/backup")==0) || (strcmp(argv[i],"-backup")==0))
      create_backup=1;
    else if((strcmp(argv[i],"/direct")==0) || (strcmp(argv[i],"-direct")==0))
      testdisk_mode|=TESTDISK_O_DIRECT;
    else if((strcmp(argv[i],"/help")==0) || (strcmp(argv[i],"-help")==0) || (strcmp(argv[i],"--help")==0) ||
      (strcmp(argv[i],"/h")==0) || (strcmp(argv[i],"-h")==0))
      help=1;
    else if((strcmp(argv[i],"/list")==0) || (strcmp(argv[i],"-list")==0))
      do_list=1;
    else if((strcmp(argv[i],"/nosetlocale")==0) || (strcmp(argv[i],"-nosetlocale")==0))
      run_setlocale=0;
    else if((strcmp(argv[i],"/safe")==0) || (strcmp(argv[i],"-safe")==0))
      safe=1;
    else if((strcmp(argv[i],"/saveheader")==0) || (strcmp(argv[i],"-saveheader")==0))
      saveheader=1;
    else if(strcmp(argv[i],"/cmd")==0)
    {
      if(i+2>=argc)
	help=1;
      else
      {
	disk_t *disk_car;
	cmd_device=argv[++i];
	cmd_run=argv[++i];
	disk_car=file_test_availability(cmd_device,verbose,arch,testdisk_mode);
	if(disk_car==NULL)
	{
	  printf("\nUnable to open file or device %s\n",cmd_device);
	  help=1;
	}
	else
	  list_disk=insert_new_disk(list_disk,disk_car);
      }
    }
    else
    {
      disk_t *disk_car=file_test_availability(argv[i],verbose,arch,testdisk_mode);
      if(disk_car==NULL)
      {
	printf("\nUnable to open file or device %s\n",argv[i]);
	help=1;
      }
      else
	list_disk=insert_new_disk(list_disk,disk_car);
    }
  }
  printf("TestDisk %s, Data Recovery Utility, %s\nChristophe GRENIER <grenier@cgsecurity.org>\nhttp://www.cgsecurity.org\n",VERSION,TESTDISKDATE);
  if(help!=0)
  {
    printf("\n" \
	"Usage: testdisk [/log] [/debug] [file or device]\n"\
	"       testdisk /list  [/log]   [file or device]\n" \
	"\n" \
	"/log          : create a testdisk.log file\n" \
	"/debug        : add debug information\n" \
	"/list         : display current partitions\n" \
	"\n" \
	"TestDisk checks and recovers lost partitions\n" \
	"It works with :\n" \
	"- BeFS (BeOS)                           - BSD disklabel (Free/Open/Net BSD)\n" \
	"- CramFS, Compressed File System        - DOS/Windows FAT12, FAT16 and FAT32\n" \
	"- HFS, HFS+, Hierarchical File System   - JFS, IBM's Journaled File System\n" \
	"- Linux Ext2 and Ext3                   - Linux Raid\n" \
	"- Linux Swap                            - LVM, LVM2, Logical Volume Manager\n" \
	"- Netware NSS                           - NTFS (Windows NT/2K/XP/2003)\n" \
	"- ReiserFS 3.5, 3.6 and 4               - Sun Solaris i386 disklabel\n" \
	"- UFS and UFS2 (Sun/BSD/...)            - XFS, SGI's Journaled File System\n" \
	"\n" \
	"If you have problems with TestDisk or bug reports, please contact me.\n");
    return 0;
  }
  aff_buffer(BUFFER_RESET,"Q");
  if(do_list!=0)
  {
    printf("Please wait...\n");
    /* Scan for available device only if no device or image has been supplied in parameter */
    if(list_disk==NULL)
      list_disk=hd_parse(list_disk,verbose,arch,testdisk_mode);
    /* Activate the cache */
    for(element_disk=list_disk;element_disk!=NULL;element_disk=element_disk->next)
      element_disk->disk=new_diskcache(element_disk->disk,testdisk_mode);
#ifdef DJGPP
    for(element_disk=list_disk;element_disk!=NULL;element_disk=element_disk->next)
    {
      printf("%s\n",element_disk->disk->description(element_disk->disk));
    }
#endif
    if(safe==0)
      hd_update_all_geometry(list_disk,0,verbose);
    for(element_disk=list_disk;element_disk!=NULL;element_disk=element_disk->next)
    {
      printf("%s, sector size=%u\n",element_disk->disk->description(element_disk->disk),element_disk->disk->sector_size);
    }
    printf("\n");

    for(element_disk=list_disk;element_disk!=NULL;element_disk=element_disk->next)
    {
      interface_list(element_disk->disk,verbose,0,saveheader,create_backup, &cmd_run);
    }
    delete_list_disk(list_disk);
    return 0;
  }
#ifdef HAVE_SETLOCALE
  if(run_setlocale>0)
  {
    const char *locale;
    locale = setlocale (LC_ALL, "");
    if (locale==NULL) {
      locale = setlocale (LC_ALL, NULL);
      log_error("Failed to set locale, using default '%s'.\n", locale);
    } else {
      log_info("Using locale '%s'.\n", locale);
    }
  }
#endif
#ifdef HAVE_NCURSES
  /* ncurses need locale for correct unicode support */
  if(start_ncurses("TestDisk",argv[0]))
    return 1;
#endif
  if(argc==1)
  {
    verbose=1;
    create_log=ask_log_creation();
    if(create_log>0)
      log_open("testdisk.log",(create_log==1?"a":"w"),"TestDisk",argc,argv);
  }
  log_info("TestDisk %s, Data Recovery Utility, %s\nChristophe GRENIER <grenier@cgsecurity.org>\nhttp://www.cgsecurity.org\n",VERSION,TESTDISKDATE);
  log_info(TESTDISK_OS);
  log_info(" (ext2fs lib: %s, ntfs lib: %s, reiserfs lib: %s, ewf lib: %s)\n",td_ext2fs_version(),td_ntfs_version(),td_reiserfs_version(), td_ewf_version());
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(DJGPP)
#else
#ifdef HAVE_GETEUID
  if(geteuid()!=0)
  {
    log_warning("User is not root!\n");
  }
#endif
#endif
#ifdef HAVE_NCURSES
  aff_copy(stdscr);
  wmove(stdscr,5,0);
  wprintw(stdscr, "Please wait...\n");
#endif
  /* Scan for available device only if no device or image has been supplied in parameter */
  if(list_disk==NULL)
    list_disk=hd_parse(list_disk,verbose,arch,testdisk_mode);
  /* Activate the cache */
  for(element_disk=list_disk;element_disk!=NULL;element_disk=element_disk->next)
    element_disk->disk=new_diskcache(element_disk->disk,testdisk_mode);
#ifdef HAVE_NCURSES
  wmove(stdscr,6,0);
  for(element_disk=list_disk;element_disk!=NULL;element_disk=element_disk->next)
  {
    wprintw(stdscr,"%s\n",element_disk->disk->description(element_disk->disk));
  }
  wrefresh(stdscr);
#endif
  if(safe==0)
    hd_update_all_geometry(list_disk,0,verbose);
  /* save disk parameters to rapport */
  log_info("Hard disk list\n");
  for(element_disk=list_disk;element_disk!=NULL;element_disk=element_disk->next)
    log_info("%s, sector size=%u\n",element_disk->disk->description(element_disk->disk),element_disk->disk->sector_size);
  log_info("\n");
  do_curses_testdisk(verbose,dump_ind,list_disk,saveheader,cmd_device,&cmd_run);
#ifdef HAVE_NCURSES
  end_ncurses();
#endif
  log_info("\n");
  while(done==0)
  {
    int command='Q';
    if(cmd_run!=NULL)
    {
      while(cmd_run[0]==',')
	(cmd_run)++;
      if(strncmp(cmd_run,"list",4)==0)
      {
	(cmd_run)+=4;
	command='L';
      }
      else if(cmd_run[0]!='\0')
      {
	log_critical("error in command line: %s\n",cmd_run);
	printf("error in command line: %s\n",cmd_run);
      }
    }
    switch(command)
    {
      case 'L':
	for(element_disk=list_disk;element_disk!=NULL;element_disk=element_disk->next)
	  interface_list(element_disk->disk,verbose,0, saveheader, create_backup, &cmd_run);
	break;
      case 'q':
      case 'Q':
	done = 1;
	break;
    }
  }
  cmd_device=NULL;
  cmd_run=NULL;
  write_used=delete_list_disk(list_disk);
  log_info("TestDisk exited normally.\n");
  if(log_close()!=0)
  {
    printf("TestDisk: Log file corrupted!\n");
  }
  else
  {
    printf("TestDisk exited normally.\n");
  }
  if(write_used!=0)
  {
    printf("You have to reboot for the change to take effect.\n");
  }
  return 0;
}