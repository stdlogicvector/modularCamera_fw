#ifndef _UVC_CFG_H_
#define _UVC_CFG_H_

#define CAMERALINK

#if defined(MT9P031)

//ffplay -f dshow -vcodec rawvideo -video_size 2592x1944 -i video="modularCamera"

#define GRAY8

#define SENSOR_WIDTH	2592	// px
#define SENSOR_HEIGHT	1944	// px
#define SENSOR_DEPTH	1		// byte

#define INTERLACED		0
#define IMG_RATIO_X		0	// Zero for progressive scan
#define IMG_RATIO_Y		0	// Zero for progressive scan

#define MAX_FRAMERATE	14

// Still Image

#define STILL_WIDTH		SENSOR_WIDTH
#define STILL_HEIGHT	SENSOR_HEIGHT
#define STILL_DEPTH		SENSOR_DEPTH

// Full Speed (12 Mbps)

#define FS_WIDTH		SENSOR_WIDTH
#define FS_HEIGHT		SENSOR_HEIGHT
#define FS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define FS_FRAMERATE	1

// High Speed (480 Mbps)

#define HS_WIDTH		SENSOR_WIDTH
#define HS_HEIGHT		SENSOR_HEIGHT
#define HS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define HS_FRAMERATE	10

// Super Speed (5 Gbps)
#define SS_WIDTH		SENSOR_WIDTH
#define SS_HEIGHT		SENSOR_HEIGHT
#define SS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define SS_FRAMERATE	MAX_FRAMERATE

#elif defined(EV76C560)

//ffplay -f dshow -vcodec rawvideo -video_size 1280x1024 -i video="modularCamera"

#define GRAY8

#define SENSOR_WIDTH	1280	// px
#define SENSOR_HEIGHT	1024	// px
#define SENSOR_DEPTH	1		// byte

#define INTERLACED		0
#define IMG_RATIO_X		0	// Zero for progressive scan
#define IMG_RATIO_Y		0	// Zero for progressive scan

#define MAX_FRAMERATE	60

// Still Image

#define STILL_WIDTH		SENSOR_WIDTH
#define STILL_HEIGHT	SENSOR_HEIGHT
#define STILL_DEPTH		SENSOR_DEPTH

// Full Speed (12 Mbps)

#define FS_WIDTH		SENSOR_WIDTH
#define FS_HEIGHT		SENSOR_HEIGHT
#define FS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define FS_FRAMERATE	1

// High Speed (480 Mbps)

#define HS_WIDTH		SENSOR_WIDTH
#define HS_HEIGHT		SENSOR_HEIGHT
#define HS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define HS_FRAMERATE	30

// Super Speed (5 Gbps)
#define SS_WIDTH		SENSOR_WIDTH
#define SS_HEIGHT		SENSOR_HEIGHT
#define SS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define SS_FRAMERATE	MAX_FRAMERATE

#elif defined(PYTHON300)

//ffplay -f dshow -vcodec rawvideo -video_size 640x480 -i video="modularCamera"

#define GRAY8

#define SENSOR_WIDTH	640	// px
#define SENSOR_HEIGHT	480	// px
#define SENSOR_DEPTH	1	// byte

#define INTERLACED		0
#define IMG_RATIO_X		0	// Zero for progressive scan
#define IMG_RATIO_Y		0	// Zero for progressive scan

#define MAX_FRAMERATE	815	// (at full ROI)

// Still Image

#define STILL_WIDTH		SENSOR_WIDTH
#define STILL_HEIGHT	SENSOR_HEIGHT
#define STILL_DEPTH		SENSOR_DEPTH

// Full Speed (12 Mbps)

#define FS_WIDTH		SENSOR_WIDTH
#define FS_HEIGHT		SENSOR_HEIGHT
#define FS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define FS_FRAMERATE	4

// High Speed (480 Mbps)

#define HS_WIDTH		SENSOR_WIDTH
#define HS_HEIGHT		SENSOR_HEIGHT
#define HS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define HS_FRAMERATE	175

// Super Speed (5 Gbps)
#define SS_WIDTH		SENSOR_WIDTH
#define SS_HEIGHT		SENSOR_HEIGHT
#define SS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define SS_FRAMERATE	MAX_FRAMERATE


#elif defined(LINEARCCD)

//ffplay -f dshow -vcodec rawvideo -video_size 5000x120 -i video="modularCamera"

#define GRAY8

#define SENSOR_WIDTH	5000	// px
#define SENSOR_HEIGHT	120		// px
#define SENSOR_DEPTH	1		// byte

#define INTERLACED		0
#define IMG_RATIO_X		0	// Zero for progressive scan
#define IMG_RATIO_Y		0	// Zero for progressive scan

#define MAX_FRAMERATE	16

// Still Image

#define STILL_WIDTH		SENSOR_WIDTH
#define STILL_HEIGHT	SENSOR_HEIGHT
#define STILL_DEPTH		SENSOR_DEPTH

// Full Speed (12 Mbps)

#define FS_WIDTH		SENSOR_WIDTH
#define FS_HEIGHT		SENSOR_HEIGHT
#define FS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define FS_FRAMERATE	MAX_FRAMERATE

// High Speed (480 Mbps)

#define HS_WIDTH		SENSOR_WIDTH
#define HS_HEIGHT		SENSOR_HEIGHT
#define HS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define HS_FRAMERATE	MAX_FRAMERATE

// Super Speed (5 Gbps)
#define SS_WIDTH		SENSOR_WIDTH
#define SS_HEIGHT		SENSOR_HEIGHT
#define SS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define SS_FRAMERATE	MAX_FRAMERATE

#elif defined(HALLARRAY)

//ffplay -f dshow -vcodec rawvideo -video_size 64x64 -i video="modularCamera"

//#define YUV444
//#define YUV565
//#define RGB888
//#define RGB565
#define YUY2
//#define GRAY8

#define SENSOR_WIDTH	64		// px
#define SENSOR_HEIGHT	64		// px

#if defined(YUV444) || defined(RGB888)
	#define SENSOR_DEPTH	3		// byte
#elif defined(YUY2) || defined(YUV565) || defined(RGB565)
	#define SENSOR_DEPTH	2		// byte
#elif defined(GRAY8)
	#define SENSOR_DEPTH	1		// byte
#endif

#define INTERLACED		0
#define IMG_RATIO_X		0	// Zero for progressive scan
#define IMG_RATIO_Y		0	// Zero for progressive scan

#define MAX_FRAMERATE	1000

// Still Image

#define STILL_WIDTH		SENSOR_WIDTH
#define STILL_HEIGHT	SENSOR_HEIGHT
#define STILL_DEPTH		SENSOR_DEPTH

// Full Speed (12 Mbps)

#define FS_WIDTH		SENSOR_WIDTH
#define FS_HEIGHT		SENSOR_HEIGHT
#define FS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define FS_FRAMERATE	250

// High Speed (480 Mbps)

#define HS_WIDTH		SENSOR_WIDTH
#define HS_HEIGHT		SENSOR_HEIGHT
#define HS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define HS_FRAMERATE	MAX_FRAMERATE

// Super Speed (5 Gbps)
#define SS_WIDTH		SENSOR_WIDTH
#define SS_HEIGHT		SENSOR_HEIGHT
#define SS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define SS_FRAMERATE	MAX_FRAMERATE

#elif defined(ORION2K)

//ffplay -f dshow -vcodec rawvideo -video_size 2048x2048 -i video="modularCamera"

#define YUY2

#define SENSOR_WIDTH	2048	// px
#define SENSOR_HEIGHT	2048	// px
#define SENSOR_DEPTH	2		// byte

#define INTERLACED		0
#define IMG_RATIO_X		0	// Zero for progressive scan
#define IMG_RATIO_Y		0	// Zero for progressive scan

#define MAX_FRAMERATE	34

// Still Image

#define STILL_WIDTH		SENSOR_WIDTH
#define STILL_HEIGHT	SENSOR_HEIGHT
#define STILL_DEPTH		SENSOR_DEPTH

// Full Speed (12 Mbps)

#define FS_WIDTH		SENSOR_WIDTH
#define FS_HEIGHT		SENSOR_HEIGHT
#define FS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define FS_FRAMERATE	1

// High Speed (480 Mbps)

#define HS_WIDTH		SENSOR_WIDTH
#define HS_HEIGHT		SENSOR_HEIGHT
#define HS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define HS_FRAMERATE	5

// Super Speed (5 Gbps)
#define SS_WIDTH		SENSOR_WIDTH
#define SS_HEIGHT		SENSOR_HEIGHT
#define SS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define SS_FRAMERATE	MAX_FRAMERATE

#elif defined(AISC110C)

// ffplay -f dshow -vcodec rawvideo -video_size 80x120 -i video="modularCamera"

#undef YUY2
#undef RGB565
#undef RGBP
#undef GRAY16
#define GRAY8

#define SENSOR_WIDTH	80		// px
#define SENSOR_HEIGHT	120		// px
#define SENSOR_DEPTH	1		// byte

#define INTERLACED		0
#define IMG_RATIO_X		0	// Zero for progressive scan
#define IMG_RATIO_Y		0	// Zero for progressive scan

#define MAX_FRAMERATE	40000

// Still Image

#define STILL_WIDTH		SENSOR_WIDTH
#define STILL_HEIGHT	SENSOR_HEIGHT
#define STILL_DEPTH		SENSOR_DEPTH

// Full Speed (12 Mbps)

#define FS_WIDTH		SENSOR_WIDTH
#define FS_HEIGHT		SENSOR_HEIGHT
#define FS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define FS_FRAMERATE	100

// High Speed (480 Mbps)

#define HS_WIDTH		SENSOR_WIDTH
#define HS_HEIGHT		SENSOR_HEIGHT
#define HS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define HS_FRAMERATE	5000

// Super Speed (5 Gbps)
#define SS_WIDTH		SENSOR_WIDTH
#define SS_HEIGHT		SENSOR_HEIGHT
#define SS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define SS_FRAMERATE	MAX_FRAMERATE

#elif defined(ADV7182)

#define YUY2
#undef RGB565
#undef RGBP
#undef GRAY16
#undef GRAY8

#define SENSOR_WIDTH	720		// px
//#define SENSOR_HEIGHT	486		// px
//#define SENSOR_HEIGHT 576
#define SENSOR_HEIGHT 507
#define SENSOR_DEPTH	2		// byte

#define INTERLACED		0x25	// Interlaced, Field 1 first, Regular Pattern of Fields 1 and 2
#define IMG_RATIO_X		4
#define IMG_RATIO_Y		3

//#define INTERLACED		0
//#define IMG_RATIO_X		0	// Zero for progressive scan
//#define IMG_RATIO_Y		0	// Zero for progressive scan

#define MAX_FRAMERATE	30

// Still Image

#define STILL_WIDTH		SENSOR_WIDTH
#define STILL_HEIGHT	SENSOR_HEIGHT
#define STILL_DEPTH		SENSOR_DEPTH

// Full Speed (12 Mbps)

#define FS_WIDTH		(SENSOR_WIDTH)
#define FS_HEIGHT		(SENSOR_HEIGHT)
#define FS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define FS_FRAMERATE	1

// High Speed (480 Mbps)

#define HS_WIDTH		(SENSOR_WIDTH)
#define HS_HEIGHT		(SENSOR_HEIGHT)
#define HS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define HS_FRAMERATE	MAX_FRAMERATE

// Super Speed (5 Gbps)
#define SS_WIDTH		(SENSOR_WIDTH)
#define SS_HEIGHT		(SENSOR_HEIGHT)
#define SS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define SS_FRAMERATE	MAX_FRAMERATE

#elif defined(CAMERALINK)

#undef YUY2
#define RGB565
#undef RGBP
#undef GRAY16
#undef GRAY8

#define SENSOR_WIDTH	1548	// px
#define SENSOR_HEIGHT	1548	// px
#define SENSOR_DEPTH	2		// byte

#define INTERLACED		0
#define IMG_RATIO_X		0	// Zero for progressive scan
#define IMG_RATIO_Y		0	// Zero for progressive scan

#define MAX_FRAMERATE	30

// Still Image

#define STILL_WIDTH		SENSOR_WIDTH
#define STILL_HEIGHT	SENSOR_HEIGHT
#define STILL_DEPTH		SENSOR_DEPTH

// Full Speed (12 Mbps)

#define FS_WIDTH		(SENSOR_WIDTH/2)
#define FS_HEIGHT		(SENSOR_HEIGHT/2)
#define FS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define FS_FRAMERATE	1

// High Speed (480 Mbps)

#define HS_WIDTH		(SENSOR_WIDTH/2)
#define HS_HEIGHT		(SENSOR_WIDTH/2)
#define HS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define HS_FRAMERATE	15

// Super Speed (5 Gbps)
#define SS_WIDTH		SENSOR_WIDTH
#define SS_HEIGHT		SENSOR_HEIGHT
#define SS_COLORDEPTH	(SENSOR_DEPTH * 8)
#define SS_FRAMERATE	MAX_FRAMERATE

#else
#error No Config selected!
#endif

// v4l2-ctl -d /dev/video2 -D --list-formats-ext

// sudo echo 0xffff > /sys/module/uvcvideo/parameters/trace
// sudo echo 0 > /sys/module/uvcvideo/parameters/trace

#endif /* _UVC_CFG_H_ */
