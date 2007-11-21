/*

    File: fnctdsk.c

    Copyright (C) 1998-2005 Christophe GRENIER <grenier@cgsecurity.org>
  
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
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "types.h"
#include "common.h"
#include "fnctdsk.h"
#include "lang.h"
#include "testdisk.h"
#include "analyse.h"
#include "log.h"
#include "guid_cpy.h"

static unsigned int get_geometry_from_list_part_aux(const disk_t *disk_car, const list_part_t *list_part, const int verbose);

unsigned long int C_H_S2LBA(const disk_t *disk_car,const unsigned int C, const unsigned int H, const unsigned int S)
{ return ((unsigned long int)C*(disk_car->CHS.head+1)+H)*disk_car->CHS.sector+S-1;
}

uint64_t CHS2offset(const disk_t *disk_car,const CHS_t*CHS)
{ return (((uint64_t)CHS->cylinder*(disk_car->CHS.head+1)+CHS->head)*disk_car->CHS.sector+CHS->sector-1)*disk_car->sector_size;
}

uint64_t C_H_S2offset(const disk_t *disk_car,const unsigned int C, const unsigned int H, const unsigned int S)
{ return (((uint64_t)C*(disk_car->CHS.head+1)+H)*disk_car->CHS.sector+S-1)*disk_car->sector_size;
}

unsigned int offset2sector(const disk_t *disk_car, const uint64_t offset)
{ return ((offset/disk_car->sector_size)%disk_car->CHS.sector)+1; }

unsigned int offset2head(const disk_t *disk_car, const uint64_t offset)
{ return ((offset/disk_car->sector_size)/disk_car->CHS.sector)%(disk_car->CHS.head+1); }

unsigned int offset2cylinder(const disk_t *disk_car, const uint64_t offset)
{ return ((offset/disk_car->sector_size)/disk_car->CHS.sector)/(disk_car->CHS.head+1); }

void offset2CHS(const disk_t *disk_car,const uint64_t offset, CHS_t*CHS)
{
  uint64_t pos=offset/disk_car->sector_size;
  CHS->sector=(pos%disk_car->CHS.sector)+1;
  pos/=disk_car->CHS.sector;
  CHS->head=pos%(disk_car->CHS.head+1);
  CHS->cylinder=pos/(disk_car->CHS.head+1);
}

void dup_CHS(CHS_t * CHS_dst, const CHS_t * CHS_source)
{
  CHS_dst->cylinder=CHS_source->cylinder;
  CHS_dst->head=CHS_source->head;
  CHS_dst->sector=CHS_source->sector;
}

void dup_partition_t(partition_t *dst, const partition_t *src)
{
#if 0
  dst->part_offset=src->part_offset;
  dst->part_size=src->part_size;
  dst->boot_sector=src->boot_sector;
  dst->boot_sector_size=src->boot_sector_size;
  dst->blocksize=src->blocksize;
  dst->part_type_i386=src->part_type_i386;
  dst->part_type_sun=src->part_type_sun;
  dst->part_type_mac=src->part_type_mac;
  dst->part_type_xbox=src->part_type_xbox;
  dst->part_type_gpt=src->part_type_gpt;
  dst->upart_type=src->upart_type;
  dst->status=src->status;
  dst->order=src->order;
  dst->errcode=src->errcode;
  strncpy(dst->info,src->info,sizeof(dst->info));
  strncpy(dst->fsname,src->name,sizeof(dst->fsname));
  strncpy(dst->partname,src->name,sizeof(dst->partname));
  dst->arch=src->arch;
#else
  memcpy(dst, src, sizeof(*src));
#endif
}

list_disk_t *insert_new_disk(list_disk_t *list_disk, disk_t *disk_car)
{
  if(disk_car==NULL)
    return list_disk;
  {
    list_disk_t *cur;
    list_disk_t *prev=NULL;
    list_disk_t *new_disk;
    /* Add it at the end if it doesn't already exist */
    for(cur=list_disk;cur!=NULL;cur=cur->next)
    {
      if(cur->disk->device!=NULL && disk_car->device!=NULL)
      {
	if(strcmp(cur->disk->device,disk_car->device)==0)
	{
	  if(disk_car->clean!=NULL)
	    disk_car->clean(disk_car);
	  free(disk_car->device);
	  free(disk_car);
	  return list_disk;
	}
      }
      prev=cur;
    }
    new_disk=(list_disk_t *)MALLOC(sizeof(*new_disk));
    new_disk->disk=disk_car;
    if(prev!=NULL)
    {
      prev->next=new_disk;
    }
    new_disk->prev=prev;
    new_disk->next=NULL;
    return (list_disk!=NULL?list_disk:new_disk);
  }
}

list_part_t *insert_new_partition(list_part_t *list_part, partition_t *part, const int force_insert, int *insert_error)
{
  list_part_t *prev=NULL;
  list_part_t *next;
  *insert_error=0;
  for(next=list_part;;next=next->next)
  { /* prev new next */
    if((next==NULL)||
      (part->part_offset<next->part->part_offset) ||
      (part->part_offset==next->part->part_offset &&
       ((part->part_size<next->part->part_size) ||
	(part->part_size==next->part->part_size && (force_insert==0 || part->sb_offset < next->part->sb_offset)))))
    {
      if(force_insert==0 &&
	(next!=NULL) &&
	(next->part->part_offset==part->part_offset) &&
	(next->part->part_size==part->part_size) &&
	(next->part->part_type_i386==part->part_type_i386) &&
	(next->part->part_type_mac==part->part_type_mac) &&
	(next->part->part_type_sun==part->part_type_sun) &&
	(next->part->part_type_xbox==part->part_type_xbox) &&
	(next->part->upart_type==part->upart_type || part->upart_type==UP_UNK))
      { /*CGR 2004/05/31*/
	if(next->part->status==STATUS_DELETED)
	{
	  next->part->status=part->status;
	}
	*insert_error=1;
	return list_part;
      }
      { /* prev new_element next */
	list_part_t *new_element;
	new_element=element_new(part);
	new_element->next=next;
	new_element->prev=prev;
	if(next!=NULL)
	  next->prev=new_element;
	if(prev!=NULL)
	{
	  prev->next=new_element;
	  return list_part;
	}
	return new_element;
      }
    }
    prev=next;
  }
}

int delete_list_disk(list_disk_t *list_disk)
{
  list_disk_t *element_disk;
  int write_used=0;
  for(element_disk=list_disk;element_disk!=NULL;)
  {
    list_disk_t *element_disk_next=element_disk->next;
    write_used|=element_disk->disk->write_used;
    if(element_disk->disk->clean!=NULL)
      element_disk->disk->clean(element_disk->disk);
    if(element_disk->disk->device!=NULL)
      free(element_disk->disk->device);
    free(element_disk->disk);
    free(element_disk);
    element_disk=element_disk_next;
  }
  return write_used;
}

int check_list_part(list_part_t *list_part)
{
  list_part_t *prev=NULL;
  list_part_t *parts;
  if((list_part!=NULL) && (list_part->prev!=NULL))
  {
    log_critical("\ncheck_list_part error: list_part->prev!=NULL\n");
    exit(EXIT_FAILURE);
  }
  log_trace("check_list_part\n");
  for(parts=list_part;parts!=NULL;parts=parts->next)
  {
    log_info("%p %p %p\n",parts->prev, parts, parts->next);
    if(prev!=parts->prev)
    {
      log_critical("\ncheck_list_part error: prev!=parts->prev\n");
      exit(EXIT_FAILURE);
    }
    prev=parts;
  }
  if((prev!=NULL) && (prev->next!=NULL))
  {
    log_critical("\ncheck_list_part error: prev->next!=NULL\n");
    exit(EXIT_FAILURE);
  }
  return 0;
}

list_part_t *sort_partition_list(list_part_t *list_part)
{
  list_part_t *new_list_part=NULL;
  list_part_t *element;
  list_part_t *next;
#ifdef DEBUG
  check_list_part(list_part);
#endif
  for(element=list_part;element!=NULL;element=next)
  {
    int insert_error=0;
    next=element->next;
    new_list_part=insert_new_partition(new_list_part, element->part, 0, &insert_error);
    if(insert_error>0)
      free(element->part);
    free(element);
  }
#ifdef DEBUG
  check_list_part(new_list_part);
#endif
  return new_list_part;
}

list_part_t *gen_sorted_partition_list(const list_part_t *list_part)
{
  list_part_t *new_list_part=NULL;
  const list_part_t *element;
  for(element=list_part;element!=NULL;element=element->next)
  {
    int insert_error=0;
    if(element->part->status!=STATUS_DELETED)
      new_list_part=insert_new_partition(new_list_part, element->part, 1, &insert_error);
  }
  return new_list_part;
}

/* Delete the list and its content */
void part_free_list(list_part_t *list_part)
{
  list_part_t *element;
#ifdef DEBUG
  check_list_part(list_part);
#endif
  element=list_part;
  while(element!=NULL)
  {
    list_part_t *next=element->next;
    free(element->part);
    free(element);
    element=next;
  }
}

/* Free the list but not its content */
void part_free_list_only(list_part_t *list_part)
{
  list_part_t *element;
#ifdef DEBUG
  check_list_part(list_part);
#endif
  element=list_part;
  while(element!=NULL)
  {
    list_part_t *next=element->next;
    free(element);
    element=next;
  }
}

int is_part_overlapping(const list_part_t *list_part)
{
  const list_part_t *element;
  /* Test overlapping
     Must be space between a primary/logical partition and a logical partition for an extended
  */
  if(list_part==NULL)
    return 0;
  element=list_part;
  while(1)
  {
    const list_part_t *next=element->next;
    const partition_t *partition=element->part;
    if(next==NULL)
      return 0;
    if( (partition->part_offset + partition->part_size - 1 >= next->part->part_offset)		||
	((partition->status==STATUS_PRIM ||
	  partition->status==STATUS_PRIM_BOOT ||
	  partition->status==STATUS_LOG) &&
	 next->part->status==STATUS_LOG &&
	 partition->part_offset + partition->part_size - 1 + 1 >= next->part->part_offset))
      return 1;
    element=next;
  }
  return 0;
}

void  partition_reset(partition_t *partition, const arch_fnct_t *arch)
{
/* partition->lba=0; Don't reset lba, used by search_part */
  partition->part_size=(uint64_t)0;
  partition->sborg_offset=0;
  partition->sb_offset=0;
  partition->sb_size=0;
  partition->blocksize=0;
  partition->part_type_i386=P_NO_OS;
  partition->part_type_sun=PSUN_UNK;
  partition->part_type_mac=PMAC_UNK;
  partition->part_type_xbox=PXBOX_UNK;
  partition->part_type_gpt=GPT_ENT_TYPE_UNUSED;
  guid_cpy(&partition->part_uuid, &GPT_ENT_TYPE_UNUSED);
  partition->upart_type=UP_UNK;
  partition->status=STATUS_DELETED;
  partition->order=NO_ORDER;
  partition->errcode=BAD_NOERR;
  partition->fsname[0]='\0';
  partition->partname[0]='\0';
  partition->info[0]='\0';
  partition->arch=arch;
}

partition_t *partition_new(const arch_fnct_t *arch)
{
  partition_t *partition=(partition_t *)MALLOC(sizeof(*partition));
  partition_reset(partition, arch);
  return partition;
}

list_part_t *element_new(partition_t *part)
{
  list_part_t *new_element=(list_part_t*)MALLOC(sizeof(*new_element));
  new_element->part=part;
  new_element->prev=new_element->next=NULL;
  new_element->to_be_removed=0;
  return new_element;
}

static unsigned int get_geometry_from_list_part_aux(const disk_t *disk_car, const list_part_t *list_part, const int verbose)
{
  const list_part_t *element;
  unsigned int nbr=0;
  for(element=list_part;element!=NULL;element=element->next)
  {
    CHS_t start;
    CHS_t end;
    offset2CHS(disk_car,element->part->part_offset,&start);
    offset2CHS(disk_car,element->part->part_offset+element->part->part_size-1,&end);
    if(start.sector==1 && start.head<=1)
    {
      nbr++;
      if(end.head==disk_car->CHS.head)
      {
	nbr++;
	/* Doesn't check if end.sector==disk_car->CHS.sector */
      }
    }
  }
  if(nbr>0)
  {
    log_info("get_geometry_from_list_part_aux head=%u nbr=%u\n",disk_car->CHS.head+1,nbr);
    if(verbose>1)
    {
      for(element=list_part;element!=NULL;element=element->next)
      {
	CHS_t start;
	CHS_t end;
	offset2CHS(disk_car,element->part->part_offset,&start);
	offset2CHS(disk_car,element->part->part_offset+element->part->part_size-1,&end);
	if(start.sector==1 && start.head<=1 && end.head==disk_car->CHS.head)
	{
	  log_partition(disk_car,element->part);
	}
      }
    }
  }
  return nbr;
}

unsigned int get_geometry_from_list_part(const disk_t *disk_car, const list_part_t *list_part, const int verbose)
{
  const unsigned int head_list[]={8,16,32,64,128,240,255,0};
  unsigned int nbr_max;
  unsigned int nbr;
  unsigned int h_index=0;
  unsigned int head_max=disk_car->CHS.head;
  disk_t *new_disk_car=MALLOC(sizeof(*new_disk_car));
  memcpy(new_disk_car,disk_car,sizeof(*new_disk_car));
  nbr_max=get_geometry_from_list_part_aux(new_disk_car, list_part, verbose);
  for(h_index=0;head_list[h_index]!=0;h_index++)
  {
    new_disk_car->CHS.head=head_list[h_index]-1;
    nbr=get_geometry_from_list_part_aux(new_disk_car, list_part, verbose);
    if(nbr>=nbr_max)
    {
      nbr_max=nbr;
      head_max=new_disk_car->CHS.head;
    }
  }
  free(new_disk_car);
  return head_max;
}

const char *size_to_unit(uint64_t disk_size, char *buffer)
{
  if(disk_size<(uint64_t)10*1024)
    sprintf(buffer,"%u B", (unsigned)disk_size);
  else if(disk_size<(uint64_t)10*1024*1024)
    sprintf(buffer,"%u KB / %u KiB", (unsigned)(disk_size/1000), (unsigned)(disk_size/1024));
  else if(disk_size<(uint64_t)10*1024*1024*1024)
    sprintf(buffer,"%u MB / %u MiB", (unsigned)(disk_size/1000/1000), (unsigned)(disk_size/1024/1024));
  else if(disk_size<(uint64_t)10*1024*1024*1024*1024)
    sprintf(buffer,"%u GB / %u GiB", (unsigned)(disk_size/1000/1000/1000), (unsigned)(disk_size/1024/1024/1024));
  else
    sprintf(buffer,"%u TB / %u TiB", (unsigned)(disk_size/1000/1000/1000/1000), (unsigned)(disk_size/1024/1024/1024/1024));
  return buffer;
}