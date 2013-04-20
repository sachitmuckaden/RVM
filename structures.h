/*
 * structures.h
 *
 *  Created on: Apr 4, 2013
 *      Author: sachit
 */

#ifndef STRUCTURES_H_
#define STRUCTURES_H_

#define TYPE_RVM 1
#define TYPE_TRANS 2
#define TYPE_OPER 3

typedef struct queue_node
{
	struct queue_node *prev;
	struct queue_node *next;
	void* value;

}node;

typedef struct queue
{
	struct queue_node *back;
	struct queue_node *front;
	int count;
}queue_t;
typedef int rvm_t;
typedef int trans_t;
typedef struct rvm_cb
{
	rvm_t rvmid;
	char* dir;
	queue_t segmentlist;
	//queue_t translist;
}rvmcb;

typedef struct segment_cb
{
	node* qseg;
	char* segname;
	int segsize;
	int segid;
	int modify;
	void* segment;
	void* segmentcopy;
}segment_cb;

typedef struct operation
{
	void* base;
	char* segname;
	int offset;
	int size;
	void* copy;
	int id;
	int transid;
}operation;

typedef struct trans_cb
{
	trans_t transid;
	queue_t segmentlist;
	queue_t operationlist;
	int operationid;
	rvm_t rvmid;
}trans_cb;

#endif /* STRUCTURES_H_ */
