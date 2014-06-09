
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>      
#include <libaio.h>

#include "aio.h"

#define PIECE_LEN	(256*1024)
#define AIO_MAXIO 	64

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

long rand_index(long radix)
{
	if(radix == 0)
	{
		return 0;
	}
	
	long rand_val = random();
	long ret_val = rand_val%radix;
	return ret_val;
}

int bitfield_find_unset(u_int8_t* bits, int piece_num, int start_index)
{
	int pos = 0;
	int index = 0;
	for(index=0; index<piece_num; index++)
	{
		pos = index+start_index;
		if(pos>=piece_num)
		{
			pos = pos%piece_num;
		}

		int byte_pos = pos/8;
		int bit_pos = pos%8;
		int bit_pos2 = 7 - bit_pos;
		if(!(bits[byte_pos] & (1<<bit_pos2)))
		{
			// find it
			return pos;
		}
	}

	return -1;
}

int bitfield_set_one(u_int8_t* bits, int piece_num, int index)
{
	int byte_pos = index/8;
	int bit_pos = index%8;
	int bit_pos2 = 7 - bit_pos;
	bits[byte_pos] = bits[byte_pos] | (1<<bit_pos2);

	return 0;
}

int read_random(char* file_name)
{
	int ret = 0;
	
	int fd = -1;
	fd = ::open(file_name, O_RDONLY);
	fprintf(stdout, "open %s, fd %d\n", file_name, fd);

	int64_t file_length = ::lseek(fd, 0, SEEK_END);
	int piece_num = (file_length + PIECE_LEN - 1)/ PIECE_LEN;
	int bitfield_size = (piece_num + 8 - 1)/8;
	u_int8_t* bitfield = (u_int8_t*)malloc(bitfield_size);
	memset(bitfield, 0, bitfield_size);

	fprintf(stdout, "file_length=%ld, piece_num=%d, bitfield_size=%d\n", file_length, piece_num, bitfield_size);

	u_int8_t data[PIECE_LEN];	
	while(1)
	{
		// read piece by random
		int piece_index = rand_index(piece_num);
		int piece_pos = bitfield_find_unset(bitfield, piece_num, piece_index);
		if(piece_pos == -1)
		{
			break;
		}

		//fprintf(stdout, "piece_pos=%d\n", piece_pos);

		lseek(fd, PIECE_LEN*piece_pos, SEEK_SET);
		read(fd, data, PIECE_LEN);

		bitfield_set_one(bitfield, piece_num, piece_pos);
	}
	
	close(fd);
	fd = -1;
	
	return 0;
}

int pread_random(char* file_name)
{
	int ret = 0;
	
	int fd = -1;
	fd = ::open(file_name, O_RDONLY);
	fprintf(stdout, "open %s, fd %d\n", file_name, fd);

	int64_t file_length = ::lseek(fd, 0, SEEK_END);
	int piece_num = (file_length + PIECE_LEN - 1)/ PIECE_LEN;
	int bitfield_size = (piece_num + 8 - 1)/8;
	u_int8_t* bitfield = (u_int8_t*)malloc(bitfield_size);
	memset(bitfield, 0, bitfield_size);

	fprintf(stdout, "file_length=%ld, piece_num=%d, bitfield_size=%d\n", file_length, piece_num, bitfield_size);

	u_int8_t data[PIECE_LEN];	
	while(1)
	{
		// read piece by random
		int piece_index = rand_index(piece_num);
		int piece_pos = bitfield_find_unset(bitfield, piece_num, piece_index);
		if(piece_pos == -1)
		{
			break;
		}

		//fprintf(stdout, "piece_pos=%d\n", piece_pos);
		pread(fd, data, PIECE_LEN, PIECE_LEN*piece_pos);

		bitfield_set_one(bitfield, piece_num, piece_pos);
	}
	
	close(fd);
	fd = -1;
	
	return 0;
}

int aio_read_random(char* file_name)
{
	int ret = 0;
	
	int fd = -1;
	fd = ::open(file_name, O_RDONLY);
	fprintf(stdout, "open %s, fd %d\n", file_name, fd);

	int64_t file_length = ::lseek(fd, 0, SEEK_END);
	int piece_num = (file_length + PIECE_LEN - 1)/ PIECE_LEN;
	int bitfield_size = (piece_num + 8 - 1)/8;
	u_int8_t* bitfield = (u_int8_t*)malloc(bitfield_size);
	memset(bitfield, 0, bitfield_size);

	fprintf(stdout, "file_length=%ld, piece_num=%d, bitfield_size=%d\n", file_length, piece_num, bitfield_size);

	io_context_t myctx;
	memset(&myctx, 0, sizeof(myctx));
    io_queue_init(AIO_MAXIO, &myctx);

	while(1)
	{
		// read piece by random
		int piece_index = rand_index(piece_num);
		int piece_pos = bitfield_find_unset(bitfield, piece_num, piece_index);
		if(piece_pos == -1)
		{
			break;
		}
		//printf("read piece=%d\n", piece_pos);
		struct iocb* io2 = (struct iocb*)malloc(sizeof(struct iocb));
		memset(io2, 0, sizeof(struct iocb));

		u_int8_t* buff = NULL;
		posix_memalign((void **)&buff, getpagesize(), PIECE_LEN);
		io_prep_pread(io2, fd, buff, PIECE_LEN, PIECE_LEN*piece_pos);
		io2->data = (void*)piece_pos;
		io_submit(myctx, 1, &io2);

		struct io_event events[AIO_MAXIO];
		int num = io_getevents(myctx, 0, AIO_MAXIO, events, NULL);
	    //printf("io_request completed %d\n", num);
		for(int i=0;i<num;i++)
		{
			struct iocb *objp = events[i].obj;
			int finish_piece_pos = (int)(long)objp->data;
			//printf("done_piece=%d, res=%ld, res2=%ld\n", finish_piece_pos, events[i].res, events[i].res2);
			//cb(myctx, io2, events[i].res, events[i].res2);
			bitfield_set_one(bitfield, piece_num, finish_piece_pos);
			free(objp->u.c.buf);
			free(objp);
		}
		
	}
	
	close(fd);
	fd = -1;
	
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
	for(index=0; index<(int)(sizeof(file_names)/sizeof(file_names[0])); index++)
	{
		read_sequence(file_names[index]);
	}

	return 0;
}

int test_case2()
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
	for(index=0; index<(int)(sizeof(file_names)/sizeof(file_names[0])); index++)
	{
		read_random(file_names[index]);
	}

	return 0;
}

int test_case3()
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
	for(index=0; index<(int)(sizeof(file_names)/sizeof(file_names[0])); index++)
	{
		pread_random(file_names[index]);
	}

	return 0;
}

int test_case4()
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
	for(index=0; index<(int)(sizeof(file_names)/sizeof(file_names[0])); index++)
	{
		aio_read_random(file_names[index]);
	}

	return 0;
}

int main(int argc, char* argv[])
{
	int ret = 0;

	//test_case1();
	//test_case2();
	test_case3();
	
	//test_aio("abc.txt", "def.txt");
	//test_case4();

	return ret;
}

