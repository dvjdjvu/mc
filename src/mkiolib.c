#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define CHECK_IRQ_FLAG       0x01
#define SET_OU_ADDR          0x02
#define SET_OUT_PACK         0x03
#define GET_IN_STAT          0x04
#define GET_IN_PACK          0x05
#define GET_CMD_ANS          0x0E
#define RELE_DETECT            0x0F

#define CHECK_READY_FLAG     0x11
#define POST_READY_DO_WRITE  0x12
#define POST_READY_DO_READ   0x13

// at start of working
// int  mkio_open ( const char *name, int chan_num )
int  mkio_open ( const char *name )
{
	int port_dev;
	port_dev = open(name,O_RDWR);
	if (port_dev==-1)   
	{  
		//printf("\n\nCan't open mkio-port!\n");
		return 0;
	}
	//ioctl(port_mkio,SET_CHAN_NUM,chan_num);
	return port_dev;
}

// sending to MKIO-port
// array - address of array of words to send
// adr, sadr, len - address, subaddress and number of len to send
// return value - ok or fail (no answer)
int  mkio_send ( int port_dev,  
                 int adr, int sadr, int len,
                 unsigned short *array )
{
	unsigned short send_arr[34];
	int            i, stat;
    if ((adr<0)||(adr>31))    return -1;
    if ((len<0)||(len>32))    return -1;
    if ((sadr<1)||(sadr>30))  return -1;
    if (len==32)  send_arr[0] = (adr<<11)|(sadr<<5);
    else          send_arr[0] = (adr<<11)|(sadr<<5)|len;
	
    for (i=0;i<len;i++)
		send_arr[i+1]=array[i];
		
    write(port_dev,send_arr,len+1);
    while(1)
    {  
		//wayt for READY here, not in kernel
		if (ioctl(port_dev,CHECK_READY_FLAG,NULL)) break;
    }
    stat = ioctl(port_dev,POST_READY_DO_WRITE,send_arr);
    if (stat&0x8000)   for (i=0;i<len+1;i++) send_arr[i]=0xffff;
    array[len+1]=send_arr[len+1];
	//printf("\nOS is %x ",send_arr[len+1]);
    return stat;
}

// receiving from MKIO-port
// array - address of array of words, received from MKIO
// adr, sadr, len - address, subaddress and number of words for exchange 
// return value - ok or fail (no answer)
int  mkio_recv ( int port_dev, 
				 int adr, int sadr, int len,
                 unsigned short *array )
{
unsigned short recv_arr[34];
int            i, stat;
    if ((adr<0)||(adr>31))    return -1;
    if ((len<0)||(len>32))    return -1;
    if ((sadr<1)||(sadr>30))  return -1;
    if (len==32)   recv_arr[0] = (adr<<11)|0x400|(sadr<<5);
    else           recv_arr[0] = (adr<<11)|0x400|(sadr<<5)|len;
    read(port_dev,recv_arr,len+1);
    while(1)
    {  
		//wayt for READY here, not in kernel
		if (ioctl(port_dev,CHECK_READY_FLAG,NULL)) break;
    }
    stat = ioctl(port_dev,POST_READY_DO_READ,recv_arr);
    if (stat&0x8000)  for (i=0;i<len+1;i++) recv_arr[i]=0xffff;
	//printf("\nStat %x",stat);
    for (i=0;i<len;i++)  array[i]=recv_arr[i];
	//printf("\nOS is %x ",recv_arr[len+1]);
    return stat;
}

// at close of working
int  mkio_close ( int port_dev )
{
    close(port_dev);
    return 0;
}

int  mkio_set_ou_addr ( int port_dev, int addr )
{
	if ((addr<0)||(addr>31))  return -1;
	ioctl(port_dev,SET_OU_ADDR,addr);
	while(1)
	{
		//wayt for irq after command here, not in kernel
		if (ioctl(port_dev,CHECK_IRQ_FLAG,NULL)) break;
	}
	ioctl(port_dev,GET_CMD_ANS,NULL);
	//printf("\nSET_OU_ADDR result %x",ioctl(port_dev,GET_CMD_ANS,NULL));
	return 0;
}
//---------------------------------------------------------------
// функция добавляет в очередь пакет на передачу с оконечного устройства контроллеру
// надо указать подадрес, длину пакета и указатель на его начало
int  mkio_add_ou_packet ( int port_dev, int subaddr, int length, 
     unsigned short *data )
{
	unsigned short tmp_pack[34], i;
    if ((subaddr<1)||(subaddr>30))  return -1;
    if ((length<0)||(length>32))    return -1;
    tmp_pack[0]=subaddr;    tmp_pack[1]=length;
    for (i=0;i<length;i++)  tmp_pack[i+2]=data[i];
    ioctl(port_dev,SET_OUT_PACK,tmp_pack);
    while(1)
    {
		//wayt for irq after command here, not in kernel
		if (ioctl(port_dev,CHECK_IRQ_FLAG,NULL)) break;
    }
    ioctl(port_dev,GET_CMD_ANS,NULL);
    //printf("\nSET_OUT_PACK result %x",ioctl(port_dev,GET_CMD_ANS,NULL));
    return 0;
}

int  mkio_get_in_pack ( int port_dev, int subaddr,  
     unsigned short *data )
{
	unsigned short tmp_pack[32], stat, i;
    if ((subaddr<1)||(subaddr>30))  return -1;
    tmp_pack[0]=subaddr;    
    stat = ioctl(port_dev,GET_IN_PACK,tmp_pack);
    for (i=0;i<stat;i++)  data[i]=tmp_pack[i];
    return stat;      
}    

int  mkio_get_in_stat ( int port_dev, int subaddr )
{
    if ((subaddr<1)||(subaddr>30))  return -1;
    return ioctl(port_dev,GET_IN_STAT,subaddr);
}

int rele_inp_detect ( int port_dev )
{
	return ioctl(port_dev,RELE_DETECT,0); 
}
