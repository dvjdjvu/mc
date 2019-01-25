#define MBYTE (1024*1024)
#define KBYTE (1024)

#include <openssl/md5.h>
#include <stdio.h>
#include <dirent.h>

#include <ustat.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <time.h>
#include <utime.h>

#define BUFSIZE 512
#define PERM_FILE (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

// Функция блочного(по BUFSIZE байт) копирования файла from в файл to.

int copy_file(const char *from, const char *to){
	int fromfd = -1, tofd = -1;
	ssize_t nread;
	char buf[BUFSIZE];

	chmod(from, 0755);

	if((fromfd = open(from, O_RDONLY)) < 0) return -1;
	//if((tofd   = open(to, O_WRONLY | O_CREAT | O_TRUNC | S_IRUSR | S_IWUSR), PERM_FILE) < 0) return -1;
	if((tofd   = creat(to, PERM_FILE)) < 0) return -1;

	while((nread = read(fromfd, buf, sizeof(buf))) > 0)
		if(write(tofd, buf, nread) != nread){
			chmod(to, 0755);
			return -1;
		}

	chmod(to, 0755);

	close(fromfd);
	close(tofd);
	
	return 0;
}

/* 	
	Использовать соответствующий временной штамп от файла from в качестве
	нового значения для изменяемого временного штампа файла to.
	(Изменение даты и времени создания файла)
*/

int touch(const char *from, const char *to){

	struct stat file_from;
	struct utimbuf file_to;

	if(stat(from, &file_from) != 0) return -1;

	file_to.actime  = file_from.st_atime;
	file_to.modtime = file_from.st_mtime;

	if(utime(to, &file_to) !=0) return -1;

	return 0;

}

#define MBYTE (1024*1024)
#define KBYTE (1024)

// Возвращает колличество свободной памяти доступоной rootу в файловой системе на которой находится каталог dir.

double free_memory(const char *dir){

	int mbyte = MBYTE;
	struct statvfs fs_pnki;
	size_t size_free;
	size_t size_all;

	if(statvfs(dir, &fs_pnki) >= 0){
		size_free = (geteuid() == 0 ) ? fs_pnki.f_frsize * fs_pnki.f_bfree : fs_pnki.f_frsize * fs_pnki.f_bavail;
		//size_t size_free = fs_pnki.f_frsize * fs_pnki.f_bavail;
		size_all  = fs_pnki.f_frsize * fs_pnki.f_blocks;

		return (size_free/mbyte);
	}else
		return 0;

}

// Сравниваем хэш функции файлов from & to, если они совпадают --- значит файл скопирован успешно.
int md5sum(const char *from, const char *to){
	// контекст хэша
	MD5_CTX from_c, to_c;
	unsigned char buf[BUFSIZE];
	unsigned char md_buf_from[MD5_DIGEST_LENGTH];
	unsigned char   md_buf_to[MD5_DIGEST_LENGTH];
	int i, fromfd = -1, tofd = -1;

	// В аргументах передаем имена файлов, для которых вычисляется хэш
	if((fromfd = open(from, O_RDONLY)) < 0) return -1;
	if((tofd   = open(to,   O_RDONLY)) < 0) return -1;

 	// Инициализируем контекст

	MD5_Init(&from_c);
	MD5_Init(&to_c);
	
	// Вычисляем хэш
	for(;;){
	    i = read(fromfd, buf, BUFSIZE);
	    if(i<=0) break;
	    MD5_Update(&from_c, buf, (unsigned long)i);
	}
	
	for(;;){
	    i = read(tofd, buf, BUFSIZE);
	    if(i<=0) break;
	    MD5_Update(&to_c, buf, (unsigned long)i);
	}
	
	// Помещаем вычисленный хэш в буфер md_buf_*
	MD5_Final(md_buf_from, &from_c);
	MD5_Final(md_buf_to,     &to_c);

	close(fromfd);
	close(tofd);
	
	for(i = 0; i < MD5_DIGEST_LENGTH; i++)
	    if(md_buf_from[i] != md_buf_to[i]) return 1;
	
	return 0;
}

int md5(const char *file, unsigned char *md_buf_file2){
	MD5_CTX md5_file;
	unsigned char buf[BUFSIZE];
	unsigned char    md_buf_file[MD5_DIGEST_LENGTH];
	//unsigned char   md_buf_file2[MD5_DIGEST_LENGTH*2];

	int i, ffile = -1;

	if((ffile = open(file, O_RDONLY)) < 0) return -1;

 	// Инициализируем контекст
	MD5_Init(&md5_file);

	// Вычисляем хэш
	for(;;){
	    i = read(ffile, buf, BUFSIZE);
	    if(i<=0) break;
	    MD5_Update(&md5_file, buf, (unsigned long)i);
	}

	MD5_Final(md_buf_file, &md5_file);

	close(ffile);

	for(i = 0; i < MD5_DIGEST_LENGTH; i++)
		sprintf(&md_buf_file2[i*2], "%.2x", md_buf_file[i]);

	md_buf_file2[MD5_DIGEST_LENGTH*2] = '\0';

	return 0;
}

int _getch(void){
	char c;
	switch(read(STDIN_FILENO, &c, 1)){
		case -1:
			return -1;
		case 1:
			break;
	}
	return c;
}