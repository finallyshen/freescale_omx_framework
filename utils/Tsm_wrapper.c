/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "Tsm_wrapper.h"

//#define TS_ENA_MULTI_STRATEGY	//for debug: run multi-strategy simultaneously
#define TS_USE_QUERY_API		//the query value may be not matched with original method
#define TS_LIST_LEN 128
#define TS_LIST_FULL_THRESHOLD	(TS_LIST_LEN-5)
#define TS_MAX_DIFF_US	90000
#define TS_MAX_QUERY_DIFF_US	90000
#define TS_INVALIDTS (-1)
#define TS_SCALE	1000
#define TS_ABS(t1,t2) (((t1)>=(t2))?((t1)-(t2)):((t2)-(t1)))

//#define TS_DEBUG
#ifdef TS_DEBUG
#define TS_API	printf
#define LOG_DEBUG printf
#define LOG_ERROR printf
#else
#define TS_API(...) //printf
#define LOG_DEBUG(...) //printf
#define LOG_ERROR printf
#endif

#define ts_memset memset//fsl_osal_memset
#define ts_memcpy memcpy//fsl_osal_memcpy
#define ts_malloc malloc//FSL_MALLOC
#define ts_free free//FSL_FREE
#define TS_TRUE 1
#define TS_FALSE 0

typedef struct
{
	//original object
	void* pHandleOri;
	int nInTsCntOri;
	long long nInCurTsOri;

	//new object
	void* pHandle2;
	int nInTsCnt2;
	long long nInCurTs2;
	int nIsActived2;	//accurate frame size is supported ?
	signed long long nAccBlkOffset2;	//debug: accumulated bytes for blk
	signed long long nAccFrmOffset2;	//debug: accumulated bytes for frame

	//global info
	long long nLastTs;
	long long nDeltaTs;
} TSM_OBJ;


void* tsmCreate()
{
	TSM_OBJ* pObj=NULL;
	void* ptr=NULL;

	//malloc memory for object
	pObj=(TSM_OBJ*)ts_malloc(sizeof(TSM_OBJ));
	if(pObj==NULL)
	{
		LOG_ERROR("%s: error: malloc %d bytes fail \n",__FUNCTION__,sizeof(TSM_OBJ));
		return NULL;
	}
	ts_memset((void*)pObj, 0, sizeof(TSM_OBJ));

	//create original ts manager
	TS_API("ori:calling createTSManager(%d) \n",TS_LIST_LEN);
	ptr=createTSManager(TS_LIST_LEN);
	if(ptr)
	{
		pObj->pHandleOri=ptr;
		pObj->nInTsCntOri=0;
		pObj->nInCurTsOri=TS_INVALIDTS;
	}
	else
	{
		goto TSM_Create_Fail;
	}

	//create new ts manager
	TS_API("new:calling createTSManager(%d) \n",TS_LIST_LEN);
	ptr=createTSManager(TS_LIST_LEN);
	if(ptr)
	{
		pObj->pHandle2=ptr;
		pObj->nInTsCnt2=0;
		pObj->nInCurTs2=TS_INVALIDTS;
	}
	else
	{
		goto TSM_Create_Fail;
	}

	//set default value
	pObj->nLastTs=TS_INVALIDTS;
	pObj->nDeltaTs=TS_INVALIDTS;
	return (void*)pObj;

TSM_Create_Fail:
	if(pObj->pHandleOri)
	{
		TS_API("ori:calling destroyTSManager \n");
		destroyTSManager(pObj->pHandleOri);
		pObj->pHandleOri=NULL;
	}
	if(pObj->pHandle2)
	{
		TS_API("new:calling destroyTSManager \n");
		destroyTSManager(pObj->pHandle2);
		pObj->pHandle2=NULL;
	}
	if(pObj)
	{
		ts_free(pObj);
	}
	return NULL;
}

int tsmDestroy(void* pHandle)
{
	TSM_OBJ* pObj=(TSM_OBJ*)pHandle;
	if(pObj->pHandleOri)
	{
		TS_API("ori:calling destroyTSManager \n");
		destroyTSManager(pObj->pHandleOri);
		pObj->pHandleOri=NULL;
	}
	if(pObj->pHandle2)
	{
		TS_API("new:calling destroyTSManager \n");
		destroyTSManager(pObj->pHandle2);
		pObj->pHandle2=NULL;
	}
	ts_free(pObj);
	return 1;
}

int tsmSetFrmRate(void* pHandle,int nFsN, int nFsD)
{
	TSM_OBJ* pObj=(TSM_OBJ*)pHandle;
	if(pObj->pHandleOri)
	{
		TS_API("ori:calling setTSManagerFrameRate: fps(%d/%d) \n",nFsN, nFsD);
		setTSManagerFrameRate(pObj->pHandleOri, nFsN, nFsD);
	}
	if(pObj->pHandle2)
	{
		TS_API("new:calling setTSManagerFrameRate: fps(%d/%d) \n",nFsN, nFsD);
		setTSManagerFrameRate(pObj->pHandle2, nFsN, nFsD);
	}
	return 1;
}


int tsmSetBlkTs(void* pHandle,int nSize, TSM_TIMESTAMP Ts)
{
	TSM_OBJ* pObj=(TSM_OBJ*)pHandle;
	TSM_TIMESTAMP ts=TS_INVALIDTS;
	if(pObj->pHandleOri)
	{
		ts=Ts;
		if(ts == TS_INVALIDTS)
		{
			ts = TSM_TIMESTAMP_NONE;
		}
		else
		{
			ts*= TS_SCALE;
		}
		TS_API("ori:calling TSManagerReceive: ts(ns): %lld, total: %d\n", ts, (int)pObj->nInTsCntOri);
		TSManagerReceive(pObj->pHandleOri, ts);
		pObj->nInTsCntOri++;
	}
	if(pObj->pHandle2)
	{
		ts=Ts;
		if(ts == TS_INVALIDTS)
		{
			ts = TSM_TIMESTAMP_NONE;
		}
		else
		{
			ts*= TS_SCALE;
		}
		TS_API("new:calling TSManagerReceive2: [%lld(0x%llX)] ts(ns): %lld, size: %d, total: %d\n",pObj->nAccBlkOffset2,pObj->nAccBlkOffset2,ts, nSize,(int)pObj->nInTsCnt2);
		TSManagerReceive2(pObj->pHandle2, ts,nSize);
		pObj->nInTsCnt2++;
		pObj->nAccBlkOffset2+=nSize;
	}
	return 1;
}

int tsmSetFrmBoundary(void* pHandle,int nStuffSize,int nFrmSize,void* pfrmHandle)
{
	/*pfrmHandle==NULL: the frame is skipped for skipmode/corrupt/... cases*/
	TSM_OBJ* pObj=(TSM_OBJ*)pHandle;
	if(pObj->pHandleOri)
	{
		//nothing
	}
	if(pObj->pHandle2)
	{
		if(pObj->nIsActived2==0)
		{
			pObj->nIsActived2=1;	//accurate frame size is reported by decoder
		}
		pObj->nAccFrmOffset2+=nStuffSize;
#if 1	//consider special case: config data contain valid frames, as result, stuffsize < 0
		if(pObj->nAccFrmOffset2+nFrmSize<=0)
		{
			TS_API("new:calling TSManagerValid2(0): [%lld(0x%llX)]: discarded frame size: %d \n",pObj->nAccFrmOffset2,pObj->nAccFrmOffset2,nFrmSize);
			TSManagerValid2(pObj->pHandle2,0,pfrmHandle);
		}
		else if(pObj->nAccFrmOffset2<=0)
		{
			TS_API("new:calling TSManagerValid2(*): [%lld(0x%llX)]: discarded frame size: %d \n",pObj->nAccFrmOffset2,pObj->nAccFrmOffset2,nFrmSize);
			TSManagerValid2(pObj->pHandle2,pObj->nAccFrmOffset2+nFrmSize,pfrmHandle);
		}
		else
#endif
		{
			if(NULL==pfrmHandle)
			{
				TS_API("new:calling TSManagerValid2: [%lld(0x%llX)]: flush %d bytes, skipped frame size: %d \n",pObj->nAccFrmOffset2,pObj->nAccFrmOffset2,nStuffSize,nFrmSize);
				TSManagerValid2(pObj->pHandle2,nStuffSize+nFrmSize,pfrmHandle);
			}
			else
			{
				TS_API("new:calling TSManagerFlush2/TSManagerValid2: [%lld(0x%llX)]: flush %d bytes, frame size: %d, frm: 0x%X \n",pObj->nAccFrmOffset2,pObj->nAccFrmOffset2,nStuffSize,nFrmSize,pfrmHandle);
				TSManagerFlush2(pObj->pHandle2,nStuffSize);
				TSManagerValid2(pObj->pHandle2,nFrmSize,pfrmHandle);
			}
		}
		//In fact, nInTsCnt2 may be changed after calling TSManagerValid2(), so nInTsCnt2 become unmeaningful
		pObj->nAccFrmOffset2+=nFrmSize;
#ifndef TS_ENA_MULTI_STRATEGY
		//disable original method automatically, only one strategy is enabled
		if(pObj->pHandleOri)
		{
			LOG_DEBUG("new strategy is detected, original will be disabled automatically\n");
			TS_API("ori: calling destroyTSManager\n");
			destroyTSManager(pObj->pHandleOri);
			pObj->pHandleOri=NULL;
		}
#endif
	}
	return 1;
}


TSM_TIMESTAMP tsmGetFrmTs(void* pHandle,void* pfrmHandle)
{
	/*nfrmHandle==NULL:
		(1) only need to pop one time stamp. (e.g. frame is not decoded at all by vpu for skipmode/corrupt/... cases);
		(2) mosaic frame which is dropped by vpu for non-gop case
	*/
	TSM_TIMESTAMP ts=TS_INVALIDTS;
	TSM_TIMESTAMP tsOri=TS_INVALIDTS;
	TSM_TIMESTAMP ts2=TS_INVALIDTS;
	TSM_TIMESTAMP tsDiff;
	TSM_OBJ* pObj=(TSM_OBJ*)pHandle;
	if(pObj->pHandleOri)
	{
		if(pObj->nInCurTsOri != TS_INVALIDTS)
		{
			tsOri = pObj->nInCurTsOri;
			pObj->nInCurTsOri = TS_INVALIDTS;
		}
		else
		{
			tsOri = TSManagerSend(pObj->pHandleOri);
			TS_API("ori: calling TSManagerSend: returned ts(ns): %lld, total: %d \n",tsOri, (int)pObj->nInTsCntOri);
			tsOri=tsOri/TS_SCALE;
			pObj->nInTsCntOri--;
		}
		ts=tsOri;
	}
	if(pObj->pHandle2)
	{
		if(pObj->nIsActived2==0)
		{
			LOG_DEBUG("new strategy isn't detected, it will be disabled automatically\n");
			TS_API("new: calling destroyTSManager\n");
			destroyTSManager(pObj->pHandle2);
			pObj->pHandle2=NULL;
		}
		else
		{
			if(pObj->nInCurTs2 != TS_INVALIDTS)
			{
				ts2 = pObj->nInCurTs2;
				pObj->nInCurTs2 = TS_INVALIDTS;
			}
			else
			{
				if(NULL==pfrmHandle)
				{
					ts2=TSManagerSend2(pObj->pHandle2, NULL);
					TS_API("new: calling TSManagerSend2(NULL): returned ts(ns): %lld, total: %d \n",ts2, (int)pObj->nInTsCnt2);
				}
				else
				{
					ts2=TSManagerSend2(pObj->pHandle2, pfrmHandle);
					TS_API("new: calling TSManagerSend2: frm: 0x%X, returned ts(ns): %lld, total: %d \n",pfrmHandle,ts2, (int)pObj->nInTsCnt2);
				}
				ts2=ts2/TS_SCALE;
				pObj->nInTsCnt2--;
			}
			//LOG_DEBUG("new: get one ts: %lld, total: %d\n", ts2, (int)pObj->nInTsCnt2);
			ts=ts2;
		}
	}

	//double check the ts for different schema
	if(pObj->pHandleOri && pObj->pHandle2)
	{
		tsDiff=TS_ABS(tsOri,ts2);
		if(tsDiff>=TS_MAX_DIFF_US)
		{
			LOG_ERROR("LEVEL: 1 %s: the time stamp is conflict: ori ts(us): %lld, new ts(us): %lld, diff(us): %lld \n",__FUNCTION__,tsOri,ts2,tsDiff);
		}
	}

	if(ts==TS_INVALIDTS)
	{
		LOG_ERROR("%s: error: can't get one valid ts \n",__FUNCTION__);
	}
#ifdef TS_DEBUG
	if(pObj->nLastTs!=TS_INVALIDTS)
	{
		pObj->nDeltaTs=ts-pObj->nLastTs;
	}
	pObj->nLastTs=ts;
	LOG_DEBUG("%s: current ts(us): %lld, delta(us): %lld \n",__FUNCTION__,pObj->nLastTs,pObj->nDeltaTs);
#endif
	return ts;
}

int tsmFlush(void* pHandle, TSM_TIMESTAMP synctime, TSMGR_MODE mode)
{
	TSM_OBJ* pObj=(TSM_OBJ*)pHandle;
	TSM_TIMESTAMP ts;
	if(pObj->pHandleOri)
	{
		ts=synctime*TS_SCALE;
		TS_API("ori: calling resyncTSManager: synctime: %lld, mode: %d \n",ts,mode);
		resyncTSManager(pObj->pHandleOri, ts, mode);
		pObj->nInTsCntOri=0;
		pObj->nInCurTsOri=TS_INVALIDTS;
	}
	if(pObj->pHandle2)
	{
		ts=synctime*TS_SCALE;
		TS_API("new: calling resyncTSManager: synctime: %lld, mode: %d \n",ts,mode);
		resyncTSManager(pObj->pHandle2, ts, mode);
		pObj->nInTsCnt2=0;
		pObj->nInCurTs2=TS_INVALIDTS;
		pObj->nAccBlkOffset2=0;
		pObj->nAccFrmOffset2=0;
	}
	pObj->nLastTs=TS_INVALIDTS;
	pObj->nDeltaTs=TS_INVALIDTS;
	return 1;
}

int tsmHasEnoughSlot(void* pHandle)
{
	TSM_OBJ* pObj=(TSM_OBJ*)pHandle;
	if(pObj->pHandleOri)
	{
		if(pObj->nInTsCntOri >= TS_LIST_FULL_THRESHOLD)
		{
			return TS_FALSE;
		}
		else
		{
			return TS_TRUE;
		}
	}
	if(pObj->pHandle2)
	{
#if 0	//won't fail(overflow) again, so we awlays return true
		if(pObj->nInTsCnt2 >= TS_LIST_FULL_THRESHOLD)
		{
			return TS_FALSE;
		}
		else
#endif
		{
			return TS_TRUE;
		}
	}
	return TS_TRUE;
}

TSM_TIMESTAMP tsmQueryCurrTs(void* pHandle)
{
	TSM_OBJ* pObj=(TSM_OBJ*)pHandle;
	TSM_TIMESTAMP ts=TS_INVALIDTS;
	TSM_TIMESTAMP tsOri=TS_INVALIDTS;
	TSM_TIMESTAMP ts2=TS_INVALIDTS;
	TSM_TIMESTAMP tsDiff;
	if(pObj->pHandleOri)
	{
		if(pObj->nInTsCntOri == 0)
		{
			tsOri=TS_INVALIDTS;
		}
		else if(pObj->nInCurTsOri != TS_INVALIDTS)
		{
			tsOri=pObj->nInCurTsOri;
		}
		else
		{
			tsOri = TSManagerSend(pObj->pHandleOri);
			TS_API("ori: calling TSManagerSend: returned(query) ts(ns): %lld \n",tsOri);
			if(tsOri<0)
			{
				LOG_ERROR("%s: error: one invalid ts(%lld) is queried: set it 0 manually\n",__FUNCTION__,tsOri);
				//set one different value with TS_INVALIDTS to match the 'nInTsCntOri'
				pObj->nInCurTsOri=TS_INVALIDTS+1;
			}
			else
			{
				tsOri=tsOri/TS_SCALE;
				pObj->nInCurTsOri = tsOri;
			}
			pObj->nInTsCntOri--;
		}
		ts=tsOri;
		//LOG_DEBUG("ori: query one ts: %lld \n",tsOri);
	}
	if(pObj->pHandle2)
	{
#ifdef TS_USE_QUERY_API
		if(pObj->nIsActived2==0)
		{
			//hasn't been enabled
			ts2=TS_INVALIDTS;
		}
		else
		{
			//FIXME: the ts may be not matched
			ts2=TSManagerQuery2(pObj->pHandle2,NULL);
			TS_API("new: calling TSManagerQuery2(NULL): returned ts(ns): %lld \n",ts2);
			ts2=(ts2==TS_INVALIDTS)?ts2:(ts2/TS_SCALE);
		}
#else
		if(pObj->nInTsCnt2 == 0)
		{
			ts2=TS_INVALIDTS;
		}
		else if(pObj->nInCurTs2 != TS_INVALIDTS)
		{
			ts2=pObj->nInCurTs2;
		}
		else
		{
			//FIXME: the ts may be not matched without calling TSManagerValid2()
			ts2 = TSManagerSend2(pObj->pHandle2,NULL);
			TS_API("new: calling TSManagerSend2(NULL): returned(query) ts(ns): %lld \n",ts2);
			if(ts2<0)
			{
				LOG_ERROR("%s: error: one invalid ts(%lld) is queried: set it 0 manually\n",__FUNCTION__,ts2);
				//set one different value with TS_INVALIDTS to match the 'nInTsCntOri'
				pObj->nInCurTs2=TS_INVALIDTS+1;
			}
			else
			{
				ts2=ts2/TS_SCALE;
				pObj->nInCurTs2 = ts2;
			}
			pObj->nInTsCnt2--;
		}
#endif
		ts=ts2;
		//LOG_DEBUG("new: query one ts: %lld \n",ts2);
	}

	//double check the ts for different schema
	//if(pObj->pHandleOri && pObj->pHandle2){
	if((tsOri!=TS_INVALIDTS)&&(ts2!=TS_INVALIDTS))
	{
		tsDiff=TS_ABS(tsOri,ts2);
		if(tsDiff>=TS_MAX_QUERY_DIFF_US)
		{
			LOG_ERROR("LEVEL: 1 %s: the time stamp is conflict: ori ts(us): %lld, new ts(us): %lld, diff(us): %lld \n",__FUNCTION__,tsOri,ts2,tsDiff);
		}
	}

	return ts;
}
