
/*
 * mirrorringbuffer.h
 *
 *  Created on: 2013-7-17
 *
 *  http://en.wikipedia.org/wiki/Circular_buffer
 */

#ifndef MIRRORRINGBUFFER_H_
#define MIRRORRINGBUFFER_H_

#define CIRCLE_BUFFER_SIZE_DEFAULT 1024*1000*3

/* Circular buffer object */
typedef struct
{
	unsigned int size; /* maximum number of elements           */
	unsigned int read_start; /* index of oldest element              */
	unsigned int read_start_tmp;
	unsigned int write_end; /* index at which to write new element  */
	unsigned int write_end_tmp;
	int w_msb;  /*flag about full or empty*/
	unsigned char  *data; /* vector of elements                   */
} CircularBuffer;

typedef enum
{
	READ_WRITE_CAN,
	WRITE_ONLY,
	READ_ONLY,
}READ_WRITE_FLAG;


extern int cbInit(CircularBuffer *cb, unsigned int size);
extern int cbIsFull(CircularBuffer *cb);
extern int cbIsEmpty(CircularBuffer *cb);
extern int cbWrite(CircularBuffer *cb, unsigned char *elem, unsigned int length);
extern int cbReadData(CircularBuffer *cb, unsigned char *outelem, unsigned int length);
extern int cbReadhead(CircularBuffer *cb,unsigned int *datalenth);

#endif /* MIRRORRINGBUFFER_H_ */
