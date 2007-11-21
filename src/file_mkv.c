/*

    File: file_mkv.c

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
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdio.h>
#include "types.h"
#include "filegen.h"

static void register_header_check_mkv(file_stat_t *file_stat);
static int header_check_mkv(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new);

const file_hint_t file_hint_mkv= {
  .extension="mkv",
  .description="Matroska",
  .min_header_distance=0,
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .header_check=&header_check_mkv,
  .register_header_check=&register_header_check_mkv
};

static const unsigned char mkv_header[4]= { 0x1a,0x45,0xdf,0xa3};

static void register_header_check_mkv(file_stat_t *file_stat)
{
  register_header_check(0, mkv_header,sizeof(mkv_header), &header_check_mkv, file_stat);
}

static int header_check_mkv(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  if(memcmp(buffer,mkv_header,sizeof(mkv_header))==0 && memcmp(buffer+8,"matroska",8)==0)
  {
    reset_file_recovery(file_recovery_new);
    file_recovery_new->extension=file_hint_mkv.extension;
    file_recovery_new->min_filesize=0;
    file_recovery_new->calculated_file_size=(uint64_t)(buffer[0x20]<<24)+(((uint64_t)buffer[0x21])<<16)+(((uint64_t)buffer[0x22])<<8)+((uint64_t)buffer[0x23])+0x24;
    file_recovery_new->data_check=&data_check_size;
    file_recovery_new->file_check=&file_check_size;
    return 1;
  }
  return 0;
}