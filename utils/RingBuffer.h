/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file RingBuffer.h
 *  @brief Class definition of ring buffer
 *  @ingroup RingBuffer
 */


#ifndef RingBuffer_h
#define RingBuffer_h

#include "fsl_osal.h"
#include "Queue.h"

#define RING_BUFFER_SCALE 2
#define TS_QUEUE_SIZE (1024*20)

typedef enum {
    RINGBUFFER_SUCCESS,
    RINGBUFFER_FAILURE,
    RINGBUFFER_INSUFFICIENT_RESOURCES
}RINGBUFFER_ERRORTYPE;

typedef struct _TS_QUEUE{
    fsl_osal_s64 ts;
	fsl_osal_s64 RingBufferBegin;
}TS_QUEUE;

class RingBuffer {
	public:
		RingBuffer();
		RINGBUFFER_ERRORTYPE BufferCreate(fsl_osal_u32 nPushModeLen, fsl_osal_u32 nRingBufferScale = RING_BUFFER_SCALE);
		RINGBUFFER_ERRORTYPE BufferFree();
		RINGBUFFER_ERRORTYPE BufferReset();
		RINGBUFFER_ERRORTYPE TS_Add(fsl_osal_s64 ts);
		/** Set TS increase after decode one of audio data */
		RINGBUFFER_ERRORTYPE TS_SetIncrease(fsl_osal_s64 ts); 
		/** Get TS for output audio data of audio decoder. The output TS is calculated for 
		 every frame of audio data */
		RINGBUFFER_ERRORTYPE TS_Get(fsl_osal_s64 *ts); 
		RINGBUFFER_ERRORTYPE BufferAdd(fsl_osal_u8 *pBuffer, fsl_osal_u32 BufferLen, fsl_osal_u32 *pActualLen);
		RINGBUFFER_ERRORTYPE BufferAddZeros(fsl_osal_u32 BufferLen, fsl_osal_u32 *pActualLen);
		fsl_osal_u32 AudioDataLen();
		fsl_osal_u32 GetFrameLen();
		RINGBUFFER_ERRORTYPE BufferGet(fsl_osal_u8 **ppBuffer, fsl_osal_u32 BufferLen, fsl_osal_u32 *pActualLen);
		/** Set consumered point for ring buffer. */
		RINGBUFFER_ERRORTYPE BufferConsumered(fsl_osal_u32 ConsumeredLen);
		fsl_osal_u32 nPrevOffset;
	private:
		Queue *TS_Queue;
		fsl_osal_u32 nPushModeInputLen;
		fsl_osal_s64 CurrentTS;
		efsl_osal_bool bHaveTS;
		fsl_osal_s64 TotalConsumeLen;
		fsl_osal_s64 RingBufferBeginPre;
		fsl_osal_u8 *RingBufferPtr;
		fsl_osal_u32 nRingBufferLen; /**< Should at least RING_BUFFER_SCALE * PUSH model input buffer length */
		fsl_osal_u8 *Reserved;
		fsl_osal_u32 ReservedLen;   /**< 1/RING_BUFFER_SCALE length of ring buffer length, 
								 used for the end of ring buffer data */
		fsl_osal_u8 *Begin;
		fsl_osal_u8 *End;
		fsl_osal_u8 *Consumered;
};

#endif
/* File EOF */
