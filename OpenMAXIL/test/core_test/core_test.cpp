/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "OMX_Core.h"
#include "OMX_ContentPipe.h"

int main(int argc, char *argv[])
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(argc < 2)
	{
        printf("Usage: ./bin <in_file>\n");
        return 0;
    }

	ret = OMX_Init();
	if (ret != OMX_ErrorNone)
	{
		printf("OMX init error.\n");
		return 0;
	}

	OMX_U8 compName[3][128];
	memset(compName, 0, 3*128);
	char role[] = "parser.mp3";
	OMX_U32 compCnt = 0;

	ret = OMX_GetComponentsOfRole(role, &compCnt, (OMX_U8 **)compName);
	if (ret != OMX_ErrorNone)
	{
		printf("OMX get component of role error.\n");
		return 0;
	}

	CP_PIPETYPE *hPipe;
    ret =  OMX_GetContentPipe((void **)&hPipe, "LOCAL_FILE_PIPE_NEW");
	if (ret != OMX_ErrorNone)
	{
		printf("OMX get content pipe error.\n");
		return 0;
	}

    return 1;
}

/* File EOF */
