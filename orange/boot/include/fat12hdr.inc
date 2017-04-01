;一个FAT项12位，每个FAT项对应数据区的一个簇，
;前两个FAT项不使用，故数据区的簇号从2开始
;header of fat12
	BS_OEMnAME 		DB 	'ForrestY'

	BPB_BytesPerSec DW 512
	BPB_SecPerClus 	DB 1
	BPB_RsvdSecCnt 	DW 1 		;现理解为引导扇区占用的扇区数
	BPB_NumFATs 	DB 2
	BPB_RootEntCnt 	DW 224 
	BPB_TotSec16 	DW 2880
	BPB_Media 		DB 0xF0
	BPB_FATsz16 	DW 9
	BPB_SecPerTrk 	DW 18
	BPB_NumHeads 	DW 2
	BPB_HiddSec 	DD 0
	BPB_TotSec32 	DD 0
	BS_DrvNum 		DB 0
	BS_Reservedl 	DB 0
	BS_BootSig 		DB 29h
	BS_VolID 		DD 0
	BS_VolLab		DB 'OrangeS0.02'
	BS_FileSysType	DB 'FAT12   '
;------------------------------------------------------
;BPB_FATsz16
;FAT表长度
	FATz 	equ 	9
;						根目录文件数最大值    每个扇区字节数   
;根目录占用的空间(扇区数)RootDirSectors = ((BPB_RootEntCnt*32)+(BPB_BytesPerSec-1))/BPB_BytesPerSec
;本例中使用小长度,size=14*512=7168byte
	RootDirSectors 	equ 14
;根目录第一个扇区号  =BPB_RsvdSecCnt + (BPB_NumFATs*FATz)	
	SectorNoOfRootDirectory 	equ 	19
;fat1的第一个扇区号	=BPB_RsvdSecCnt
	SectorNoOfFAT1 	equ 	1
;DeltaSectorNo = BPB_RsvdSecCnt + (BPB_NumFATs*FATz) -2
;文件开始的Sector号等于DirEntry中的Sector号+根目录占用的Sector数目 +DeltaSectorNo
	DeltaSectorNo 	equ 	17
