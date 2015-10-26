#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include "hi_comm_vb.h"

#define SYS_ALIGN_WIDTH 16
#define PRT_ERR(fmt...) \
	do { \
		fprintf(stderr, "[%s]-%d: ", __FUNCTION__, __LINE__); \
		fprintf(stderr, fmt); \
	} while(0)

#define PIXEL_FORMAT         PIXEL_FORMAT_YUV_SEMIPLANAR_420

#define VI_SUBCHN_2_W 720
#define VI_SUBCHN_2_H  640

type def enum vi_mode_e
{
	VI_MODE_16_Cif,
	VI_MODE_4_720P,
	VI_MODE_4_1080P
} VI_MODE_E;

typedef struct vi_param_s
{
    HI_S32 s32ViDevCnt;		// VI Dev Total Count
    HI_S32 s32ViDevInterval;	// Vi Dev Interval
    HI_S32 s32ViChnCnt;		// Vi Chn Total Count
    HI_S32 s32ViChnInterval;	// VI Chn Interval
}VI_PARAM_S;

typedef enum vi_chn_set_e
{
    VI_CHN_SET_NORMAL = 0, /* mirror, filp close */
    VI_CHN_SET_MIRROR,      /* open MIRROR */
    VI_CHN_SET_FILP		/* open filp */
}VI_CHN_SET_E;

/* init mpp sys and vb */
int comm_sys_init(VB_CONF_S *pvb_conf);

/* set sys memory config */
int comm_vi_memconfig(VI_MODE_E vi_mode);

/* vi start */
int comm_vi_start(VI_MODE_E vi_mode, VIDEO_NORM_E norm);

/* vda start */
int comm_vda_odstart(int vda_chn, int  vi_chn, SIZE_S *pstSize);
#ifdef __cplusplus
}
#endif
