/*
 ***************************************************************************
 * Copyright (c) 2007-2009,2012 Freescale Semiconductor Inc.,
 * All Rights Reserved.
 *
 * The following programs are the sole property of Freescale Semiconductor Inc.,
 * and contain its proprietary and confidential information.
 ***************************************************************************
 ****************************************************************************
 * ANSI C source code
 *
 * Project Name : Parametric EQ PPP
 * File Name    : db2dc_16bit.h
 *
 * Description  : Using the following formula to convert dB value to dc, which
 *                ranging from -20dB~20dB, step by 0.5dB
 *
 *                n= -20:0.5:20;
 *                y = round(power(10,n/20)./16.*(2.^15));
 *
 * FREESCALE CONFIDENTIAL PROPRIETARY
 ***************************************************************************/
 /***************************************************************************
 *
 *   (C) 2007 FREESCALE SEMICONDUCTOR.
 *
 *   CHANGE HISTORY
 *    dd/mm/yy        Code Ver      CR          Author     Description      
 *    --------        -------      -------      ------     ----------- 
 *                     0.1     	       Tao Jun    created file
 *                            
 **************************************************************************/  
         205,
         217,
         230,
         243,
         258,
         273,
         289,
         306,
         325,
         344,
         364,
         386,
         409,
         433,
         458,
         486,
         514,
         545,
         577,
         611,
         648,
         686,
         727,
         770,
         815,
         864,
         915,
         969,
        1026,
        1087,
        1152,
        1220,
        1292,
        1369,
        1450,
        1536,
        1627,
        1723,
        1825,
        1933,
        2048,
        2169,
        2298,
        2434,
        2578,
        2731,
        2893,
        3064,
        3246,
        3438,
        3642,
        3858,
        4086,
        4328,
        4585,
        4857,
        5144,
        5449,
        5772,
        6114,
        6476,
        6860,
        7267,
        7697,
        8153,
        8636,
        9148,
        9690,
       10264,
       10873,
       11517,
       12199,
       12922,
       13688,
       14499,
       15358,
       16268,
       17232,
       18253,
       19334,
       20480
