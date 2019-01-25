#ifndef __MKIOLIB_H
#define __MKIOLIB_H

// some functions
//int   mkio_open         ( const char *, int );
int   mkio_open         ( const char * );
int   mkio_send         ( int, int, int, int, unsigned short * );
int   mkio_recv         ( int, int, int, int, unsigned short * );
int   mkio_close        ( int );
int   mkio_set_ou_addr  ( int, int );
int   mkio_add_ou_pack  ( int, int, int, unsigned short * );
int   rele_inp_detect   ( int );
int   mkio_get_in_pack  ( int, int, unsigned short * );
int   mkio_get_in_stat  ( int, int );

#endif /* __MKIOLIB_H */

/*
	if (dev_ou0=mkio_open("/dev/manchester0"))
	{
		if (dev_ou1=mkio_open("/dev/manchester1"))
		{ 
			if (rele_inp_detect(dev_ou0))
				AdrOU=4;
			else
				AdrOU=2;
		}
	}
*/
