/*
 * queuelib.h
 *
 *  Created on: Apr 18, 2013
 *      Author: sachit
 */

#ifndef QUEUELIB_H_
#define QUEUELIB_H_

#define DEBUG 0
#include "structures.h"
#include <stdio.h>
#include <stdlib.h>

int initializeQueue(queue_t* queue_t)
{

	/*queue_t = (queue_t*)malloc(sizeof(queue_t));*/
	queue_t->front = NULL;
	queue_t->back = NULL;
	queue_t->count = 0;
	return 1;
}

node* addToQueue(queue_t* queue, void* val)
{
	node *qnode;
	qnode = (node*) malloc(sizeof(node));
	qnode->next = NULL;
	qnode->value = val;

    if(queue->back == NULL)
    {
    		if(queue->front != NULL)
    		{
    			return -1;
    		}	/*error since both front and back should be null together
    	/*adding the first node to the queue*/
		qnode->prev = NULL;
		queue->back = qnode;
		queue->front = qnode;
		queue->count++;
	}
    else
    {

		qnode->prev = NULL;
		qnode->next =queue->front;
		queue->front->prev = qnode;
		queue->front = qnode;
		queue->count++;
    }
    return qnode;
}


void removeFromQueue(queue_t* queue, node* qnode)
{
	if(queue->count==0)
	{
		fflush(stdout);
		exit(0);
		/*if(DEBUG)
			printf("Attempted to remove from empty queue\n");*/
		return;
	}
	if(queue->count == 1)
	{
		if(DEBUG)printf("Removing when queue size is 1\n");
		queue->front = queue->back = NULL;
		queue->count--;
		free(qnode->value);
		free(qnode);
		return;
	}
	if(qnode == queue->front)
	{
		queue->front = queue->front->next;
		queue->count--;
		free(qnode->value);
		free(qnode);
		return;
	}
	if(qnode == queue->back)
	{
		if(DEBUG)printf("Removing from back of queue\nQueue size: %d", queue->count);
		queue->back = queue->back->prev;
		queue->back->next = NULL;
		queue->count--;
		if(DEBUG)printf("Removed. Queue size %d\n", queue->count);
		free(qnode->value);
		free(qnode);
		return;
	}
	qnode->prev->next = qnode->next;
	qnode->next->prev = qnode->prev;
	queue->count--;
	free(qnode->value);
	free(qnode);
	return;
}

void* findInQueue(queue_t* queue, int id, int type)
{
	node* qnode = queue->front;
	if(queue->count==0)
	{
		printf("Attempted to find in empty queue\n");
		return NULL;
	}
	int found = 0;
	while(qnode!=NULL)
	{
		if(type == TYPE_RVM)
		{
			rvmcb* rvm = (rvmcb*) qnode->value;
			if(rvm->rvmid==id)
			{
				found = 1;
				return qnode->value;
			}
		}
		else if(type == TYPE_TRANS)
		{
			trans_cb* trans = (trans_cb*) qnode->value;
			if(trans->transid==id)
			{
				found = 1;
				return qnode->value;
			}
		}
		qnode = qnode->next;
	}
	if(!found)
	{
		printf("ID: %d could not be found in queue\n", id);
	}
	return NULL;
}

node* queue_remove(queue_t* q,  int thread)
{
	if(q->count == 0)
	{
		if(DEBUG)
			printf("Attempted to remove node from empty queue");
		return NULL;
	}

	node* qnode = q->front;
	trans_cb* trans = (trans_cb*)qnode->value;
	while(trans->transid!= thread)
	{
		qnode = qnode->next;
		trans = (trans_cb*)qnode->value;
	}
	trans_cb* fronttrans = (trans_cb*)q->front->value;
	trans_cb* backtrans = (trans_cb*)q->back->value;
	if(trans->transid == fronttrans->transid)
	{
		q->front = q->front->next;
	}
	if(trans->transid == backtrans->transid)
	{
		q->back = q->back->prev;
	}
	if(qnode->prev!=NULL)
		qnode->prev->next = qnode->next;
	if(qnode->next!=NULL)
		qnode->next->prev = qnode->prev;
	q->count--;

	return qnode;
}

node* returnFromQueue(queue_t* queue, node* node)
{
	if(queue->count==0)
	{
		fflush(stdout);
		/*if(DEBUG)
			printf("Attempted to remove from empty queue\n");*/
		return NULL;
	}
	if(queue->count == 1)
	{
		if(DEBUG)printf("Removing when queue size is 1\n");
		queue->front = queue->back = NULL;
		queue->count--;
		return node;
	}
	if(node == queue->front)
	{
		queue->front = queue->front->next;
		queue->count--;
		return node;
	}
	if(node == queue->back)
	{
		if(DEBUG)printf("Removing from back of queue\nQueue size: %d\n", queue->count);
		queue->back = queue->back->prev;
		queue->back->next = NULL;
		queue->count--;
		if(DEBUG)printf("Removed. Queue size %d\n", queue->count);
		return node;
	}
	node->prev->next = node->next;
	node->next->prev = node->prev;
	queue->count--;
	return node;
}

void destroyQueue(queue_t* queue, int type)
{
	node* qnode = queue->front;
	int i = 0;
	while(i<queue->count)
	{
		queue->front = queue->front->next;
		if(type==TYPE_OPER)
		{
			operation* op = (operation*) qnode->value;
			free(op->copy);
		}
		free(qnode);
		qnode = queue->front;
		i++;
	}
}



#endif /* QUEUELIB_H_ */
