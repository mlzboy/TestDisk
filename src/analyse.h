/*

    file: analyse.h

    Copyright (C) 1998-2004 Christophe GRENIER <grenier@cgsecurity.org>
  
    this software is free software; you can redistribute it and/or modify
    it under the terms of the gnu general public license as published by
    the free software foundation; either version 2 of the license, or
    (at your option) any later version.
  
    this program is distributed in the hope that it will be useful,
    but without any warranty; without even the implied warranty of
    merchantability or fitness for a particular purpose.  see the
    gnu general public license for more details.
  
    You should have received a copy of the GNU General Public License along
    with this program; if not, write the Free Software Foundation, Inc., 51
    Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */

int search_type_0(unsigned char *buffer, disk_t *disk_car,partition_t *partition,const int verbose, const int dump_ind);
int search_type_1(unsigned char *buffer, disk_t *disk_car,partition_t *partition,const int verbose, const int dump_ind);
int search_type_2(unsigned char *buffer, disk_t *disk_car,partition_t *partition,const int verbose, const int dump_ind);
int search_type_8(unsigned char *buffer, disk_t *disk_car,partition_t *partition,const int verbose, const int dump_ind);
int search_type_16(unsigned char *buffer, disk_t *disk_car,partition_t *partition,const int verbose, const int dump_ind);
int search_type_64(unsigned char *buffer, disk_t *disk_car,partition_t *partition,const int verbose, const int dump_ind);
int search_type_128(unsigned char *buffer, disk_t *disk_car,partition_t *partition,const int verbose, const int dump_ind);
int search_FAT_backup(unsigned char *buffer, disk_t *disk_car,partition_t *partition, const int verbose, const int dump_ind);
int search_HFS_backup(unsigned char *buffer, disk_t *disk_car,partition_t *partition, const int verbose, const int dump_ind);
int search_NTFS_backup(unsigned char *buffer, disk_t *disk_car,partition_t *partition, const int verbose, const int dump_ind);
list_part_t *search_superblock(disk_t *disk_car, const partition_t *partition, const int verbose, const int dump_ind, const int interface);