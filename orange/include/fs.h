#ifndef	_ORANGES_FS_H_
#define	_ORANGES_FS_H_
;
/**
 * @struct dev_drv_map fs.h "include/sys/fs.h"
 * @brief  The Device_nr.\ - Driver_nr.\ MAP.
 *
 *  \dot
 *  digraph DD_MAP {
 *    graph[rankdir=LR];
 *    node [shape=record, fontname=Helvetica];
 *    b [ label="Device Nr."];
 *    c [ label="Driver (the task)"];
 *    b -> c [ label="DD_MAP", fontcolor=blue, URL="\ref DD_MAP", arrowhead="open", style="dashed" ];
 *  }
 *  \enddot
 */
struct dev_drv_map {
	int driver_nr; /**< The proc nr.\ of the device driver. */
};
/*magic number of FS v1.0*/
#define MAGIC_V1 	0x111

/*
* @struct super_block fs.h "include/fs.h"
* @brief The 2nd sector of the FS
* Remmber to change SUPER_BLOCK_SIZE if the member are changed  
*/
struct super_block{
       u32 	magic;				/*magic number*/
	u32 	nr_inodes;
	u32 	nr_sects;
        u32 	nr_imap_sects;
	u32 	nr_smap_sects;
	u32  n_lst_sect;
  u32  nr_inode_sects;
  u32  root_inode;
  u32 inode_size ;
  u32 inode_isize_off ;
  u32 inode_start_off ;
  u32 dir_ent_size;
  u32 dir_ent_inode_off;
  u32 dir_ent_fname_off;

  int sb_dev;     //P356

};

/*@def SUPER_BLOCK_SIZE*/
#define SUPER_BLOCK_SIZE  56


/*@struct inode*/
struct inode {
  u32 i_mode;
  u32 i_size;
  u32 i_start_sect;
  u32 i_nr_sects;
  u8   _unused[16];

  /*the fllowing items are only present in memory*/
  int i_dev ;
  int i_cnt ;
  int i_num;
};

/*INODE_SIZE*/
#define INODE_SIZE 32
/* MAX_FILENAME_LEN*/
#define MAX_FILENAME_LEN 12
struct dir_entry{
  int inode_nr;
  char name[MAX_FILENAME_LEN];
};

/*DIR_ENTRY_SIZE*/
#define DIR_ENTRY_SIZE sizeof(struct dir_entry)


#endif
