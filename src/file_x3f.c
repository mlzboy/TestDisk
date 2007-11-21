/*

    File: file_x3f.c

    Copyright (C) 1998-2005,2007 Christophe GRENIER <grenier@cgsecurity.org>
  
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

static void register_header_check_x3f(file_stat_t *file_stat);
static int header_check_x3f(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new);

const file_hint_t file_hint_x3f= {
  .extension="x3f",
  .description="Sigma/Foveon X3 raw picture",
  .min_header_distance=0,
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .header_check=&header_check_x3f,
  .register_header_check=&register_header_check_x3f
};

static const unsigned char x3f_header[4]= {'F','O','V','b'};

static void register_header_check_x3f(file_stat_t *file_stat)
{
  register_header_check(0, x3f_header,sizeof(x3f_header), &header_check_x3f, file_stat);
}

static int header_check_x3f(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{ /* http://www.x3f.info/technotes/FileDocs/X3F_Format.pdf */
  if(memcmp(buffer,x3f_header,sizeof(x3f_header))==0)
  {
    reset_file_recovery(file_recovery_new);
    file_recovery_new->extension=file_hint_x3f.extension;
    return 1;
  }
  return 0;
}