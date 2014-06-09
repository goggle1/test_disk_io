
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>       

#define PIECE_LEN	(256*1024)

int read_sequence(char* file_name)
{
	int ret = 0;
	
	int fd = -1;
	fd = ::open(file_name, O_RDONLY);
	fprintf(stdout, "open %s, fd %d\n", file_name, fd);
	
	u_int8_t data[PIECE_LEN];
	while(1)
	{
		ret = read(fd, data, PIECE_LEN);
		if(ret < PIECE_LEN)
		{
			break;
		}
	}

	fprintf(stdout, "close fd %d\n", fd);
	::close(fd);
	fd = -1;

	return 0;	
}

int read_random()
{
	return 0;
}

int test_case1()
{	
	char* file_names[] = 
	{
		"/home/guoqiang/code/0.0.2/MediaSvr/02A805D023585316849992F61018FD8A360239D6.dat",
		"/home/guoqiang/code/NewMediaServer/new_ms/MediaSvr/02A805D023585316849992F61018FD8A360239D6.dat",
		"/home/guoqiang/code/NewMediaServer/new_ms/MediaSvr/14C39AB98D3205AC196308FAA27B3485BCB7109C.dat",
		"/home/guoqiang/code/NewMediaServer/new_ms1/MediaSvr/02A805D023585316849992F61018FD8A360239D6.dat",
		"/home/guoqiang/code/NewMediaServer/new_ms1/MediaSvr/C102015F1FD646DC1534F8244C4C4A6A2AF0C425.dat",
		"/home/guoqiang/code/NewMediaServer/new_ms1/MediaSvr/14C39AB98D3205AC196308FAA27B3485BCB7109C.dat",
		"/home/guoqiang/code/NewMediaServer/xiongm/MediaSvr/02A805D023585316849992F61018FD8A360239D6.dat",
		"/home/guoqiang/code/NewMediaServer/xiongm/MediaSvr/C102015F1FD646DC1534F8244C4C4A6A2AF0C425.dat",
		"/home/guoqiang/code/NewMediaServer/xiongm/MediaSvr/14C39AB98D3205AC196308FAA27B3485BCB7109C.dat",
		"/home/html/02A805D023585316849992F61018FD8A360239D6.dat",
		"/home/html/C102015F1FD646DC1534F8244C4C4A6A2AF0C425.dat",
		"/home/html/B421F1690EDF4115D39A0531770FF1D46B57F9FC.dat",
	};

	int index = 0;
	for(index=0; index<sizeof(file_names)/sizeof(file_names[0]); index++)
	{
		read_sequence(file_names[index]);
	}

	return 0;
}

int main(int argc, char* argv[])
{
	int ret = 0;

	test_case1();

	return ret;
}

