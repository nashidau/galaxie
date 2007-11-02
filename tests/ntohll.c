#include <arpa/inet.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "../server.h"

uint64_t vals[] = {
	1,
	2,
	1000,
	2000,
	1234,
	12345,
	0xff,
	0xdeadbeef,
	0xfeedbeef,
	INT_MAX,
	UINT_MAX,
	0xbeafbeafbeafbeafULL,
	0xdeaddeaddeaddeadULL,
	0x0123456789abcdefULL,
	0x123456789abcdef0ULL,
};
#define N_VALS (sizeof(vals)/sizeof(vals[0]))


int
main(int argc, char **argv){
	int i;
	int64_t in;
	int64_t out;

	for (i = 0 ; i < N_VALS ; i ++){
		in = vals[i];
		out = ntohll(vals[i]);
		out = ntohll(out);
		if (in != out){
			printf("Mismatch: %llx vs %llx\n",in,out);
			exit(1);
		}
	}
	exit(0);
}

