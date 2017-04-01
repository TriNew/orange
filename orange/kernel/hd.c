#include "head.h"
#include "hd.h"
//#include "fs.h"
/*Main loop of HD driver*/
PRIVATE void	init_hd			();
PRIVATE void	hd_cmd_out		(struct hd_cmd* cmd);
PRIVATE int	waitfor			(int mask, int val, int timeout);
PRIVATE void	interrupt_wait		();
PRIVATE void	print_identify_info	(u16* hdinfo);
PRIVATE void	hd_identify		(int drive);
PRIVATE void  partition(int device ,int style);
PRIVATE void  print_hdinfo(struct hd_info *hdi);
PRIVATE void  get_part_table(int drive , int sect_nr, struct part_ent *entry);

PRIVATE void   hd_rdwt(MESSAGE *p);
PRIVATE void 	hd_open(int device);
PRIVATE void	hd_identify		(int drive);
PRIVATE void   hd_ioctl(MESSAGE *p);
PRIVATE void hd_close(int device);

PRIVATE	u8	hd_status;
PRIVATE	u8	hdbuf[SECTOR_SIZE * 2];
/*hd_info[0]包含一个硬盘上的所有主、扩展分区的信息*/
PRIVATE struct hd_info hd_info[1];

/*该宏定义根据dev的值计算选取IDE通道上的主盘还是副盘,
*NR_PRIM_PER_DRIVE一个disk中所有的主分区数=5
*从设备号从MINOR_hd1a=0x10开始,
*NR_SUB_PER_DRIVE是一个disk中的所有分区数=4*16
*本程序中仅使用分区号为32的分区
*
*/
#define DRV_OF_DEV(dev) (dev <= MAX_PRIM ? \
						dev / NR_PRIM_PER_DRIVE : \
						(dev - MINOR_hd1a) / NR_SUB_PER_DRIVE) /* 4*16 */

PUBLIC void task_hd()
{
	MESSAGE msg;
	
	init_hd();

	while(1)
	{
		send_recv(RECEIVE, ANY, &msg);

		int src = msg.source;

		switch(msg.type)
		{
			case DEV_OPEN:
				hd_open(msg.DEVICE);//msg.DEVICE即设备号，唯一标识硬盘中的唯一一个分区
				break;
			case DEV_CLOSE:
				hd_close(msg.DEVICE);
				break;
			case DEV_READ:
			case DEV_WRITE:
				hd_rdwt(&msg);

				break;
				//	case DEV_IOCTL:
				//	hd_ioctl(&msg);
				//	break;
			default:
				dump_msg("HD driver::unknown msg",&msg);
				spin("FS::main_loop (invalid msg.type)");
				break;
		}

		send_recv(SEND, src, &msg);
	}
}
/*init_hd
* <ring1> Check hard drive,set IRQ handler,enable IRQ and initialize data
*structures
*/
PRIVATE void init_hd()
{
	/*Get the number of drives from the BIOS data area*/
	int i;
	int infoSize= sizeof(hd_info) / sizeof(hd_info[0]);
	u8 * pNrDrives = (u8*)(0x475);
	print1("NrDrives:%d.\n",*pNrDrives);
	assert(*pNrDrives);

	for(i = 0; i < infoSize; i++)
		memset(&hd_info[i], 0, sizeof(hd_info[0]));
	hd_info[0].open_cnt = 0;
	
	put_irq_handler(AT_WINT_IRQ,hd_handler);
	enable_irq(CASCADE_IRQ);  //equ 2 链接主片的第三根线
	enable_irq(AT_WINT_IRQ);
}
/*<ring1> THis routine handles DEV_CLOSE msg
*/
PRIVATE void hd_close(int device)
{	/*选择需要操作的硬盘*/
	int drive = DRV_OF_DEV(device);
	assert(drive == 0);

	hd_info[drive].open_cnt--;
}

/*<ring1 > This routine handles DEV_READ and DEV_WRITE msg;
*/
PRIVATE void hd_rdwt(MESSAGE *p)
{/*选择需要操作的硬盘*/
	int drive = DRV_OF_DEV(p->DEVICE);
	/*文件相对于扇区起始地址的偏移(B)*/
	u64 pos = p->POSITION;
	assert((pos >> SECTOR_SIZE_SHIFT) < (1 << 31));
	
	/**
	*We only allow to R/W from a SECTOR boundary
	**/
	assert((pos & 0x1FF) == 0);
	/*pos/SECTOR_SIZE*/
	u32 sect_nr = (u32)(pos >> SECTOR_SIZE_SHIFT);
	int logidx = (p->DEVICE - MINOR_hd1a) % NR_SUB_PER_DRIVE;
	sect_nr += p->DEVICE < MAX_PRIM ?
							hd_info[drive].primary[p->DEVICE].base:
							hd_info[drive].logical[logidx].base;

	struct hd_cmd cmd;
	cmd.features 	= 0;
	cmd.count 		= (p->CNT + SECTOR  -1 )/ SECTOR_SIZE;
	cmd.lba_low 	= sect_nr & 0xFF;
	cmd.lba_mid  	= (sect_nr >> 8) & 0xFF;
	cmd.lba_high  	= (sect_nr >> 16) & 0xFF;
	cmd.device 		= MAKE_DEVICE_REG(1,drive, (sect_nr >> 24) & 0xF);
	cmd.command 	= (p->type == DEV_READ) ? ATA_READ : ATA_WRITE;
	hd_cmd_out(&cmd);
	/*剩余未读字节数*/
	int bytes_left = p->CNT;
	void * la = (void*)va2la(p->PROC_NR,p->BUF);

	while(bytes_left)
	{
		int bytes = min(SECTOR_SIZE, bytes_left);
		if(p->type == DEV_READ)
		{
			interrupt_wait();
			port_read(REG_DATA, hdbuf, SECTOR_SIZE);
			phys_copy(la, (void*)va2la(TASK_HD, hdbuf), bytes);

		}
		else 
		{
			if(!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT))
				panic("hd writing error");

			port_write(REG_DATA, la, bytes);
			interrupt_wait();
		}
		bytes_left -= SECTOR_SIZE;
		la += SECTOR_SIZE;
	}
}

/*******************************************************
* hd_ioctl
*<ring1> This routine handles the DEV_IOCTL msssage
*/
PRIVATE void hd_ioctl(MESSAGE *p)
{
  //	int device = p->DEVICE;
	/*选择需要操作的硬盘*/
  //	int drive = DRV_OF_DEV(device);

  //	struct hd_info *hdi = &hd_info[drive];
/*将起始扇区和扇区数目返回给调用者 */
/*	if(p->REQUEST == DIOCTL_GET_GEO)
	{
		void *dst =va2la(p->PROC_NR, p->BUF);
		void *src =va2la(TASK_HD,device < MAX_PRIM ?
								&hdi->primary[device] :
								&hdi->logical[
									(device - MINOR_hd1a) %
									NR_SUB_PER_DRIVE]);

		phys_copy(dst, src, sizeof(struct part_info));

	}
	else 
	{
		assert(0);
		}*/

}


/*hd_identify
* <ring1> Get the disk information
*/
PRIVATE void hd_identify(int drive)
{
	struct hd_cmd cmd;
	cmd.device = MAKE_DEVICE_REG(0, drive, 0);
	/*When an IDE drive is sent the IDENTIFY DRIVE (0xec) command, 
	it will return 256 words (512 bytes) of information.*/
	cmd.command = ATA_IDENTIFY;
	hd_cmd_out(&cmd);
	interrupt_wait();//blocked to wait for disk completed,handler inform it
	port_read(REG_DATA, hdbuf, SECTOR_SIZE); //0x1F0

	print_identify_info((u16*)hdbuf);

	u16* hdinfo = (u16*)hdbuf;
	hd_info[drive].primary[0].base = 0;
	/*Total NR of User Addressable Sectors*/
	hd_info[drive].primary[0].size = ((int)hdinfo[61]<<16) + hdinfo[60];

}
/*print_identify_info
* <ring1> Print the hdinfo retrieved via ATA_IDENTIFY command
*/
PRIVATE void print_identify_info(u16* hdinfo)
{
	int i,k;
	char s[64];

	struct iden_info_ascii{
		int idx;
		int len;
		char * desc;
	} iinfo[] = {{10,20,"HD SN"},/*Serial number in ASCII*/
				 {27,40,"HD Model"}/*Model number in ASCII*/};

	for(k=0;k<sizeof(iinfo)/sizeof(iinfo[0]);k++)
	{
		char *p = (char*)&hdinfo[iinfo[k].idx];
		for(i=0; i < iinfo[k].len/2; i++)
		{
			s[i*2+1] = *p++;
			s[i*2] = *p++;
		}
		s[i*2] = 0;
		print1("%s: %s\n",iinfo[k].desc, s);
	}

	int capabilities = hdinfo[49];
	print1("LBA supported: %s\n",(capabilities & 0x0200) ? "Yes":"No");

	int cmd_set_supported = hdinfo[83];
	print1("LBA48 supported: %s\n",
			(cmd_set_supported & 0x0400) ? "Yes" : "No");

	int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
	print1("HD size: %dMB\n",sectors * 512 / 1000000);

}
/* hd_cmd_out
*<ring1> Output a command to HD controller
*/
PRIVATE void hd_cmd_out(struct hd_cmd* cmd)
{
	/*For all commands the host must check if BSY=1 first
	* and should processed no further unless and until BSY=0
	*/
	if(!waitfor(STATUS_BSY, 0, HD_TIMEOUT))
		panic("hd error.");
	/*Active the Interrupt enable bit*/
	out_byte(REG_DEV_CTRL, 0);//0x3F6
	/*load required parameters in the Command Block Registers*/
	out_byte(REG_FEATURES, cmd->features);//0x1F1
	out_byte(REG_NSECTOR, cmd->count);
	out_byte(REG_LBA_LOW, cmd->lba_low);
	out_byte(REG_LBA_MID, cmd->lba_mid);
	out_byte(REG_LBA_HIGH, cmd->lba_high);
	out_byte(REG_DEVICE, cmd->device);
	/*Write the command code to the Command Register*/
	out_byte(REG_CMD, cmd->command); //0x1F7
}
/* interrupt_wait
* <ring1> Wait until a disk interrupt occurs
*/
PRIVATE void interrupt_wait()
{
	MESSAGE msg;
	send_recv(RECEIVE,INTERRUPT, &msg);
	
}
/*waitfor
*<ring 1> wait for a certain status
*/
PRIVATE int waitfor(int mask , int val ,int timeout )
{
	int t = get_ticks();
/*未超时，一直检测ststus regaster的BSY位是否位0,为0表示可写
*超时表示写端口失败
*/
	while(((get_ticks() - t ) * 1000/HZ) < timeout)
		if((in_byte(REG_STATUS) & mask) == val )
			return 1;

	return 0;
}
/* hd_handler
* <ring 0> Interrupt handler
*/
PUBLIC void hd_handler(int irq)
{
	/*
	* Interrupts are cleared when the host
	* - reads the Status Register
	* - issues a reset
	* - writes to the Command Register
	*/
	hd_status = in_byte(REG_STATUS);
	assert((hd_status & 0x01) == 0);
/*
*Inform a proc that an interrupt has occured.
 */
	inform_int(TASK_HD);
}
/*********************************************************
* <Ring1> This routine handles DEV_OPEN messages It identify the 
* drive of the given device and read the partition table of the 
*drive if it has not been read 
*/
PRIVATE void hd_open(int device)
{
	/*
	*drive指示操作一个IDE通道上的主盘还是副盘
	*选择需要操作的硬盘
	*/
	int drive = DRV_OF_DEV(device);

	assert(drive == 0);
/*get disk info through command ATA_IDENTIFY(0xec)*/
	hd_identify(drive);

	if(hd_info[drive].open_cnt++ == 0)
	{/*读主分区时，需要乘以5,确定是哪个disk的第一个主分区.P346*/
		partition(drive * (NR_PART_PER_DRIVE + 1), P_PRIMARY);
	
		print_hdinfo(&hd_info[drive]);
	}

}
/************************************************************
* get_part_table
*<ring1> Get a partition table of a drive 
*drive 驱动号 ,一个IDE通道可以连接2个盘
*sect_nr 起始扇区号
* part_ent 分区表结构
*/
PRIVATE void get_part_table(int drive , int sect_nr, struct part_ent *entry)
{
	int a=50000;
	struct hd_cmd cmd;
	cmd.features 	= 0;
	cmd.count 		= 1;
	cmd.lba_low 	= sect_nr & 0xFF;
	cmd.lba_mid 	= (sect_nr >> 8) & 0xFF;
	cmd.lba_high 	= (sect_nr >> 16)& 0xFF;
	cmd.device 		= MAKE_DEVICE_REG(1,/*LBA mode*/
									 drive,/*drive=0使用主盘drive=1使用副盘*/
									 (sect_nr >> 24) & 0xF);
	cmd.command 	=  ATA_READ;//0x20
	hd_cmd_out(&cmd);
	interrupt_wait();

	port_read(REG_DATA, hdbuf, SECTOR_SIZE);
	/*PARTITION_TABLE_OFFSET:0x1BE*/
	memcpy(entry,
		hdbuf + PARTITION_TABLE_OFFSET,
		sizeof(struct part_ent) * NR_PART_PER_DRIVE);
}
/*******************************************************
* partition 
* <Ring 1 > This routine is called when a device is opened 
* It reads the partition table(s) and fcills the hd_info struct
*device次设备号 P346
*/
	
PRIVATE void partition(int device ,int style)
{
	int i ;
	int drive = DRV_OF_DEV(device);
	/*主分区0包括主分区1，2,3,4.本质上主分区0是虚的，从磁盘开始直接是主分区1的信息*/
	struct hd_info * hdi = &hd_info[drive];
/*分区表.在get_part_tbl中填充,一个disk中，一个逻辑分区对应一项*/

	
	struct part_ent part_tbl[NR_SUB_PER_DRIVE];/*4*16*/
	if(style == P_PRIMARY)
	{				/*drive:起始扇区号:待填充分区表,一次读出全部的4个主分区表*/
		get_part_table(drive , drive , part_tbl);

		int nr_prim_parts = 0;
		
		for(i =0 ; i < NR_PART_PER_DRIVE; i++)
		{/*遍历主分区*/

			if(part_tbl[i].sys_id == NO_PART)//0
				continue;
			assert(part_tbl[i].sys_id != NO_PART);
			nr_prim_parts++;//有效主分区个数
			int dev_nr = i+1; /*1~4,设备号从1开始*/
			hdi->primary[dev_nr].base = part_tbl[i].start_sect;
			hdi->primary[dev_nr].size = part_tbl[i].nr_sects;
			/*id为5,扩展分区*/
			if(part_tbl[i].sys_id == EXT_PART)//extend_partition
				partition(device + dev_nr, P_EXTENDED);
		}
		assert(nr_prim_parts != 0);
		
	}
	else if(style == P_EXTENDED)
	{
		int j = device % NR_PRIM_PER_DRIVE ;/*1~4*/
		int ext_start_sect = hdi->primary[j].base;/*base addr of extend partition*/
		int s = ext_start_sect;
		int nr_lst_sub = (j - 1) * NR_SUB_PER_PART;/*0/16/32/48*/

		for(i =0; i < NR_SUB_PER_PART; i++)
		{	/*0~15/16~31/32~47/48~63*/
			int dev_nr = nr_lst_sub + i;

			get_part_table(drive, s, part_tbl);

			hdi->logical[dev_nr].base = s + part_tbl[0].start_sect;
			hdi->logical[dev_nr].size = part_tbl[0].nr_sects;

			s = ext_start_sect + part_tbl[1].start_sect;
			/*no more logical partitions*/

			if(part_tbl[1].sys_id == NO_PART)
				break;
		}	
	}
	else 
	{
		assert(0);
	}

}
/****************************************
*print_hdinfo
*<ring 1> Print disk info
*/
PRIVATE void print_hdinfo(struct hd_info *hdi)
{
	int i ;
	for(i = 0; i < NR_PART_PER_DRIVE +1 ; i++)
	{
		print1("%sPART_%d: base %d(0x%x), size %d(0x%x) (in sector)\n",
				i == 0 ? " " : "    ",
				i,
				hdi->primary[i].base,
				hdi->primary[i].base,
				hdi->primary[i].size,
				hdi->primary[i].size);
	}
	for(i = 0; i < NR_SUB_PER_DRIVE +1 ; i++)
	{
		if(hdi->logical[i].size == 0)
			continue;
		print1("       %d: base %d(0x%x), size %d(0x%x) (in sector)\n",
				i,
				hdi->logical[i].base,
				hdi->logical[i].base,
				hdi->logical[i].size,
				hdi->logical[i].size);
	}
}
int min(int s1,int s2)
{
	if(s1 < s2)
		return s1;
	else 
		return s2;
}
