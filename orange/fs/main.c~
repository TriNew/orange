#include "type.h"
#include "config.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"

#include "hd.h"

/*<ring 1> The main loop of TASK_FS*/

PUBLIC void task_fs()
{
	print1("Task FS begins.\n");
	/*open the device : hard disk
	*dd_map是驱动程序序号
	*ROOT_DEV包含驱动程序序号在dd_map中的位置(高8位,即主设备号)和次设备号(低8位)
	*MAJOR get high 8bits主设备号; MINOR get low 8bits 次设备号. 
	*定义ROOT_DEV为主分区2（hd2（2））的第一个逻辑分区（hd2a（32））
	*/






        init_fs();
	
	spin("FS");
}

PRIVATE void init_fs()
{
 	MESSAGE driver_msg;  
	driver_msg.type = DEV_OPEN;
	driver_msg.DEVICE = MINOR(ROOT_DEV);
	assert(dd_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
	
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg);

	mkfs();

}
PRIVATE void mkfs()
{
  MESSAGE driver_msg;
  int i,j ;
  int bits_per_sect = SECTOR_SIZE * 8;

  struct part_info geo;
  driver_msg.type = DEV_IOCTL;
  driver_msg.DEVICE = MINOR(ROOT_DEV);
  driver_msg.REQUEST = DIOCTL_GET_GEO;
  driver_msg.PROC_NR = TASK_FS;
  assert(dd_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
  send_recv(BOTH,dd_map[MAJOR(ROOT_DEV)].driver_nr,&driver_msg);
  print1("dev size:0x%x sectors\n" ,geo.size);

  /*super_block*/
  struct super_block sb;
  sb.magic  = MAGIC_V1;
  sb.nr_inodes = bits_per_sect;
  sb.nr_inode_sects = sb.nr_inodes * INODE_SIZE /SECTOR_SIZE;
  sb.nr_sects =geo.size;
  sb.nr_imap_sects =1;
  sb.nr_smap_sects = sb.nr_sects;
  sb.n_1st_sect = 1+1+ /*boot sector and super block*/
    sb.nr_imap_sects + sb.nr_smap_sects + sb.nr_inode_sects;
  sb.root_inode = ROOT_INODE;
  sb.inode_size = INODE_SIZE;
  struct inode x;
  sb.inode_isize_off = (int)&x.i_size -(int)&x;
  sb.inode_start_off = (int)&x.i_start_sect - (int)&x;
  sb.dir_ent_size = DIR_ENTRY_SIZE;
  struct dir_entry de;
  sb.dir_ent_inode_off = (int)&de.inode_nr -(int)&de;
  sb.dir_ent_fname_off = (int)&de.name - (int)&de;


  memset(fsbuf,0x90,SECTOR_SIZE);
  memcpy(fsbuf, &sb, SUPER_BLOCK_SIZE);

  print1("devbase:0x%x00 sb:0x%x00,imap:0x%x00,smap:0x%x00\n"
	 "            inodes:0x%x00, 1st_sector:0x%x00\n",
	 geo.base *2,
	 (geo.base + 1) *2 ,
	 (geo.base +1 +1 ) *2,
	 (geo.base +1 +1 +sb.nr_imap_sects)*2,
	 (geo.base+1 +1 + sb.nr_imap_sects + sb.nr_smap_sects)*2 ,
	 (geo.base +sb.n_1st_sect)*2);

  /*inode map*/
  memset(fsbuf,0 ,SECTOR_SIZE);
  for(i = 0 ; i < (NR_CONSOLES + 2); i++)
    fsbuf[0] |=( 1<< i) ;
  assert(fsbuf[0] == 0x1f);

  WR_SECT(ROOT_DEV, 2);

  /*sector map*/
  memset(fsbuf, 0, SECTOR_SIZE);
  int nr_sects = NR_DEFAULT_FILE_SECTS + 1;

  for(i = 0; i < nr_sects/8; i++)
    bsbuf[i] = 0xFF;

  for(j =0; j < nr_sects %8; j++)
    fsbuf[i] |=(i<<1) ;

  WR_SECT(ROOT_DEV, 2+sb.nr_imap_sects);

  memset(fsbuf, 0 , SECTOR_SIZE);
  for(i = 1 ; i<sb.nr_smap_sects; i++)
    WR_SECT(ROOT_DEV, 2+sb.nr_imap_sects + i);


  /*inodes*/
  memset(fsbuf, 0, SECTOR_SIZE);
  struct inode *pi = (struct inode *)fsbuf;
  pi->i_mode = I_DIRECTORY;
  pi->i_size = DIR_ENTRY_SIZE * 4; /* 4 files:  '.', 'dev_tty0', 'dev_tty1','dev_tty2'.'dev_tty2'*/
  pi->i_start_sect =  sb.n_1st_sect;
  pi->i_nr_sects = NR_DEFAULT_FILE_SECTS;

  for(i = 0; i < NR_CONSOLES; i++)
    {
      pi = (struct inode *)(fsbuf + (INODE_SIZE * (i+1)));
      pi->i_mode = I_CHAR_SPECIAL;
      pi->i_size = 0;
      pi->i_start_sect = MAKE_DEV(DEV_CHAR_TTY,i);
      pi->i_nr_sects = 0;
      
    }
  WR_SECT(ROOT_DEV, 2+sb.nr_imap_sects+sb.nr_smap_sects);

  /************'/'******/
  memset(fsbuf, 0 ,SECTOR_SIZE);
  struct dir_entry * pde = (struct dir_entry * )fsbuf;
  pde->inode_nr = 1;
  strcpy(pde->name, ".");

  /*device entry of dev_tty0~2*/
  for(i= 0; i < NR_CONSLODES; i++)
    {
      pde++;
      pde->inode_nr = i+2;
      sprintf(pde->name, "dev_tty%d", i);
      
    }
  WR_SECT(ROOT_DEV, sb.n_1st_sect);
  
      }
/*R/W a sector via message with the corresponding driver*/
PUBLIC int rw_sector(int io_tyoe , int dev, u64 pos, int bytes, int proc_nr, void *buf)
{
  MESSAGE driver_msg;

  driver_msg.type  = io_type;
  driver_msg.DEVICE = MINOR(dev);
  driver_msg.POSITION =pos;
  driver_msg.BUF = buf;
  driver_msg.CNT = bytes;
  driver_msg.PROC_NR = proc_nr;
  assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
  send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &driver_msg);

  return 0;//p36 2 
}
