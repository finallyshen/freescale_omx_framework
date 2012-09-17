/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <stdio.h>
#include <dlfcn.h>
#include "Mem.h"
#include "Log.h"
#include "RingBuffer.h"

RingBuffer::RingBuffer()
{
	TS_Queue = NULL;
	RingBufferPtr = NULL;
	Reserved = NULL;
	CurrentTS = 0;
	bHaveTS = E_FSL_OSAL_FALSE;
	TotalConsumeLen = 0;
	RingBufferBeginPre = -1;
	nPrevOffset = 0;
}

RINGBUFFER_ERRORTYPE RingBuffer::BufferCreate(fsl_osal_u32 nPushModeLen, fsl_osal_u32 nRingBufferScale)
{
    RINGBUFFER_ERRORTYPE ret = RINGBUFFER_SUCCESS;

	nPushModeInputLen = nPushModeLen;
	nRingBufferLen = nPushModeInputLen * nRingBufferScale;
 
	/** Create queue for TS. */
	TS_Queue = FSL_NEW(Queue, ());
	if (TS_Queue == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return RINGBUFFER_INSUFFICIENT_RESOURCES;
	}

	if (TS_Queue->Create(TS_QUEUE_SIZE, sizeof(TS_QUEUE), E_FSL_OSAL_FALSE) != QUEUE_SUCCESS)
	{
		FSL_DELETE(TS_Queue);
		LOG_ERROR("Can't create audio ts queue.\n");
		return RINGBUFFER_INSUFFICIENT_RESOURCES;
	}

	/** Create ring buffer for audio decoder input stream. */
	LOG_DEBUG("Ring buffer len: %d\n", nRingBufferLen);
	RingBufferPtr = (fsl_osal_u8 *)FSL_MALLOC(nRingBufferLen+8);
	if (RingBufferPtr == NULL)
	{
		FSL_DELETE(TS_Queue);
		TS_Queue->Free();
		LOG_ERROR("Can't get memory.\n");
		return RINGBUFFER_INSUFFICIENT_RESOURCES;
	}

	Reserved = (fsl_osal_u8 *)FSL_MALLOC(nPushModeInputLen);
	if (Reserved == NULL)
	{ 
		FSL_DELETE(TS_Queue);
		TS_Queue->Free();
		FSL_FREE(RingBufferPtr);
		LOG_ERROR("Can't get memory.\n");
		return RINGBUFFER_INSUFFICIENT_RESOURCES;
	}

	CurrentTS = 0;
	bHaveTS = E_FSL_OSAL_FALSE;
	TotalConsumeLen = 0;
	RingBufferBeginPre = -1;
	ReservedLen = nPushModeInputLen;   
	Begin = RingBufferPtr;
	End = RingBufferPtr;
	Consumered = RingBufferPtr;
	nPrevOffset = 0;

    return ret;
}

RINGBUFFER_ERRORTYPE RingBuffer::BufferReset()
{
	RINGBUFFER_ERRORTYPE ret = RINGBUFFER_SUCCESS;

	while (TS_Queue->Size() > 0)
	{
		TS_QUEUE TS_Item;

		if (TS_Queue->Get(&TS_Item) != QUEUE_SUCCESS)
		{
			LOG_ERROR("Can't get audio TS item.\n");
			return RINGBUFFER_FAILURE;
		}
	}

	CurrentTS = 0;
	bHaveTS = E_FSL_OSAL_FALSE;
	TotalConsumeLen = 0;
	RingBufferBeginPre = -1;
	Begin = RingBufferPtr;
	End = RingBufferPtr;
	Consumered = RingBufferPtr;
	nPrevOffset = 0;

	return ret;
}

RINGBUFFER_ERRORTYPE RingBuffer::BufferFree()
{
	RINGBUFFER_ERRORTYPE ret = RINGBUFFER_SUCCESS;

	LOG_DEBUG("TS queue free\n");
	if(TS_Queue != NULL)
		TS_Queue->Free();
	LOG_DEBUG("TS queue delete\n");
	FSL_DELETE(TS_Queue);
	LOG_DEBUG("Free ring buffer\n");
	FSL_FREE(RingBufferPtr);
	LOG_DEBUG("Free reserved buffer\n");
	FSL_FREE(Reserved);
	LOG_DEBUG("Ring buffer free finished.\n");

	return ret;
}

RINGBUFFER_ERRORTYPE RingBuffer::TS_Add(fsl_osal_s64 ts)
{
	RINGBUFFER_ERRORTYPE ret = RINGBUFFER_SUCCESS;
	TS_QUEUE TS_Item;

	if (TS_Queue->Size() > 0)
	{
		if (TS_Queue->Access(&TS_Item, TS_Queue->Size()) != QUEUE_SUCCESS)
		{
			LOG_ERROR("Can't get audio TS item.\n");
			return RINGBUFFER_FAILURE;
		}
		if (TS_Item.ts == ts)
		{
			LOG_DEBUG("The same TS.\n");
			return ret;
		}
	}

	fsl_osal_s32 DataLen = AudioDataLen();
	TS_Item.ts = ts;
	/** Should add TS first after received buffer from input port */
	TS_Item.RingBufferBegin = TotalConsumeLen + DataLen; 
	LOG_LOG("TS: %lld\t RingBufferBegin: %lld\n", TS_Item.ts, TS_Item.RingBufferBegin);

	/* Update current time stamp at the first frame */
	if (TotalConsumeLen == 0 && AudioDataLen() == 0 && ts >= 0)
	{
		CurrentTS = ts;
	}

	if (TS_Queue->Add(&TS_Item) != QUEUE_SUCCESS)
	{
		LOG_ERROR("Can't add TS item to audio ts queue. Queue size: %d, max queue size: %d \n", TS_Queue->Size(), TS_QUEUE_SIZE);
		return RINGBUFFER_FAILURE;
	}

	return ret;
}

RINGBUFFER_ERRORTYPE RingBuffer::TS_Get(fsl_osal_s64 *ts)
{
	RINGBUFFER_ERRORTYPE ret = RINGBUFFER_SUCCESS;
	*ts = CurrentTS;
	LOG_DEBUG_INS("Get CurrentTS = %lld\n", CurrentTS);
	return ret;
}

RINGBUFFER_ERRORTYPE RingBuffer::TS_SetIncrease(fsl_osal_s64 ts)
{
    RINGBUFFER_ERRORTYPE ret = RINGBUFFER_SUCCESS;
	if (bHaveTS == E_FSL_OSAL_FALSE)
	{
		CurrentTS += ts;
	}
	bHaveTS = E_FSL_OSAL_FALSE;
	LOG_DEBUG_INS("Set CurrentTS = %lld\n", CurrentTS);
	return ret;
}

RINGBUFFER_ERRORTYPE RingBuffer::BufferAdd(fsl_osal_u8 *pBuffer, fsl_osal_u32 BufferLen, fsl_osal_u32 *pActualLen)
{
    RINGBUFFER_ERRORTYPE ret = RINGBUFFER_SUCCESS;

	fsl_osal_s32 DataLen = AudioDataLen();
	fsl_osal_s32 FreeBufferLen = nRingBufferLen - DataLen - 1;
	if (FreeBufferLen < (fsl_osal_s32)BufferLen)
	{
		*pActualLen = FreeBufferLen;
	}
	else
	{
		*pActualLen = BufferLen;
	}

	if (Begin + *pActualLen > RingBufferPtr + nRingBufferLen)
	{
		fsl_osal_u32 FirstSegmentLen = nRingBufferLen - (Begin - RingBufferPtr);
		fsl_osal_memcpy(Begin, pBuffer, FirstSegmentLen);
		fsl_osal_memcpy(RingBufferPtr, pBuffer + FirstSegmentLen, *pActualLen - FirstSegmentLen);
		Begin = RingBufferPtr + *pActualLen - FirstSegmentLen;
	}
	else
	{
		fsl_osal_memcpy(Begin, pBuffer, *pActualLen);
		Begin += *pActualLen;
	}

	LOG_LOG("nRingBufferLen = %d\t DataLen = %d\n", nRingBufferLen, DataLen);

    return ret;
}

RINGBUFFER_ERRORTYPE RingBuffer::BufferAddZeros(fsl_osal_u32 BufferLen, fsl_osal_u32 *pActualLen)
{
    RINGBUFFER_ERRORTYPE ret = RINGBUFFER_SUCCESS;

	fsl_osal_u32 DataLen = AudioDataLen();
	fsl_osal_u32 FreeBufferLen = nRingBufferLen - DataLen - 1;
	if (FreeBufferLen < BufferLen)
	{
		*pActualLen = FreeBufferLen;
	}
	else
	{
		*pActualLen = BufferLen;
	}

	if (Begin + *pActualLen > RingBufferPtr + nRingBufferLen)
	{
		fsl_osal_u32 FirstSegmentLen = nRingBufferLen - (Begin - RingBufferPtr);
		fsl_osal_memset(Begin, 0, FirstSegmentLen);
		fsl_osal_memset(RingBufferPtr, 0, *pActualLen - FirstSegmentLen);
		Begin = RingBufferPtr + *pActualLen - FirstSegmentLen;
	}
	else
	{
		fsl_osal_memset(Begin, 0, *pActualLen);
		Begin += *pActualLen;
	}

	LOG_LOG("nRingBufferLen = %d\t DataLen = %d\n", nRingBufferLen, DataLen);

    return ret;
}


fsl_osal_u32 RingBuffer::AudioDataLen()
{
	fsl_osal_s32 DataLen = Begin - End;
	if (DataLen < 0)
	{
		DataLen = nRingBufferLen - (End - Begin);
	}

    return (fsl_osal_u32)DataLen;
}

RINGBUFFER_ERRORTYPE RingBuffer::BufferGet(fsl_osal_u8 **ppBuffer, fsl_osal_u32 BufferLen, fsl_osal_u32 *pActualLen)
{
    RINGBUFFER_ERRORTYPE ret = RINGBUFFER_SUCCESS;
	fsl_osal_s32 DataLen = AudioDataLen();
	if (DataLen < (fsl_osal_s32)BufferLen)
	{
		*pActualLen = DataLen;
	}
	else
	{
		*pActualLen = BufferLen;
	}
	if (ReservedLen < *pActualLen)
	{
		LOG_WARNING("Reserved buffer is too short.\n");
		*pActualLen = ReservedLen;
	}

	if (End + *pActualLen > RingBufferPtr + nRingBufferLen)
	{
		fsl_osal_u32 FirstSegmentLen = nRingBufferLen - (End - RingBufferPtr);
		fsl_osal_memcpy(Reserved, End, FirstSegmentLen);
		fsl_osal_memcpy(Reserved + FirstSegmentLen, RingBufferPtr, *pActualLen - FirstSegmentLen);
		*ppBuffer = Reserved;
	}
	else
	{
		*ppBuffer = End;
	}

	LOG_LOG("nRingBufferLen = %d\t DataLen = %d\n", nRingBufferLen, DataLen);
    return ret;
}

fsl_osal_u32 RingBuffer::GetFrameLen()
{
	TS_QUEUE TS_Item;
	fsl_osal_u32 nFrameLen = 0;

	BufferConsumered(0);

	if (TS_Queue->Access(&TS_Item, 1) != QUEUE_SUCCESS)
	{
		LOG_WARNING("Can't get audio TS item.\n");
		return AudioDataLen();
	}

	nFrameLen = TS_Item.RingBufferBegin - TotalConsumeLen;
	LOG_LOG("TS_Item.RingBufferBegin: %lld\n", TS_Item.RingBufferBegin);
	LOG_LOG("TotalConsumeLen: %lld\n", TotalConsumeLen);
	LOG_LOG("Frame Len: %d\n", nFrameLen);

	return nFrameLen;
}

RINGBUFFER_ERRORTYPE RingBuffer::BufferConsumered(fsl_osal_u32 ConsumeredLen)
{
    RINGBUFFER_ERRORTYPE ret = RINGBUFFER_SUCCESS;
	fsl_osal_s32 DataLen = AudioDataLen();
	if (DataLen < (fsl_osal_s32)ConsumeredLen)
	{
		LOG_ERROR("Ring buffer consumer point set error.\n");
		return RINGBUFFER_FAILURE;
	}

	if (End + ConsumeredLen > RingBufferPtr + nRingBufferLen)
	{
		End += ConsumeredLen - nRingBufferLen;
	}
	else
	{
		End += ConsumeredLen;
	}

	TotalConsumeLen += ConsumeredLen;

	if (TS_Queue->Size() < 1)
	{
		return ret;
	}
	/** Adjust current TS */
	TS_QUEUE TS_Item;

	while (1)
	{
		if (TS_Queue->Access(&TS_Item, 1) != QUEUE_SUCCESS)
		{
			LOG_ERROR("Can't get audio TS item.\n");
			return RINGBUFFER_FAILURE;
		}

		if (TotalConsumeLen >= TS_Item.RingBufferBegin)
		{
		    if(TS_Item.ts >= 0 && RingBufferBeginPre != TS_Item.RingBufferBegin) 
			{
				RingBufferBeginPre = TS_Item.RingBufferBegin;
        		CurrentTS = TS_Item.ts;
				bHaveTS = E_FSL_OSAL_TRUE;
				LOG_DEBUG_INS("Reset CurrentTS = %lld\n", CurrentTS);
			}
			if (TS_Queue->Size() > 1)
			{ 
				if (TS_Queue->Get(&TS_Item) != QUEUE_SUCCESS)
				{
					LOG_ERROR("Can't get audio TS item.\n");
					return RINGBUFFER_FAILURE;
				}
				LOG_LOG("Remove TS: %lld\t RingBufferBegin: %lld\n", TS_Item.ts, TS_Item.RingBufferBegin);
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	LOG_LOG("nRingBufferLen = %d\t DataLen = %d\n", nRingBufferLen, DataLen);
	return ret;
}

/* File EOF */
