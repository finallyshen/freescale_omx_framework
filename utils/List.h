/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file utils/List.h
 *  @brief List template class.
 *  @ingroup utils
 */

#ifndef List_h
#define List_h

#include "fsl_osal.h"
#include "Log.h"
#include "Mem.h"

typedef enum LIST_RETURNTYPE {
	LIST_SUCCESS = 0,
    LIST_FAILURE
} LSIT_RETURNTYPE; 
 
template<class T> class List
{
	public:
		List();
		LSIT_RETURNTYPE Add(T *node, fsl_osal_u32 priority = 0); /**< Add node at the tail or based on priority*/
		LSIT_RETURNTYPE Add(T *node, T *pUpNode); /**< Add node behind up node*/
		LSIT_RETURNTYPE Remove(T *node); /**< Remove one node */
		LSIT_RETURNTYPE Replace(T *node, T *nodeNew); /**< Replace one node with new node */
		fsl_osal_u32 GetNodeCnt();
		T *GetNode(fsl_osal_u32 index); /**< Get one node based on index, first index is 0 */
		~List(); /**< Free all node */
	private:
		struct NODE{
			NODE *pNext;
			T *pT;
			fsl_osal_u32 priority;
		};
		NODE *pFirst;
		fsl_osal_u32 NodeCnt;
};

template<class T> List<T>::List()
{
	pFirst = NULL;
	NodeCnt = 0;	
}

template<class T>
LSIT_RETURNTYPE List<T>::Add(T *node, fsl_osal_u32 priority)
{
	NODE **ppTmp = &pFirst, **ppTmp2 = &pFirst, *pTmp3;

	if (node == NULL)
	{
		LOG_ERROR("Add NULL to list.\n");
		return LIST_FAILURE;
	}

	while (*ppTmp != NULL)
	{
		if (priority > (*ppTmp)->priority) 
		{
			break;
		}
		ppTmp2 = ppTmp;
		ppTmp = &((*ppTmp)->pNext);
	}

	pTmp3 = FSL_NEW(NODE, ());
	pTmp3->pT = node;
	pTmp3->pNext = NULL;
	pTmp3->priority = priority;
	NodeCnt ++;

	if (priority == 0)
	{
		*ppTmp = pTmp3;
	}
	else
	{
		if (*ppTmp == *ppTmp2)
		{
			/** Insert to begin. */
			if (pFirst != NULL)
			{
				pTmp3->pNext = pFirst;
			}
			*ppTmp2 = pTmp3;
		}
		else if (*ppTmp == NULL)
		{
			/** Insert to the end */
			*ppTmp = pTmp3;
		}
		else
		{
			/** Insert to the middle */
			pTmp3->pNext = *ppTmp;
			(*ppTmp2)->pNext = pTmp3;
		}
	}

	return LIST_SUCCESS;
}

template<class T>
LSIT_RETURNTYPE List<T>::Add(T *node, T *pUpNode)
{
	NODE **ppTmp = &pFirst, **ppTmp2 = &pFirst, *pTmp3;

	if (node == NULL)
	{
		LOG_ERROR("Add NULL to list.\n");
		return LIST_FAILURE;
	}

	while (*ppTmp != NULL)
	{
		if (pUpNode == (*ppTmp)->pT) 
		{
			ppTmp2 = ppTmp;
			break;
		}
		ppTmp = &((*ppTmp)->pNext);
	}

	if (*ppTmp == NULL)
	{
		LOG_ERROR("Add NULL to list.\n");
		return LIST_FAILURE;
	}
	ppTmp = &((*ppTmp)->pNext);

	pTmp3 = FSL_NEW(NODE, ());
	pTmp3->pT = node;
	pTmp3->pNext = NULL;
	pTmp3->priority = (*ppTmp2)->priority;
	NodeCnt ++;

	if (*ppTmp == *ppTmp2)
	{
		/** Insert to begin. */
		if (pFirst != NULL)
		{
			pTmp3->pNext = pFirst;
		}
		*ppTmp2 = pTmp3;
	}
	else if (*ppTmp == NULL)
	{
		/** Insert to the end */
		*ppTmp = pTmp3;
	}
	else
	{
		/** Insert to the middle */
		pTmp3->pNext = *ppTmp;
		(*ppTmp2)->pNext = pTmp3;
	}

	return LIST_SUCCESS;
}


template<class T>
LSIT_RETURNTYPE List<T>::Remove(T *node)
{
	NODE *pTmp, *pTmp2;

	if (node == NULL)
	{
		LOG_ERROR("Remove NULL from list.\n");
		return LIST_FAILURE;
	}

	pTmp = pFirst;
	pTmp2 = pFirst;
	while (pTmp != NULL)
	{
		if (pTmp->pT == node)
		{
			LOG_LOG("Found the node in the list.\n");
			if (pTmp == pFirst)
			{
				pFirst = pTmp->pNext;
			}
			else
			{
				pTmp2->pNext = pTmp->pNext;
			}
			FSL_DELETE(pTmp);
			NodeCnt --;
			if (NodeCnt == 0)
			{
				pFirst = NULL;
			}

			return LIST_SUCCESS;
		}
		pTmp2 = pTmp;
		pTmp = pTmp->pNext;
	}

	LOG_ERROR("Can't find the node.\n");
	return LIST_FAILURE;
}

template<class T>
fsl_osal_u32 List<T>::GetNodeCnt()
{
	return NodeCnt;
}

template<class T>
T *List<T>::GetNode(fsl_osal_u32 index)
{
	NODE *pTmp;
	fsl_osal_u32 i;

	if (NodeCnt == 0)
	{
		LOG_LOG("No node in the list.\n");
		return NULL;
	}

	if (index > NodeCnt - 1)
	{
		LOG_LOG("No so many node in the list.\n");
		return NULL;
	}

	pTmp = pFirst;
	for (i = 0; i < index; i ++)
	{
		pTmp = pTmp->pNext;
	}

	return pTmp->pT;
}

template<class T>
LSIT_RETURNTYPE List<T>::Replace(T *node, T *nodeNew)
{
	NODE *pTmp;

	if (node == NULL)
	{
		LOG_ERROR("Remove NULL from list.\n");
		return LIST_FAILURE;
	}

	pTmp = pFirst;
	while (pTmp != NULL)
	{
		if (pTmp->pT == node)
		{
			pTmp->pT = nodeNew;
			return LIST_SUCCESS;
		}
		pTmp = pTmp->pNext;
	}

	LOG_ERROR("Can't find the node.\n");
	return LIST_FAILURE;
}


template<class T> List<T>::~List()
{
	NODE *pTmp = pFirst, *pTmp2 = pFirst;

	while (pTmp != NULL)
	{
		pTmp2 = pTmp;
		pTmp = pTmp->pNext;
		FSL_DELETE(pTmp2);
	}
}


#endif
/* File EOF */
