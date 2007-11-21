/*

    File: filegen.h

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
#include "list.h"

#if defined(DJGPP)
#define PHOTOREC_MAX_FILE_SIZE (((uint64_t)1<<31)-1)
#else
#define PHOTOREC_MAX_FILE_SIZE (((uint64_t)1<<41)-1)
#endif
typedef struct file_hint_struct file_hint_t;
typedef struct file_recovery_struct file_recovery_t;
typedef struct file_enable_struct file_enable_t;
typedef struct file_stat_struct file_stat_t;
typedef struct alloc_data_struct alloc_data_t;
#if 0
typedef struct file_magic_struct file_magic_t;

struct file_magic_struct
{
  const unsigned char *value;
  const unsigned int length;
  const unsigned int offset;
};
#endif

struct file_enable_struct
{
  unsigned int enable;
  const file_hint_t *file_hint;
};

struct file_stat_struct
{
  unsigned int not_recovered;
  unsigned int recovered;
  const file_hint_t *file_hint;
};

struct file_recovery_struct
{
  file_stat_t *file_stat;
  FILE *handle;
  char filename[2048];
  uint64_t file_size;
  uint64_t file_size_on_disk;
  alloc_list_t location;
  const char *extension;
  uint64_t min_filesize;
  uint64_t offset_error;
  uint64_t calculated_file_size;
  int (*data_check)(const unsigned char*buffer, const unsigned int buffer_size, file_recovery_t *file_recovery);
  /* data_check returns 0: bad, 1: EOF not found, 2: EOF
     It can modify file_recovery->calculated_file_size, not must not modify file_recovery->file_size
  */
  void (*file_check)(file_recovery_t *file_recovery);
};

struct file_hint_struct
{
  const char *extension;
  const char *description;
  const uint64_t min_header_distance;
  /* don't try head_check if min_header_distance >0 and previous_header_distance <= min_header_distance */
  /* needed by tar header */
  const uint64_t max_filesize;
  const int recover;
  void (*register_header_check)(file_stat_t *file_stat);
  int (*header_check)(const unsigned char *buffer, const unsigned int buffer_size,
      const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new);
};

struct alloc_data_struct
{
  struct td_list_head list;
  uint64_t start;
  uint64_t end;
  file_stat_t *file_stat;
};

void file_search_footer(file_recovery_t *file_recovery, const unsigned char*footer, const unsigned int footer_length);
void file_search_lc_footer(file_recovery_t *file_recovery, const unsigned char*footer, const unsigned int footer_length);
alloc_data_t *del_search_space(alloc_data_t *list_search_space, uint64_t start, uint64_t end);
int data_check_size(const unsigned char *buffer, const unsigned int buffer_size, file_recovery_t *file_recovery);
void file_check_size(file_recovery_t *file_recovery);
void reset_file_recovery(file_recovery_t *file_recovery);
void register_header_check(const unsigned int offset, const unsigned char *value, const unsigned int length, 
  int (*header_check)(const unsigned char *buffer, const unsigned int buffer_size,
      const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new),
  file_stat_t *file_stat);

typedef struct file_check_struct file_check_t;

struct file_check_struct
{
  const unsigned char *value;
  unsigned int length;
  unsigned int offset;
  int (*header_check)(const unsigned char *buffer, const unsigned int buffer_size,
      const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new);
  file_stat_t *file_stat;
  file_check_t *next;
};
void free_header_check(void);
#define NL_BARENL       (1 << 0)
#define NL_CRLF         (1 << 1)
#define NL_BARECR       (1 << 2)
void file_allow_nl(file_recovery_t *file_recovery, const unsigned int nl_mode);