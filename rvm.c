/*
 * rvm.c
 *
 *  Created on: Apr 4, 2013
 *      Author: sachit
 */
#include "rvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "queuelib.h"

#define DEBUG 1
void emptylogfiles(char* dir);
void openlogfiles(char* dir, FILE** fp1, FILE** fp2);

rvmqueueinit = 0;

rvm_t rvm_init(const char *directory)
{
	struct stat sb;
	if(!rvmqueueinit)
	{
		initializeQueue(&rvmqueue);
		rvmid_num = 0;
		transid_num = 0;
		segid_num = 0;
		rvmqueueinit = 1;
	}
	char temp[100];
	sprintf(temp, "%s", directory);
	printf("Initializing in directory %s\n", temp);
	if(!(stat(directory, &sb) == 0 && S_ISDIR(sb.st_mode)))
	{
	    printf("ERROR: Directory does not exist!\n");
	    exit(0);
	}

	rvmcb* rvm = (rvmcb*) malloc(sizeof(rvmcb));
	rvm->rvmid = rvmid_num++;
	initializeQueue(&rvm->segmentlist);
	//initializeQueue(&rvm->translist);
	rvm->dir = (char*)malloc((strlen(directory))*sizeof(char));
	memset(rvm->dir, 0 , strlen(directory));
	strncpy(rvm->dir, directory, strlen(directory));
	if(rvmqueue.count!=0)
	{
		node* qnode = rvmqueue.front;
		int i = 0;
		while(i<rvmqueue.count)
		{
			rvmcb* rvmt = (rvmcb*)qnode->value;
			if(strcmp(rvmt->dir, directory)==0)
			{
				printf("Reinitializing RVM in backing store %s\n", directory);
				node* segnode = rvmt->segmentlist.front;
				int j = 0;
				while(j<rvmt->segmentlist.count)
				{
					segment_cb* segment = (segment_cb*) segnode->value;
					segnode = segnode->next;
					char name[100];
					sprintf(name, "%s", segment->segname);
					rvm_unmap(rvmt->rvmid, segment->segment);
					rvm_destroy(rvmt->rvmid, name);
					j++;
				}
			}
			qnode = qnode->next;
			i++;
		}
	}
	addToQueue(&rvmqueue, rvm);

	return rvm->rvmid;
}

void *rvm_map(rvm_t rvm, const char *segname, int size_to_create)
{
   FILE* fp;
   struct stat sb;
   int filesize;
   int exists;
   rvm_truncate_log(rvm);
   rvmcb* rvmc = (rvmcb*) findInQueue(&rvmqueue, rvm, TYPE_RVM);

   int pathlength = (strlen(segname)+strlen(rvmc->dir)+1);
   char path[100];
   sprintf(path, "%s/%s", rvmc->dir, segname);
   if((fp = fopen(path, "r"))==NULL)
   {
	   printf("Creating file\n");
	   fp = fopen(path, "a+");
   }
   /*stat(fp, &sb);
   printf("The size of the file is: %d", (int)sb.st_size);*/
   fseek(fp, 0L, SEEK_END);
   int sz = ftell(fp);
   printf("The size of the file is: %d\n", sz);
   fseek(fp, 0L, SEEK_SET);
   if (sz == 0) /*Segment did not exist on disk*/
   {
	   filesize = 0;
	   exists = 0;
   }
   else
   {
	   filesize =sz;
	   exists = 1;
   }

   char* memloc = malloc(size_to_create*sizeof(char));
   memset(memloc, 0, size_to_create*sizeof(char));
   if(exists)
   {
	   if( 1!=fread( memloc , filesize, 1 , fp) )
	   {
			 fclose(fp);
			 free(memloc);
			 fputs("entire read fails\n",stderr);
			 exit(1);
	   }
   }
   char* name = (char*)malloc(strlen(segname)+1);
   memset(name,0,strlen(segname)+1);
   /*printf("Length of name is %d\n", strlen(segname));*/
   strncpy(name, segname, strlen(segname));
   /*printf("The provided segname is %s\nThe name of the segment is %s\n", segname, name);*/

   segment_cb* segment = (segment_cb*) malloc(sizeof(segment_cb));

   segment->segsize = size_to_create;
   segment->qseg = addToQueue(&rvmc->segmentlist, (void*)segment);		//TODO
   segment->segid = segid_num++;
   segment->modify = 0;
   segment->segment = (void*)memloc;
   segment->segmentcopy = NULL;
   segment->segname = name;
   fprintf(fp, "%s", memloc);
   fclose(fp);
   return (void*) memloc;
}

void rvm_unmap(rvm_t rvm, void *segbase)
{
	rvmcb* rvmc = (rvmcb*) findInQueue(&rvmqueue, rvm, TYPE_RVM);
	rvm_truncate_log(rvm);
	int found = 0;
	if(rvmc->segmentlist.count == 0)
	{
		printf("ERROR: Segment is not mapped in memory\n");
		//emptylogfiles(rvmc->dir);
		exit(0);
	}
	node* segnode = rvmc->segmentlist.front;
	while(segnode!=NULL)
	{
		segment_cb* segment = (segment_cb*) segnode->value;
		if(segment->segment == segbase)
		{
			found = 1;
			segnode = returnFromQueue(&rvmc->segmentlist, segnode);
			break;
		}
		segnode = segnode->next;
	}
	segment_cb* seg = (segment_cb*)segnode->value;
	free(seg->segname);
	free(seg->segment);
	free(seg);
	free(segnode);
	if(!found)
	{
		printf("ERROR: Attempt to unmap segment that is not currently mapped\n");
		//emptylogfiles(rvmc->dir);
		exit(0);

	}
}

void rvm_destroy(rvm_t rvm, const char *segname)
{
	rvmcb* rvmc = (rvmcb*) findInQueue(&rvmqueue, rvm, TYPE_RVM);
	node* segnode = rvmc->segmentlist.front;
	while(segnode!=NULL)
	{
		segment_cb* segment = (segment_cb*) segnode->value;
		if(strcmp(segment->segname,segname)==0)
		{
			printf("ERROR: Attempt to destroy segment that is currently mapped in memory\n");
			//emptylogfiles(rvmc->dir);
			exit(0);
			return;
		}
		segnode = segnode->next;
	}
	int rc;
	int pathlength = (strlen(segname)+strlen(rvmc->dir)+strlen("/"));
	char path[100];
	sprintf(path, "%s/%s", rvmc->dir, segname);

	if((rc = unlink(path))<0)
	{
		printf("Error while deleting file. File does not exist: %s\n", path);
	}
}

trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases)
{
	/*Check if any of the segments are being modified by a different transaction*/
	rvmcb* rvmc = (rvmcb*) findInQueue(&rvmqueue, rvm, TYPE_RVM);
	int i =0;
	trans_cb* trans = (trans_cb*)malloc(sizeof(trans_cb));
	initializeQueue(&trans->segmentlist);
	initializeQueue(&trans->operationlist);
	while(i<numsegs)
	{
		void* seg = *segbases;
		node* segnode = rvmc->segmentlist.front;
		while(segnode!=NULL)
		{
			segment_cb* segment = (segment_cb*) segnode->value;
			if(segment->segment == seg)
			{
				if(segment->modify)
				{
					printf("ERROR: Segment: %s is being modified by another transaction\n", segment->segname);
					return -1;
				}

				segment->modify = 1;
				addToQueue(&trans->segmentlist, segment);
			}
			segnode = segnode->next;
		}
		i++;
		segbases++;
	}
	trans->transid = ++transid_num;
	trans->operationid = 0;
	trans->rvmid = rvm;
	addToQueue(&trans_queue, trans);
	return trans->transid;
}

void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size)
{

	/*Check if the specified range is valid*/
	trans_cb* trans = (trans_cb*)findInQueue(&trans_queue, tid, TYPE_TRANS);
	if(trans == NULL)
	{
		printf("ERROR: Invalid transaction id: %d\n", tid);
		exit(0);
		return;
	}
	node* segnode = trans->segmentlist.front;
	int found = 0;
	segment_cb* segment;
	while(segnode!=NULL)
	{
		segment = (segment_cb*) segnode->value;
		if(segment->segment == segbase)
		{
			found = 1;
			if(segment->segsize<(offset+size))
			{
				printf("ERROR: Specified range is invalid for segment %s\n", segment->segname);
				exit(0);
				return;
			}
			break;
		}
		segnode = segnode->next;
	}

	if(!found)
	{
		printf("ERROR: Segment was not specified in begin transaction\n");
		exit(0);
		return;
	}

	/*Check if the range is already being modified*/
	if(trans->operationlist.count!=0)
	{
		node* opnode = trans->operationlist.front;
		while(opnode!=NULL)
		{
			operation* op = (operation*)opnode->value;
			long opstart = op->base+op->offset;
			long opend = op->base+op->offset+op->size;
			long segstart = segbase+offset;
			long segend = segstart+size;
			if((segstart<opstart&&segend>opstart)||(segstart<opend&&segend>opend)||(segstart==opstart)||segend==opend)
			{
				printf("ERROR: Specified range is being modified by a transaction: %d\n", op->transid);
				exit(0);
				return;
			}
			opnode = opnode->next;
		}

	}
	/*Create operation object and add it to the list*/
	operation* oper = (operation*) malloc(sizeof(operation));
	oper->base = segbase;
	oper->offset = offset;
	oper->size = size;
	oper->id = trans->operationid++;
	oper->copy = (void*) malloc(size*sizeof(char));
	oper->transid = trans->transid;
	oper->segname = segment->segname;
	memcpy(oper->copy, (segbase+offset), size);
	addToQueue(&trans->operationlist, oper);

}

void rvm_commit_trans(trans_t tid)
{
	trans_cb* trans = (trans_cb*)findInQueue(&trans_queue, tid, TYPE_TRANS);
	rvmcb* rvmt = (rvmcb*)findInQueue(&rvmqueue, trans->rvmid, TYPE_RVM);


	/*Write all operations to log file*/
	node* opnode = trans->operationlist.front;
	int i = 0;
	FILE* fp1;
	FILE* fp2;
	openlogfiles(rvmt->dir, &fp1, &fp2);
	while(i<trans->operationlist.count)
	{
		operation* oper = (operation*)opnode->value;
		printf("Writing to meta log file %s %d %d\n", oper->segname, oper->offset, oper->size);
		fprintf(fp1, "%s %d %d\n", oper->segname, oper->offset, oper->size);
		void* buffer;
		buffer = malloc((oper->size+1));
		memset(buffer, 0, oper->size+1);
		memcpy(buffer, oper->base+oper->offset, oper->size);
		//memcpy(buffer+oper->size, '\0', 1);
		printf("Writing to redo log %s\n", buffer);
		fwrite(buffer, oper->size, 1, fp2);
		free(buffer);
		i++;
		opnode = opnode->next;
	}
	fclose(fp1);
	fclose(fp2);
	/*Remove transaction from queue and delete copies from memory*/
	node* removenode = queue_remove(&trans_queue, tid);
	node* transsegnode = trans->segmentlist.front;
	int k =0;
	while(k<trans->segmentlist.count)
	{
		segment_cb* seg = (segment_cb*)transsegnode->value;
		seg->modify = 0;
		k++;
		transsegnode = transsegnode->next;
	}
	/*destroy operationslist*/
	destroyQueue(&trans->operationlist, TYPE_OPER);
	destroyQueue(&trans->segmentlist, 0);
	free(trans);
}

void rvm_abort_trans(trans_t tid)
{
	trans_cb* trans = (trans_cb*)findInQueue(&trans_queue, tid, TYPE_TRANS);
	int i =0;
	node* opnode = trans->operationlist.front;
	while(i<trans->operationlist.count)
	{
		operation* op = (operation*)opnode->value;
		memcpy((op->base+op->offset), op->copy, op->size);
		i++;
		opnode = opnode->next;
	}
	int k =0;
	node* removenode = queue_remove(&trans_queue, tid);
	node* transsegnode = trans->segmentlist.front;
	while(k<trans->segmentlist.count)
	{
			segment_cb* seg = (segment_cb*)transsegnode->value;
			seg->modify = 0;
			k++;
			transsegnode = transsegnode->next;
	}
	destroyQueue(&trans->operationlist, TYPE_OPER);
	destroyQueue(&trans->segmentlist, 0);
	free(trans);
}

void rvm_truncate_log(rvm_t rvm)
{
	rvmcb* rvmt = (rvmcb*)findInQueue(&rvmqueue, rvm, TYPE_RVM);
	FILE* fp1, fp2;
	openlogfiles(rvmt->dir, &fp1, &fp2);
	char * line = NULL;
	size_t len = 0;
    ssize_t read;
    int logfilepos = 0;
    while ((read = getline(&line, &len, fp1)) != -1) {
    	/*printf("Retrieved line of length %zu :\n", read);
    	printf("%s", line);*/
    	int j =0;
    	char segname[20];
    	char temp1[20];
    	char temp2[20];
    	int offset;
    	int size;
    	char temp[20];
    	int count = 0;
    	if(sscanf(line, "%[^ ] %[^ ] %[^ ]", segname, temp1, temp2)<0)
    	{
    		printf("Error scanning line\n");
    	}

    	offset = atoi(temp1);
    	size = atoi(temp2);
    	printf("%s %d %d\n", segname, offset, size);
    	void* buffer, *writebuffer;
    	buffer = (char*)malloc((size+1));
    	memset(buffer, 0, size+1);
    	/*writebuffer = malloc(size*sizeof(char));*/
    	char fullpath[100], fullpathseg[100];
    	sprintf(fullpath, "%s/redolog.log", rvmt->dir);
    	sprintf(fullpathseg, "%s/%s", rvmt->dir, segname);
    	printf("The segment opened for writing is %s\n", fullpathseg);
    	FILE* fp = fopen(fullpath, "r");
    	FILE* fpseg = fopen(fullpathseg, "rb+");
    	if(fpseg==NULL)
    	{
    		printf("Error opening file %s\n", strerror(errno));
    		break;
    	}
    	fseek(fp, logfilepos, SEEK_SET);
    	fseek(fpseg, offset, SEEK_SET);
    	fread(buffer, size, 1, fp);
    	printf("Contents of the redo log are %s\n", buffer);
    	fwrite(buffer, size, 1, fpseg);
    	//writebuffer = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fdseg, offset);
    	logfilepos = logfilepos + size;
    	//memcpy(writebuffer, buffer, size);
    	//munmap(buffer, size);
    	//munmap(writebuffer, size);
    	fclose(fp);
    	fclose(fpseg);
    	free(buffer);

    }
    emptylogfiles(rvmt->dir);
}

void openlogfiles(char* dir, FILE** fp1, FILE** fp2)
{
	char metalogfile[20] = "metadata.log";
	char logfile[20] = "redolog.log";
	int pathlength = (strlen(metalogfile)+strlen(dir)+strlen("/"));
	char path[100];
	sprintf(path, "%s/%s", dir, metalogfile);
	*fp1 = fopen(path, "a+");
	sprintf(path, "%s/%s", dir,logfile);
	*fp2 = fopen(path, "a+");
}

void emptylogfiles(char* dir)
{
	FILE* fp1;
	FILE* fp2;
	char metalogfile[20] = "metadata.log";
	char logfile[20] = "redolog.log";
	int pathlength = (strlen(metalogfile)+strlen(dir)+strlen("/"));
	char path[100];
	sprintf(path, "%s/%s", dir, metalogfile);
	fp1 = fopen(path, "w+");
	fclose(fp1);
	pathlength = (strlen(logfile)+strlen(dir)+strlen("/"));
	sprintf(path, "%s/%s", dir, logfile);
	fp2 = fopen(path, "w+");
	fclose(fp2);
}
