/*

    File: ext2_dir.c

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

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_EXT2FS_EXT2_FS_H
#include "ext2fs/ext2_fs.h"
#endif
#ifdef HAVE_EXT2FS_EXT2FS_H
#include "ext2fs/ext2fs.h"
#endif

#include "types.h"
#include "common.h"
#include "intrf.h"
#include "dir.h"
#include "ext2_dir.h"
#include "ext2_inc.h"
#include "log.h"

#if defined(HAVE_LIBEXT2FS)
#define DIRENT_DELETED_FILE	4
/*
 * list directory
 */

#define LONG_OPT	0x0001

/*
 * I/O Manager routine prototypes
 */
static errcode_t my_open(const char *dev, int flags, io_channel *channel);
static errcode_t my_close(io_channel channel);
static errcode_t my_set_blksize(io_channel channel, int blksize);
static errcode_t my_read_blk(io_channel channel, unsigned long block, int count, void *buf);
static errcode_t my_write_blk(io_channel channel, unsigned long block, int count, const void *buf);
static errcode_t my_flush(io_channel channel);

static io_channel alloc_io_channel(disk_t *disk_car,my_data_t *my_data);
static void dir_partition_ext2_close(dir_data_t *dir_data);

static struct struct_io_manager my_struct_manager = {
        magic: EXT2_ET_MAGIC_IO_MANAGER,
        name: "TestDisk I/O Manager",
        open: my_open,
        close: my_close,
        set_blksize: my_set_blksize,
        read_blk: my_read_blk,
        write_blk: my_write_blk,
        flush: my_flush,
	write_byte: NULL,
#ifdef HAVE_STRUCT_STRUCT_IO_MANAGER_SET_OPTION
	set_option: NULL
#endif
};
static file_data_t *ext2_dir(disk_t *disk_car, const partition_t *partition, dir_data_t *dir_data, const unsigned long int cluster);

io_channel *shared_ioch=NULL;
io_manager my_io_manager = &my_struct_manager;
/*
 * Macro taken from unix_io.c
 * For checking structure magic numbers...
 */

#define EXT2_CHECK_MAGIC(struct, code) \
          if ((struct)->magic != (code)) return (code)

/*
 * Allocate libext2fs structures associated with I/O manager
 */
static io_channel alloc_io_channel(disk_t *disk_car,my_data_t *my_data)
{
  io_channel     ioch;
#ifdef DEBUG
  log_trace("alloc_io_channel start\n");
#endif
  ioch = (io_channel)MALLOC(sizeof(struct struct_io_channel));
  if (ioch==NULL)
    return NULL;
  memset(ioch, 0, sizeof(struct struct_io_channel));
  ioch->magic = EXT2_ET_MAGIC_IO_CHANNEL;
  ioch->manager = my_io_manager;
  ioch->name = (char *)MALLOC(strlen(my_data->partition->fsname)+1);
  if (ioch->name==NULL) {
	  free(ioch);
	  return NULL;
  }
  memcpy(ioch->name, my_data->partition->fsname,strlen(my_data->partition->fsname)+1);
  ioch->private_data = my_data;
  ioch->block_size = 1024; /* The smallest ext2fs block size */
  ioch->read_error = 0;
  ioch->write_error = 0;
#ifdef DEBUG
  log_trace("alloc_io_channel end\n");
#endif
  return ioch;
}

static errcode_t my_open(const char *dev, int flags, io_channel *channel)
{
  *channel = *shared_ioch;
#ifdef DEBUG
  log_trace("my_open %s done\n",dev);
#endif
  return 0;
}

static errcode_t my_close(io_channel channel)
{
  free(channel->private_data);
  free(channel->name);
  free(channel);
#ifdef DEBUG
  log_trace("my_close done\n");
#endif
  return 0;
}

static errcode_t my_set_blksize(io_channel channel, int blksize)
{
  channel->block_size = blksize;
#ifdef DEBUG
  log_trace("my_set_blksize done\n");
#endif
  return 0;
}

static errcode_t my_read_blk(io_channel channel, unsigned long block, int count, void *buf)
{
  size_t size;
  my_data_t *my_data=(my_data_t*)channel->private_data;
  EXT2_CHECK_MAGIC(channel, EXT2_ET_MAGIC_IO_CHANNEL);

  size = (count < 0) ? -count : count * channel->block_size;
#ifdef DEBUG
  log_trace("my_read_blk start size=%lu, offset=%lu name=%s\n",
      (long unsigned)size, (unsigned long)(block*channel->block_size), my_data->partition->fsname);
#endif
  if(my_data->disk_car->read(my_data->disk_car,size,buf,my_data->partition->part_offset+(uint64_t)block*channel->block_size)!=0)
    return 1;
#ifdef DEBUG
  log_trace("my_read_blk done\n");
#endif
  return 0;
}

static errcode_t my_write_blk(io_channel channel, unsigned long block, int count, const void *buf)
{
  my_data_t *my_data=(my_data_t*)channel;
  EXT2_CHECK_MAGIC(channel, EXT2_ET_MAGIC_IO_CHANNEL);
/* if(my_data->disk_car->write(my_data->disk_car,count*channel->block_size,buf,my_data->partition->part_offset+(uint64_t)block*channel->block_size))!=0) */
    return 1;
/* return 0; */
}

static errcode_t my_flush(io_channel channel)
{
  return 0;
}

static int list_dir_proc2(ext2_ino_t dir,
			 int    entry,
			 struct ext2_dir_entry *dirent,
			 int	offset,
			 int	blocksize,
			 char	*buf,
			 void	*private)
{
  struct ext2_inode	inode;
  ext2_ino_t		ino;
  unsigned int		thislen;
  struct ext2_dir_struct *ls = (struct ext2_dir_struct *) private;
  file_data_t *new_file=MALLOC(sizeof(*new_file));
  new_file->prev=ls->current_file;
  new_file->next=NULL;

  thislen = ((dirent->name_len & 0xFF) < EXT2_NAME_LEN) ?
    (dirent->name_len & 0xFF) : EXT2_NAME_LEN;
  if(thislen>DIR_NAME_LEN)
    thislen=DIR_NAME_LEN;
  memcpy(new_file->name, dirent->name, thislen);
  new_file->name[thislen] = '\0';
  ino = dirent->inode;
  if (ino) {
    errcode_t retval;
//    log_trace("ext2fs_read_inode(ino=%u)\n",ino);
    if ((retval=ext2fs_read_inode(ls->current_fs,ino, &inode))!=0)
    {
      log_error("ext2fs_read_inode(ino=%u) failed with error %ld.\n",(unsigned)ino, (long)retval);
      memset(&inode, 0, sizeof(struct ext2_inode));
    }
  } else {
    memset(&inode, 0, sizeof(struct ext2_inode));
  }
  new_file->filestat.st_dev=0;
  new_file->filestat.st_ino=ino;
  new_file->filestat.st_mode=inode.i_mode;
  new_file->filestat.st_nlink=inode.i_links_count;
  new_file->filestat.st_uid=inode.i_uid;
  new_file->filestat.st_gid=inode.i_gid;
  new_file->filestat.st_rdev=0;
  new_file->filestat.st_size=LINUX_S_ISDIR(inode.i_mode)?inode.i_size:
    inode.i_size| ((uint64_t)inode.i_size_high << 32);
  new_file->filestat.st_blksize=blocksize;
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
  new_file->filestat.st_blocks=inode.i_blocks;
#endif
  new_file->filestat.st_atime=inode.i_atime;
  new_file->filestat.st_mtime=inode.i_mtime;
  new_file->filestat.st_ctime=inode.i_ctime;
  if(ls->current_file!=NULL)
    ls->current_file->next=new_file;
  else
    ls->dir_list=new_file;
  ls->current_file=new_file;
  return 0;
}

static file_data_t *ext2_dir(disk_t *disk_car, const partition_t *partition, dir_data_t *dir_data, const unsigned long int cluster)
{
  errcode_t       retval;
  struct ext2_dir_struct *ls=(struct ext2_dir_struct*)dir_data->private_dir_data;
  ls->dir_list=NULL;
  ls->current_file=NULL;
  if((retval=ext2fs_dir_iterate2(ls->current_fs, cluster, ls->flags, 0, list_dir_proc2, ls))!=0)
  {
    log_error("ext2fs_dir_iterate failed with error %ld.\n",(long)retval);
  }
  return ls->dir_list;
}

static void dir_partition_ext2_close(dir_data_t *dir_data)
{
  struct ext2_dir_struct *ls=(struct ext2_dir_struct *)dir_data->private_dir_data;
  ext2fs_close (ls->current_fs);
  /* ext2fs_close call the close function that freed my_data */
  free(ls);
}
#endif

int dir_partition_ext2_init(disk_t *disk_car, const partition_t *partition, dir_data_t *dir_data, const int verbose)
{
#if defined(HAVE_LIBEXT2FS)
  struct ext2_dir_struct *ls=(struct ext2_dir_struct *)MALLOC(sizeof(*ls));
  io_channel ioch;
  my_data_t *my_data;
  ls->dir_list=NULL;
  ls->current_file=NULL;
  /*  ls->flags = DIRENT_FLAG_INCLUDE_EMPTY; */
  ls->flags = 0;
  my_data=MALLOC(sizeof(*my_data));
  my_data->partition=partition;
  my_data->disk_car=disk_car;
  ioch=alloc_io_channel(disk_car,my_data);
  shared_ioch=&ioch;
  /* An alternate superblock may be used if the calling function has set an IO redirection */
  if(ext2fs_open ("/dev/testdisk", 0, 0, 0, my_io_manager, &ls->current_fs)!=0)
  {
    free(ls);
    return -1;
  }
  strncpy(dir_data->current_directory,"/",sizeof(dir_data->current_directory));
  dir_data->current_inode=EXT2_ROOT_INO;
  dir_data->verbose=verbose;
  dir_data->get_dir=ext2_dir;
  dir_data->copy_file=NULL;
  dir_data->close=&dir_partition_ext2_close;
  dir_data->local_dir=NULL;
  dir_data->private_dir_data=ls;
  return 0;
#else
  return -2;
#endif
}

const char*td_ext2fs_version(void)
{
  const char *ext2fs_version="none";
#if defined(HAVE_LIBEXT2FS)
  ext2fs_get_library_version(&ext2fs_version,NULL);
#endif
  return ext2fs_version;
}