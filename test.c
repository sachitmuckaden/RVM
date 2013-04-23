/*
 * test.c
 *
 *  Created on: Apr 18, 2013
 *      Author: sachit
 */


#include <stdio.h>
#include "rvm.h"

void main(int argc, void** argv)
{
	rvm_t rvm = rvm_init("rvm_segments");
	char* buffer = rvm_map(rvm, "mysegment", 1000);
	char* buffer2 = rvm_map(rvm, "myothersegment", 1000);
	printf("Contents of buffer 2 %s\n", buffer2);
	char* segbase[2];
	segbase[0] = buffer;
	segbase[1] = buffer2;
	trans_t tid = rvm_begin_trans(rvm, 1, (void**)segbase);
	//trans_t tid2 = rvm_begin_trans(rvm, 2, (void**)segbase);
	rvm_about_to_modify(tid, buffer2, 0, 23);
	rvm_about_to_modify(tid, buffer2, 0, 23);
	memcpy(buffer2, "Hello HorldHello Horld", 22);
	printf("Contents of buffer 2 in memory are: %s\n", buffer2);
	rvm_abort_trans(tid);
	printf("Contents of buffer 2 after abort are %s\n", buffer2);
	printf("The transaction id is: %d\n", tid);
	rvm_unmap(rvm, buffer);
	rvm_destroy(rvm, "mysegment");
}



