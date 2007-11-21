/*

    File: file_swf.c

    Copyright (C) 2007 Christophe GRENIER <grenier@cgsecurity.org>
  
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


static void register_header_check_swf(file_stat_t *file_stat);
static int header_check_swf(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new);

const file_hint_t file_hint_swf= {
  .extension="swf",
  .description="Macromedia Flash (Compiled)",
  .min_header_distance=0,
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .header_check=&header_check_swf,
  .register_header_check=&register_header_check_swf
};

static const unsigned char swf_header_compressed[3]= {'C','W','S'};
static const unsigned char swf_header[3]= {'F','W','S'};

static void register_header_check_swf(file_stat_t *file_stat)
{
  register_header_check(0, swf_header_compressed,sizeof(swf_header_compressed), &header_check_swf, file_stat);
  register_header_check(0, swf_header,sizeof(swf_header), &header_check_swf, file_stat);
}

static int header_check_swf(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  if(memcmp(buffer, swf_header_compressed, sizeof(swf_header_compressed))==0)
  { /* Compressed flash */
    reset_file_recovery(file_recovery_new);
    file_recovery_new->extension="swc";
    return 1;
  }
  if(memcmp(buffer, swf_header, sizeof(swf_header))==0)
  {
    reset_file_recovery(file_recovery_new);
    file_recovery_new->extension=file_hint_swf.extension;
    file_recovery_new->calculated_file_size=(uint64_t)buffer[4]+(((uint64_t)buffer[5])<<8)+(((uint64_t)buffer[6])<<16)+(((uint64_t)buffer[7])<<24);
    file_recovery_new->data_check=&data_check_size;
    file_recovery_new->file_check=&file_check_size;
    return 1;
  }
  return 0;
}