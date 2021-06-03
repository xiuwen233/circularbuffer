/*
 * mirrorringbuffer.c
 *
 *  Created on: 2013-7-17
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "mirrorringbuffer.h"

pthread_mutex_t work_mutex;

int cbInit(CircularBuffer *cb, unsigned int size)
{
	cb->size = size;
	cb->read_start = 0;
	cb->read_start_tmp = 0;
	cb->write_end = 0;
	cb->write_end_tmp = 0;
	cb->w_msb = WRITE_ONLY;
	cb->data = (unsigned char *) malloc(size);
	if(NULL == cb->data)
	{
		printf("malloc cbInit  error \n");
		return -1;
	}
	memset(cb->data,0,size);
	int ret = pthread_mutex_init(&work_mutex, NULL);
	if(ret != 0) {
	   printf("mutex init failure \n");
	   return -1;
	}
	return 0;
	
}

int cbIsFull(CircularBuffer *cb)
{
	int ret = 0;
	pthread_mutex_lock(&work_mutex);
	ret = cb->write_end == cb->read_start && cb->w_msb ==READ_ONLY;
	pthread_mutex_unlock(&work_mutex);
	return ret;
}

int cbIsEmpty(CircularBuffer *cb)
{
	int ret = 0;
	pthread_mutex_lock(&work_mutex);
	ret = cb->write_end == cb->read_start && cb->write_end == WRITE_ONLY;
	pthread_mutex_unlock(&work_mutex);
	return ret;
}




void cb_INwrite(CircularBuffer *cb, unsigned char *elem, unsigned int length)
{

	if(cb->write_end_tmp + length > cb->size)
	{
		memcpy(cb->data+cb->write_end_tmp,elem,cb->size-cb->write_end_tmp);
		memcpy(cb->data,elem+cb->size-cb->write_end_tmp,length -cb->size+cb->write_end_tmp  );
		cb->write_end_tmp = length +cb->write_end_tmp-cb->size;	
	}
	else
	{
		memcpy(cb->data+cb->write_end_tmp,elem,length);
		if(cb->write_end_tmp + length == cb->size)
		{
			cb->write_end_tmp = 0; //boundary
		}
		else
		{
			cb->write_end_tmp = cb->write_end_tmp + length;
		}
	}
}


int cbWrite(CircularBuffer *cb, unsigned char *elem, unsigned int length)
{

    char len_str[10] ;
	char head_str[20];
	unsigned int sum_length = 0;
	unsigned int length_len = 0;
	unsigned int head_len = 0;
	
	memset(len_str,0,sizeof(len_str));
	sprintf(len_str, "%d",length);
	length_len =  strlen(len_str);
	memset(head_str,0,sizeof(head_str));
	sprintf(head_str,"%d%s",length_len,len_str);
	head_len = strlen(head_str);

    sum_length = length + head_len;
	
    // 判断是否写满
    int ret = cbIsFull(cb);
	if(ret )
	{
		return -1;
	}

	// 判断是否足够写
	if(cb->write_end < cb->read_start)
	{
		if(cb->read_start - cb->write_end < sum_length)
		{
			return -2;
		}
	}
	else
	{
		if(sum_length > cb->size - cb->write_end + cb->read_start)
		{
			return -2;
		}
	}


	// 写数据 

	// 先写描述数据
	cb->write_end_tmp = cb->write_end;

	cb_INwrite(cb,(unsigned char *)head_str, head_len);

	//写data
	cb_INwrite(cb,elem, length);

    // 更新状态
    pthread_mutex_lock(&work_mutex);
    cb->write_end =cb->write_end_tmp;
	if(cb->write_end == cb->read_start)
	{
		cb->w_msb =READ_ONLY;
	}
	else
	{
		cb->w_msb = READ_WRITE_CAN;
	}

	pthread_mutex_unlock(&work_mutex);
	
	return 0;
}



void cb_INread(CircularBuffer *cb, unsigned char *elem, unsigned int length,unsigned int offset )
{
	if(cb->read_start_tmp+length+offset > cb->size)
	{
		memcpy(elem,cb->data+cb->read_start_tmp+offset,cb->size - cb->read_start_tmp-offset );
		memcpy(elem+cb->size - cb->read_start_tmp-offset,cb->data,length- cb->size + cb->read_start_tmp-offset );
		cb->read_start_tmp = cb->read_start_tmp+length+offset -  cb->size;
	}
	else
	{
		memcpy(elem,cb->data+cb->read_start_tmp+offset,length );
		if(cb->read_start_tmp+length+offset == cb->size)
		{
			cb->read_start_tmp  = 0;
		}else
		{
			cb->read_start_tmp = cb->read_start_tmp+length+offset;
		}
	}
}


int cbRead(CircularBuffer *cb, unsigned char *elem, unsigned int *length)
{
	//判断是否空
	int ret = cbIsEmpty(cb);
	if(ret)
	{
		return -1;
	}

	//首先读取头部描述

	cb->read_start_tmp = cb->read_start;
	unsigned int head_len = *(cb->data + cb->read_start_tmp)-'0';
	unsigned char head_str[20];
	unsigned int data_len = 0;
	
	memset(head_str,0,sizeof(head_str));
	cb_INread(cb,head_str,head_len,1);
	data_len = atoi((char *)head_str);
	//printf("cbRead %d \n", data_len );
	*length = data_len;

	//读取内容
	cb_INread(cb,elem,data_len,0);
	//更新read_start 不知道是否要加锁
    
    pthread_mutex_lock(&work_mutex);
	cb->read_start = cb->read_start_tmp;
	if(cb->read_start == cb->write_end)
	{
		cb->w_msb  = WRITE_ONLY;
	}
	else
	{
		cb->w_msb  = READ_WRITE_CAN;
	}

	pthread_mutex_unlock(&work_mutex);

	return 0;
 
}


int cbReadhead(CircularBuffer *cb,unsigned int *datalenth)
{
	//判断是否空
	int ret = cbIsEmpty(cb);
	if(ret)
	{
		return -1;
	}
	//首先读取头部描述

	cb->read_start_tmp = cb->read_start;
	unsigned int head_len = *(cb->data + cb->read_start_tmp)-'0';
	unsigned char head_str[20];
	unsigned int data_len = 0;
	
	memset(head_str,0,sizeof(head_str));
	cb_INread(cb,head_str,head_len,1);
	data_len = atoi((char *)head_str);
	//printf("cbRead %d \n", data_len );
	*datalenth = data_len;
	return 0;
}

int cbReadData(CircularBuffer *cb, unsigned char *outelem, unsigned int length)
{
	//读取内容
	cb_INread(cb,outelem,length,0);
	//更新read_start 不知道是否要加锁
    
    pthread_mutex_lock(&work_mutex);
	cb->read_start = cb->read_start_tmp;
	if(cb->read_start == cb->write_end)
	{
		cb->w_msb  = WRITE_ONLY;
	}
	else
	{
		cb->w_msb  = READ_WRITE_CAN;
	}

	pthread_mutex_unlock(&work_mutex);

	return 0;
}





