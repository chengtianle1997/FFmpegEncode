#include <stdio.h>
#define __STDC_CONSTANT_MACROS
#define _CRT_SECURE_NO_WARNINGS

#include "iostream"
#include "CameraApi.h"

#ifdef _WIN64
#pragma comment(lib, "MVCAMSDK_X64.lib")
#else
#pragma comment(lib, "..\\MVCAMSDK.lib")
#endif
#include "..//include//CameraApi.h"

#ifdef _WIN64

//Windows

extern "C"

{

#include "libavutil/opt.h"

#include "libavcodec/avcodec.h"

#include "libavutil/imgutils.h"

};

#else

//Linux...

#ifdef __cplusplus

extern "C"

{

#endif

#include <libavutil/opt.h>

#include <libavcodec/avcodec.h>

#include <libavutil/imgutils.h>

#ifdef __cplusplus

};

#endif

#endif

#pragma comment(lib, "avcodec.lib")   

#pragma comment(lib, "avutil.lib")  

//test different codec

#define TEST_H264  1

#define TEST_HEVC  0

UINT            m_threadID;		//图像抓取线程的ID
HANDLE          m_hDispThread;	//图像抓取线程的句柄
BOOL            m_bExit = FALSE;//用来通知图像抓取线程结束
CameraHandle    m_hCamera;		//相机句柄，多个相机同时使用时，可以用数组代替	
BYTE*           m_pFrameBuffer; //用于将原始图像数据转换为RGB的缓冲区
tSdkFrameHead   m_sFrInfo;		//用于保存当前图像帧的帧头信息

int	            m_iDispFrameNum;	//用于记录当前已经显示的图像帧的数量
float           m_fDispFps;			//显示帧率
float           m_fCapFps;			//捕获帧率
tSdkFrameStatistic  m_sFrameCount;
tSdkFrameStatistic  m_sFrameLast;
int					m_iTimeLast;
char		    g_CameraName[64];
UINT WINAPI uiDisplayThread(LPVOID lpParam)
{
	tSdkFrameHead 	sFrameInfo;
	CameraHandle    hCamera = (CameraHandle)lpParam;
	BYTE*			pbyBuffer;
	CameraSdkStatus status;
	//IplImage *iplImage = NULL;

	while (!m_bExit)
	{

		if (CameraGetImageBuffer(hCamera, &sFrameInfo, &pbyBuffer, 1000) == CAMERA_STATUS_SUCCESS)
		{
			//将获得的原始数据转换成RGB格式的数据，同时经过ISP模块，对图像进行降噪，边沿提升，颜色校正等处理。
			//我公司大部分型号的相机，原始数据都是Bayer格式的
			status = CameraImageProcess(hCamera, pbyBuffer, m_pFrameBuffer, &sFrameInfo);//连续模式

			//分辨率改变了，则刷新背景
			if (m_sFrInfo.iWidth != sFrameInfo.iWidth || m_sFrInfo.iHeight != sFrameInfo.iHeight)
			{
				m_sFrInfo.iWidth = sFrameInfo.iWidth;
				m_sFrInfo.iHeight = sFrameInfo.iHeight;
				//图像大小改变，通知重绘
			}

			if (status == CAMERA_STATUS_SUCCESS)
			{
				//调用SDK封装好的显示接口来显示图像,您也可以将m_pFrameBuffer中的RGB数据通过其他方式显示，比如directX,OpengGL,等方式。
				//CameraImageOverlay(hCamera, m_pFrameBuffer, &sFrameInfo);
#if 0
				if (iplImage)
				{
					cvReleaseImageHeader(&iplImage);
				}
				iplImage = cvCreateImageHeader(cvSize(sFrameInfo.iWidth, sFrameInfo.iHeight), IPL_DEPTH_8U, sFrameInfo.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? 1 : 3);
				cvSetData(iplImage, m_pFrameBuffer, sFrameInfo.iWidth*(sFrameInfo.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? 1 : 3));
				cvShowImage(g_CameraName, iplImage);//展示初始捕获图像
				//对捕获图像进行处理		
				IplImage *imgOrign = 0;
				cvCopy(iplImage, imgOrign, NULL);
				IplImage *imgDest = 0;
				LaserRange laservision;
				LaserRange::RangeResult *temp = laservision.GetRange(imgOrign, imgDest);






				cv::Mat matImage(
					cvSize(sFrameInfo.iWidth, sFrameInfo.iHeight),
					sFrameInfo.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? CV_8UC1 : CV_8UC3,
					m_pFrameBuffer
				);
				imshow(g_CameraName, matImage);
#endif
				m_iDispFrameNum++;

			}

			//在成功调用CameraGetImageBuffer后，必须调用CameraReleaseImageBuffer来释放获得的buffer。
			//否则再次调用CameraGetImageBuffer时，程序将被挂起，知道其他线程中调用CameraReleaseImageBuffer来释放了buffer
			CameraReleaseImageBuffer(hCamera, pbyBuffer);
			memcpy(&m_sFrInfo, &sFrameInfo, sizeof(tSdkFrameHead));
		}
#if 0
		int c = waitKey(10);

		if (c == 'q' || c == 'Q' || (c & 255) == 27)
		{
			m_bExit = TRUE;
			break;
		}
#endif
	}


	_endthreadex(0);
	return 0;

}
bool InitCamera()
{
	tSdkCameraDevInfo sCameraList[10];
	INT iCameraNums;
	CameraSdkStatus status;
	tSdkCameraCapbility sCameraInfo;

	//枚举设备，获得设备列表
	iCameraNums = 10;//调用CameraEnumerateDevice前，先设置iCameraNums = 10，表示最多只读取10个设备，如果需要枚举更多的设备，请更改sCameraList数组的大小和iCameraNums的值

	if (CameraEnumerateDevice(sCameraList, &iCameraNums) != CAMERA_STATUS_SUCCESS || iCameraNums == 0)
	{
		printf("No camera was found!");
		return FALSE;
	}

	//该示例中，我们只假设连接了一个相机。因此，只初始化第一个相机。(-1,-1)表示加载上次退出前保存的参数，如果是第一次使用该相机，则加载默认参数.
	//In this demo ,we just init the first camera.
	if ((status = CameraInit(&sCameraList[0], -1, -1, &m_hCamera)) != CAMERA_STATUS_SUCCESS)
	{
		char msg[128];
		sprintf_s(msg, "Failed to init the camera! Error code is %d", status);
		printf(msg);
		printf(CameraGetErrorString(status));
		return FALSE;
	}


	//Get properties description for this camera.
	CameraGetCapability(m_hCamera, &sCameraInfo);//"获得该相机的特性描述"

	m_pFrameBuffer = (BYTE *)CameraAlignMalloc(sCameraInfo.sResolutionRange.iWidthMax*sCameraInfo.sResolutionRange.iWidthMax * 3, 16);

	if (sCameraInfo.sIspCapacity.bMonoSensor)
	{
		CameraSetIspOutFormat(m_hCamera, CAMERA_MEDIA_TYPE_MONO8);
	}

	strcpy_s(g_CameraName, sCameraList[0].acFriendlyName);
	//镜像及旋转
	CameraSetMirror(m_hCamera, 0, 1);
	CameraSetRotate(m_hCamera, 2);
	CameraCreateSettingPage(m_hCamera, NULL,
		g_CameraName, NULL, NULL, 0);//"通知SDK内部建该相机的属性页面";

#ifdef USE_CALLBACK_GRAB_IMAGE //如果要使用回调函数方式，定义USE_CALLBACK_GRAB_IMAGE这个宏
	//Set the callback for image capture
	CameraSetCallbackFunction(m_hCamera, GrabImageCallback, 0, NULL);//"设置图像抓取的回调函数";
#else
	m_hDispThread = (HANDLE)_beginthreadex(NULL, 0, &uiDisplayThread, (PVOID)m_hCamera, 0, &m_threadID);
#endif

	CameraPlay(m_hCamera);

	CameraShowSettingPage(m_hCamera, TRUE);//TRUE显示相机配置界面。FALSE则隐藏。

	//while (m_bExit != TRUE)
	//{
	//	waitKey(10);
	//}

	CameraUnInit(m_hCamera);

	CameraAlignFree(m_pFrameBuffer);

	//destroyWindow(g_CameraName);

#ifdef USE_CALLBACK_GRAB_IMAGE
	if (g_iplImage)
	{
		cvReleaseImageHeader(&g_iplImage);
	}
#endif
}

int main(int argc, char* argv[])

{
	//InitCamera();

	//<span style = "white-space:pre">	< / span>
	AVCodec *pCodec;

	AVCodecContext *pCodecCtx = NULL;

	int i, ret, got_output;

	FILE *fp_in;

	FILE *fp_out;

	AVFrame *pFrame;

	AVPacket pkt;

	

	int y_size;

	int framecnt = 0;
	char filename_in[] = "ds_480x272.yuv";





#if TEST_HEVC

	AVCodecID codec_id = AV_CODEC_ID_HEVC;

	char filename_out[] = "ds.hevc";

#else
	//转码后的视频格式
	AVCodecID codec_id = AV_CODEC_ID_H264;

	char filename_out[] = "ds.h264";

#endif

	int in_w = 480, in_h = 272;

	int framenum = 100;
	//注册所有编译器
	avcodec_register_all();
	//查找编码器
	pCodec = avcodec_find_encoder(codec_id);

	if (!pCodec) {

		printf("Codec not found\n");

		return -1;

	}
	//申请AVcodecContext空间
	pCodecCtx = avcodec_alloc_context3(pCodec);

	if (!pCodecCtx) {

		printf("Could not allocate video codec context\n");

		return -1;

	}

	pCodecCtx->bit_rate = 400000;

	pCodecCtx->width = in_w;

	pCodecCtx->height = in_h;

	pCodecCtx->time_base.num = 1;

	pCodecCtx->time_base.den = 25;

	pCodecCtx->gop_size = 10;

	pCodecCtx->max_b_frames = 1;

	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	if (codec_id == AV_CODEC_ID_H264)
		av_opt_set(pCodecCtx->priv_data, "preset", "slow", 0);

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {

		printf("Could not open codec\n");

		return -1;

	}


	//frame申请空间
	pFrame = av_frame_alloc();

	if (!pFrame) {

		printf("Could not allocate video frame\n");

		return -1;

	}

	pFrame->format = pCodecCtx->pix_fmt;

	pFrame->width = pCodecCtx->width;

	pFrame->height = pCodecCtx->height;




	ret = av_image_alloc(pFrame->data, pFrame->linesize, pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 16);
	/*
	pointers[4]：保存图像通道的地址。如果是RGB，则前三个指针分别指向R, G, B的内存地址。第四个指针保留不用
		linesizes[4]：保存图像每个通道的内存对齐的步长，即一行的对齐内存的宽度，此值大小等于图像宽度。
	    w : 要申请内存的图像宽度。
		h : 要申请内存的图像高度。
		pix_fmt : 要申请内存的图像的像素格式。
		align : 用于内存对齐的值。
		返回值：所申请的内存空间的总大小。如果是负值，表示申请失败。
    	/**
		 * Allocate an image with size w and h and pixel format pix_fmt, and

		 * fill pointers and linesizes accordingly.

		 * The allocated image buffer has to be freed by using

		 * av_freep(&pointers[0]).

		 *

		 * @param align the value to use for buffer size alignment

		 * @return the size in bytes required for the image buffer, a negative

		 * error code in case of failure

		 */
	if (ret < 0) {

		printf("Could not allocate raw picture buffer\n");

		return -1;

	}

	//Input raw data
	fp_in = fopen(filename_in, "rb");

	if (!fp_in) {

		printf("Could not open %s\n", filename_in);

		return -1;

	}

	//Output bitstream

	fp_out = fopen(filename_out, "wb");

	if (!fp_out) {

		printf("Could not open %s\n", filename_out);

		return -1;

	}





	y_size = pCodecCtx->width * pCodecCtx->height;

	//Encode

	for (i = 0; i < framenum; i++) {

		av_init_packet(&pkt);

		pkt.data = NULL;    // packet data will be allocated by the encoder

		pkt.size = 0;

		//Read raw YUV data

		if (fread(pFrame->data[0], 1, y_size, fp_in) <= 0 ||		// Y

			fread(pFrame->data[1], 1, y_size / 4, fp_in) <= 0 ||	// U

			fread(pFrame->data[2], 1, y_size / 4, fp_in) <= 0) {	// V

			return -1;

		}
		else if (feof(fp_in)) {

			break;

		}





		pFrame->pts = i;

		/* encode the image */

		ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_output);

		if (ret < 0) {

			printf("Error encoding frame\n");

			return -1;

		}

		if (got_output) {

			printf("Succeed to encode frame: %5d\tsize:%5d\n", framecnt, pkt.size);

			framecnt++;

			fwrite(pkt.data, 1, pkt.size, fp_out);

			av_free_packet(&pkt);

		}

	}

	//Flush Encoder

	for (got_output = 1; got_output; i++) {

		ret = avcodec_encode_video2(pCodecCtx, &pkt, NULL, &got_output);

		if (ret < 0) {

			printf("Error encoding frame\n");

			return -1;

		}

		if (got_output) {

			printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", pkt.size);

			fwrite(pkt.data, 1, pkt.size, fp_out);

			av_free_packet(&pkt);

		}

	}





	fclose(fp_out);

	avcodec_close(pCodecCtx);

	av_free(pCodecCtx);

	av_freep(&pFrame->data[0]);

	av_frame_free(&pFrame);





	return 0;

}