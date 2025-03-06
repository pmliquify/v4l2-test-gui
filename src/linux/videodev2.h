#pragma once

typedef unsigned int __u32;

/*
 *	M I S C E L L A N E O U S
 */

/*  Four-character-code (FOURCC) */
#define v4l2_fourcc(a, b, c, d)\
	((__u32)(a) | ((__u32)(b) << 8) | ((__u32)(c) << 16) | ((__u32)(d) << 24))

#define V4L2_PIX_FMT_GREY     v4l2_fourcc('G', 'R', 'E', 'Y') /*  8  Greyscale     */
#define V4L2_PIX_FMT_Y10      v4l2_fourcc('Y', '1', '0', ' ') /* 10  Greyscale     */
#define V4L2_PIX_FMT_Y12      v4l2_fourcc('Y', '1', '2', ' ') /* 12  Greyscale     */
 
#define V4L2_PIX_FMT_SBGGR8   v4l2_fourcc('B', 'A', '8', '1') /*  8  BGBG.. GRGR.. */
#define V4L2_PIX_FMT_SGBRG8   v4l2_fourcc('G', 'B', 'R', 'G') /*  8  GBGB.. RGRG.. */
#define V4L2_PIX_FMT_SGRBG8   v4l2_fourcc('G', 'R', 'B', 'G') /*  8  GRGR.. BGBG.. */
#define V4L2_PIX_FMT_SRGGB8   v4l2_fourcc('R', 'G', 'G', 'B') /*  8  RGRG.. GBGB.. */
 
#define V4L2_PIX_FMT_SBGGR10  v4l2_fourcc('B', 'G', '1', '0') /* 10  BGBG.. GRGR.. */
#define V4L2_PIX_FMT_SGBRG10  v4l2_fourcc('G', 'B', '1', '0') /* 10  GBGB.. RGRG.. */
#define V4L2_PIX_FMT_SGRBG10  v4l2_fourcc('B', 'A', '1', '0') /* 10  GRGR.. BGBG.. */
#define V4L2_PIX_FMT_SRGGB10  v4l2_fourcc('R', 'G', '1', '0') /* 10  RGRG.. GBGB.. */
 
#define V4L2_PIX_FMT_SBGGR12  v4l2_fourcc('B', 'G', '1', '2') /* 12  BGBG.. GRGR.. */
#define V4L2_PIX_FMT_SGBRG12  v4l2_fourcc('G', 'B', '1', '2') /* 12  GBGB.. RGRG.. */
#define V4L2_PIX_FMT_SGRBG12  v4l2_fourcc('B', 'A', '1', '2') /* 12  GRGR.. BGBG.. */
#define V4L2_PIX_FMT_SRGGB12  v4l2_fourcc('R', 'G', '1', '2') /* 12  RGRG.. GBGB.. */
 
#define V4L2_PIX_FMT_YUYV     v4l2_fourcc('Y', 'U', 'Y', 'V') /* 16  YUV 4:2:2     */

#define V4L2_PIX_FMT_SBGGR10P v4l2_fourcc('p', 'B', 'A', 'A')
#define V4L2_PIX_FMT_SGBRG10P v4l2_fourcc('p', 'G', 'A', 'A')
#define V4L2_PIX_FMT_SGRBG10P v4l2_fourcc('p', 'g', 'A', 'A')
#define V4L2_PIX_FMT_SRGGB10P v4l2_fourcc('p', 'R', 'A', 'A')
