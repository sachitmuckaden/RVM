/*
 * rvm.h
 *
 *  Created on: Apr 4, 2013
 *      Author: sachit
 */

#ifndef RVM_H_
#define RVM_H_
#include "structures.h"

/*global variables*/
int rvmid_num;
int transid_num;
int segid_num;
queue_t rvmqueue;
queue_t trans_queue;
int rvmqueueinit;
/*end of global variables*/



rvm_t rvm_init(const char *directory);
void *rvm_map(rvm_t rvm, const char *segname, int size_to_create);/* - map a segment from disk into memory. If the segment does not already exist, then create it and give it size size_to_create. If the segment exists but is shorter than size_to_create, then extend it until it is long enough. It is an error to try to map the same segment twice.*/
void rvm_unmap(rvm_t rvm, void *segbase); /*unmap a segment from memory.*/
void rvm_destroy(rvm_t rvm, const char *segname); /*destroy a segment completely, erasing its backing store. This function should not be called on a segment that is currently mapped.*/
trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases); /*begin a transaction that will modify the segments listed in segbases. If any of the specified segments is already being modified by a transaction, then the call should fail and return (trans_t) -1. Note that trant_t needs to be able to be typecasted to an integer type.*/
void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size); /*declare that the library is about to modify a specified range of memory in the specified segment. The segment must be one of the segments specified in the call to rvm_begin_trans. Your library needs to ensure that the old memory has been saved, in case an abort is executed. It is legal call rvm_about_to_modify multiple times on the same memory area.*/
void rvm_commit_trans(trans_t tid); /*commit all changes that have been made within the specified transaction. When the call returns, then enough information should have been saved to disk so that, even if the program crashes, the changes will be seen by the program when it restarts.*/
void rvm_abort_trans(trans_t tid);/*undo all changes that have happened within the specified transaction.*/
void rvm_truncate_log(rvm_t rvm);/*play through any committed or aborted items in the log file(s) and shrink the log file(s) as much as possible.*/
#endif /* RVM_H_ */
