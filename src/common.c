/*

    File: common.c

    Copyright (C) 1998-2006 Christophe GRENIER <grenier@cgsecurity.org>
  
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
 
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <stdarg.h>
#include "types.h"
#include "common.h"
#include "lang.h"
#include <ctype.h>      /* tolower */
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#include "intrf.h"
#include "log.h"

static unsigned int up2power_aux(const unsigned int number);

void *MALLOC(size_t size)
{
  void *res;
  if(size<=0)
  {
    log_critical("Try to allocate 0 byte of memory\n");
    exit(EXIT_FAILURE);
  }
#if defined(HAVE_POSIX_MEMALIGN)
  if(size>=512)
  {
    /* Warning, memory leak checker must be posix_memalign aware, otherwise  *
     * reports may look strange. Aligned memory is required if the buffer is *
     * used for read/write operation with a file opened with O_DIRECT        */
    if(posix_memalign(&res,4096,size)==0)
    {
      memset(res,0,size);
      return res;
    }
  }
#endif
  if((res=malloc(size))==NULL)
  {
    log_critical("\nCan't allocate %lu bytes of memory.\n", (long unsigned)size);
    exit(EXIT_FAILURE);
  }
  memset(res,0,size);
  return res;
}

#ifndef HAVE_SNPRINTF
int snprintf(char *str, size_t size, const char *format, ...)
{
  int res;
  va_list ap;
  va_start(ap,format);
  res=vsnprintf(str, size, format, ap);
  va_end(ap);
  return res;
}
#endif

#ifndef HAVE_VSNPRINTF
int vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
  return vsprintf(str,format,ap);
}
#endif

#ifndef HAVE_STRNCASECMP
/** Case-insensitive, size-constrained, lexical comparison. 
 *
 * Compares a specified maximum number of characters of two strings for 
 * lexical equivalence in a case-insensitive manner.
 *
 * @param[in] s1 - The first string to be compared.
 * @param[in] s2 - The second string to be compared.
 * @param[in] len - The maximum number of characters to compare.
 * 
 * @return Zero if at least @p len characters of @p s1 are the same as
 *    the corresponding characters in @p s2 within the ASCII printable 
 *    range; a value less than zero if @p s1 is lexically less than
 *    @p s2; or a value greater than zero if @p s1 is lexically greater
 *    than @p s2.
 *
 * @internal
 */
int strncasecmp(const char * s1, const char * s2, size_t len)
{
  while (*s1 && (*s1 == *s2 || tolower(*s1) == tolower(*s2)))
  {
    len--;
    if (len == 0) 
      return 0;
    s1++;
    s2++;
  }
  return (int)*(const unsigned char *)s1 - (int)*(const unsigned char *)s2;
}
#endif

unsigned int up2power(const unsigned int number)
{
  /* log_trace("up2power(%u)=>%u\n",number, (1<<up2power_aux(number-1))); */
  if(number==0)
    return 1;
  return (1<<up2power_aux(number-1));
}

static unsigned int up2power_aux(const unsigned int number)
{
  if(number==0)
	return 0;
  else
	return(1+up2power_aux(number/2));
}

int check_volume_name(const char *name,const unsigned int max_size)
{
  unsigned int i;
  for(i=0;name[i]!='\0' && i<max_size;i++)
  {
    if((name[i]>=0x6 && name[i]<=0x1f)||
	(name[i]>=0x3A && name[i]<=0x3f))
      return 1;
    switch(name[i])
    {
      case 0x1:
      case 0x2:
      case 0x3:
      case 0x4:
      case 0x22:
      case 0x2A:
      case 0x2B:
      case 0x2C:
/*     case 0x2E: Pas sur */
      case 0x2F:
      case 0x5B:
      case 0x5C:
      case 0x5D:
      case 0x7C:
/*     case 'a' ... 'z': */
	return 1;
    }
  }
  return 0; /* Ok */
}

void set_part_name(partition_t *partition,const char *src,const int max_size)
{
  int i;
  for(i=0;(i<max_size) && (src[i]!=(char)0);i++)
    partition->fsname[i]=src[i];
  partition->fsname[i--]='\0';
}

static inline unsigned char convert_char(unsigned char car)
{
#ifdef DJGPP
  switch(car)
  {
    case ' ':
    case '*':
    case '+':
    case ',':
    case '.':
    case '=':
    case '[':
    case ']':
      return '_';
  }
  /* 'a' */
  if(car>=224 && car<=230)      
    return 'a';
  /* 'c' */
  if(car==231)
    return 'c';
  /* 'e' */
  if(car>=232 && car<=235)
    return 'e';
  /* 'i' */
  if(car>=236 && car<=239)
    return 'n';
  /* n */
  if(car==241)
    return 'n';
  /* 'o' */
  if((car>=242 && car<=246) || car==248)
    return 'o';
  /* 'u' */
  if(car>=249 && car<=252)
    return 'u';
  /* 'y' */
  if(car>=253)
    return 'y';
  /* Chars allowed under msdos, a-z is stored as upercase by the OS itself */
  if((car>='A' && car<='Z') || (car>='a' && car<='z')|| (car>='0' && car<='9'))
    return car;
  switch(car)
  {
    case '$':
    case '%':
    case '\'':
    case '`':
    case '-':
    case '@':
    case '{':
    case '}':
    case '~':
    case '!':
    case '#':
    case '(':
    case ')':
    case '&':
    case '_':
    case '^':
    case ' ':
    case '+':
    case ',':
    case '.':
    case '=':
    case '[':
    case ']':
    case '/':   /* without it, no subdirectory */
      return car;
  }
  if(car>=224)
    return car;
  return '_';
#endif
  return car;
}

unsigned int filename_convert(char *dst, const char*src, const unsigned int n)
{
  unsigned int i;
  for(i=0;i<n-1 && src[i]!='\0';i++)
  {
    if(i==1 && src[i]==':')
      dst[i]=src[i];
    else
      dst[i]=convert_char(src[i]);
  }
#if defined(DJGPP) || defined(__CYGWIN__) || defined(__MINGW32__)
  while(i>0 && (dst[i-1]==' '||dst[i-1]=='.'))
    i--;
  if(i==0 && (dst[i]==' '||dst[i]=='.'))
    dst[i++]='_';
#endif
  dst[i]='\0';
  return i;
}

void create_dir(const char *dir_name, const unsigned int is_dir_name)
{
  /* create all sub-directories */
  char *pos;
  char *path=strdup(dir_name);
  if(path==NULL)
    return;
  pos=path+1;
  do
  {
    strcpy(path,dir_name);
    pos=strchr(pos+1,'/');
    if(pos!=NULL)
      *pos='\0';
    if(pos!=NULL || is_dir_name!=0)
    {
#ifdef __CYGWIN__
      if(memcmp(&path[1],":/cygdrive",10)!=0)
#endif
#ifdef HAVE_MKDIR
#ifdef __MINGW32__
      mkdir(path);
#else
      mkdir(path, 0775);
#endif
#else
#warn You need a mkdir function!
#endif
    }
  } while(pos!=NULL);
  free(path);
}