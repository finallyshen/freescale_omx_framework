/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */
#include "Log.h"
#include "Mem.h"
#include "RegistryAnalyser.h"

RegistryAnalyser::RegistryAnalyser()
{
	pfile = NULL;
	FileReadEnd = E_FSL_OSAL_FALSE;
	BufferDataLen = 0;
	UseOffset = 0;
}

RegistryAnalyser::~RegistryAnalyser()
{
}

REG_ERRORTYPE RegistryAnalyser::Open(
        fsl_osal_char *file_name) 
{
	efsl_osal_return_type_t ret;

	if (file_name == NULL)
	{
		LOG_ERROR("Input file name is NULL.");
		return REG_INVALID_REG;
	}

	ret = fsl_osal_fopen(file_name, "r", (fsl_osal_file *)&pfile);
	if (ret != E_FSL_OSAL_SUCCESS)
	{
		LOG_ERROR("Can't open file: %s\n", file_name);
		return REG_FAILURE;
	}

	return REG_SUCCESS;
}

REG_ERRORTYPE RegistryAnalyser::Close() 
{
	if (pfile == NULL)
	{
		LOG_ERROR("No file need close.\n");
		return REG_FAILURE;
	}

	fsl_osal_fclose(pfile);

	return REG_SUCCESS;
}

List<REG_ENTRY> *RegistryAnalyser::GetNextEntry()
{
	fsl_osal_char symbol;
	efsl_osal_bool bSkip = E_FSL_OSAL_FALSE;
	fsl_osal_char symbolBuffer[ITEM_VALUE_LEN] = {0}, *pStrSeg;
	efsl_osal_bool EntryFounded = E_FSL_OSAL_FALSE;
	fsl_osal_s32 symbolCnt = 0;
	efsl_osal_return_type_t ret;
	REG_ENTRY *pRegEntry;

	if (pfile == NULL)
	{
		LOG_ERROR("No file to read.\n");
		return &RegList;
	}


	/** Remove previous entry */
	RemoveEntry();
	
	while (BufferDataLen - UseOffset > 0 || FileReadEnd == E_FSL_OSAL_FALSE)
	{
		if (BufferDataLen - UseOffset == 0)
		{
			ret = fsl_osal_fread(readBuffer, FILE_READ_SIZE, pfile, &BufferDataLen);
			if (ret != E_FSL_OSAL_SUCCESS && ret != E_FSL_OSAL_EOF)
			{
				LOG_ERROR("file read error.");
				return &RegList;
			}

			if (BufferDataLen < FILE_READ_SIZE)
			{
				FileReadEnd = E_FSL_OSAL_TRUE;
			}

			UseOffset = 0;
		}
		
		symbol = readBuffer[UseOffset];
		UseOffset ++;

		if (bSkip == E_FSL_OSAL_TRUE)
		{
			if (symbol == '\n')
			{
				bSkip = E_FSL_OSAL_FALSE;
			}
		}
		else
		{
			if (symbol == '#')
			{
				bSkip = E_FSL_OSAL_TRUE;
			}
			else if (symbol == '\t'
					|| symbol == ' '
					|| symbol == '\n'
					|| symbol == '\r')
			{
			}
			else if (symbol == '@')
			{
				EntryFounded = E_FSL_OSAL_TRUE;
			}
			else if (symbol == '$')
			{
				break;
			}
			else if (symbol == ';')
			{
				fsl_osal_char *pLast = NULL;
				if (symbolCnt != 0 && ((pStrSeg = fsl_osal_strtok_r(symbolBuffer, "=", &pLast)) != NULL))
				{
					pRegEntry = FSL_NEW(REG_ENTRY, ());
					fsl_osal_memset(pRegEntry, 0, sizeof(REG_ENTRY));
					fsl_osal_strcpy(pRegEntry->name, pStrSeg);
					pStrSeg = fsl_osal_strtok_r(NULL, "=", &pLast);
					fsl_osal_strcpy(pRegEntry->value, pStrSeg);
					fsl_osal_memset(symbolBuffer, 0, ITEM_VALUE_LEN);
					symbolCnt = 0;
					RegList.Add(pRegEntry);
				}
			}
			else
			{
				if (EntryFounded == E_FSL_OSAL_TRUE)
				{
					symbolBuffer[symbolCnt] = symbol;
					symbolCnt ++;
				}
			}
		}
	}
	return &RegList;
}

REG_ERRORTYPE RegistryAnalyser::RemoveEntry() 
{
	fsl_osal_u32 NodeCnt = RegList.GetNodeCnt();
	fsl_osal_s32 i;
	REG_ENTRY *pRegEntry;

	if (NodeCnt == 0)
	{
		return REG_SUCCESS;
	}

	for (i = NodeCnt - 1; i >= 0; i --)
	{
		pRegEntry = RegList.GetNode(i);
		RegList.Remove(pRegEntry);
		FSL_DELETE(pRegEntry);
	}

	return REG_SUCCESS;
}

/* File EOF */
