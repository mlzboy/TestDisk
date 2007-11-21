/*

    File: file_gif.c

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

static void register_header_check_gif(file_stat_t *file_stat);
static int header_check_gif(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new);
static void file_check_gif(file_recovery_t *file_recovery);

const file_hint_t file_hint_gif= {
  .extension="gif",
  .description="Graphic Interchange Format",
  .min_header_distance=0,
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .header_check=&header_check_gif,
  .register_header_check=&register_header_check_gif
};

static const unsigned char gif_header[6]=  { 'G','I','F','8','7','a'};
static const unsigned char gif_header2[6]= { 'G','I','F','8','9','a'};

static void register_header_check_gif(file_stat_t *file_stat)
{
  register_header_check(0, gif_header,sizeof(gif_header), &header_check_gif, file_stat);
  register_header_check(0, gif_header2,sizeof(gif_header2), &header_check_gif, file_stat);
}

static int header_check_gif(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  if(memcmp(buffer,gif_header,sizeof(gif_header))==0
      || memcmp(buffer,gif_header2,sizeof(gif_header2))==0)
  {
    reset_file_recovery(file_recovery_new);
    file_recovery_new->extension=file_hint_gif.extension;
    file_recovery_new->file_check=&file_check_gif;
    file_recovery_new->min_filesize=42;
    return 1;
  }
  return 0;
}

static void file_check_gif(file_recovery_t *file_recovery)
{
  const unsigned char gif_footer[2]= {0x00, 0x3b};
  file_search_footer(file_recovery, gif_footer,sizeof(gif_footer));
}