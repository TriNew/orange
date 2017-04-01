#include "head.h"

PRIVATE char* i2a(int val, int base, char **ps)
{
	int m = val % base;
	int q = val / base;
	if(q)
	{
		i2a(q, base, ps);
	}
	*(*ps)++ = (m < 10) ? (m + '0') : (m - 10 + 'A');

	return *ps; 
}



/*---------------------------------------
 printf(fmt,i,j);
        push    j
        push    i
        push    fmt
        call    printf
        add     esp, 3 * 4
----------------------------------------*/
int printf(const char *fmt,...)
{
	int i;
	char buf[256];

	va_list arg = (va_list)((char*)(&fmt)+4);
	i = vsprintf(buf, fmt, arg);
	//write(buf, i);
	buf[i]=0;
	printx(buf);
	return i;
}
int print1(const char *fmt,...)
{
	int i;
	char buf[256];

	va_list arg = (va_list)((char*)(&fmt)+4);
	i = vsprintf(buf, fmt, arg);
	//write(buf, i);
	buf[i] = 0;
	printx(buf);
	return i;
}
PUBLIC int vsprintf(char *buf,const char *fmt,va_list args)
{
	char* 	p;/*p= buf*/
	char 	inner_buf[STR_DEFAULT_LEN];/*用于将后面变量展开的暂存空间*/
	char 	cs;  /*cs为指定当实际输出宽度小于指定输出宽度时，指定输入空格还是字符0*/
	int 	align_nr;/*align_nr为指定的输出宽度*/

	va_list p_next_arg = args;
	int m; /*存储p_next_arg指向的整数*/

	for(p=buf; *fmt;fmt++)
	{
		if(*fmt != '%')
		{
			*p++ = *fmt;
			continue;
		}
		else {
			align_nr = 0;
		}


		fmt++;/*处理%后紧挨的字符*/
		if(*fmt == '%')
		{/*两个%，简单复制一个*/
			*p++ = *fmt;
			continue;
		}
		else if (*fmt == '0')
		{/*“%0"形式,若需要，指定填充0,否则填充空格*/
			cs = '0';
			fmt++;
		}
		else {cs = ' ';}
		/*%+数字 形式*/
		while(((unsigned char)(*fmt) >= '0') && ((unsigned char)(*fmt) <= '9') )
		{/*字符”12345678“在内存中低地址--》高地址‘12’‘34’‘56’‘78’*/
			align_nr *= 10;
			align_nr += *fmt - '0';
			fmt++;
		}

		char *q = inner_buf ;
		memset(q, 0, sizeof(inner_buf));

		switch(*fmt)
		{
		  case 'x':
			m = *((int*)p_next_arg);
			i2a(m, 16, &q);/*16jinzhi*/
			p_next_arg += 4;
		    break;
		  case 's':
		  	/* while(*p_next_arg != '\0')
		  	 {  ERROR!!!!!!!!!!!!!!!!!!!!!
		  	 	*p++ = *(char*)p_next_arg++ ;
		  	 }*/
		  	 	strcpy(q, (*((char**)p_next_arg)));
		  	 	q += strlen(*((char**)p_next_arg));
		  	 	p_next_arg += 4;
		    break;
		  case 'c':
		   	*q++ = *((char*)p_next_arg) ;
		   	p_next_arg += 4;

		   	break;
		  case 'd':
		  	m = *((int*)p_next_arg);
		  	if(m<0)
		  	{
		  		m = m * (-1);
		  		*q++ ='-';
		  	}
		  	i2a(m, 10, &q);
		  	p_next_arg += 4;
		  default:
		  	break;
		}
/*
align_nr为指定的输出宽度
%0nx：若x指代的变量的宽度小于n，前面补0
%nx：若x指代的变量的宽度小于n，前面补空格
*/
		int k;
		for(k = 0; 
			k < ((align_nr > strlen(inner_buf)) ? (align_nr - strlen(inner_buf)) : 0);
			k++)
		{
			*p++ = cs;
		}
		q = inner_buf;
		while(*q)
		{
			*p++ = *q++;
		}
	}
	*p = 0;

	return (p-buf);
}


int sprintf(char *buf, const char *fmt,...)
{
	//int i;
	//char buf[256];

	va_list arg = (va_list)((char*)(&fmt)+4);
	//i = vsprintf(buf, fmt, arg);
	//write(buf, i);
	return vsprintf(buf, fmt, arg);
	//return i;
}
