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

UINT            m_threadID;		//ͼ��ץȡ�̵߳�ID
HANDLE          m_hDispThread;	//ͼ��ץȡ�̵߳ľ��
BOOL            m_bExit = FALSE;//����֪ͨͼ��ץȡ�߳̽���
CameraHandle    m_hCamera;		//��������������ͬʱʹ��ʱ���������������	
BYTE*           m_pFrameBuffer; //���ڽ�ԭʼͼ������ת��ΪRGB�Ļ�����
tSdkFrameHead   m_sFrInfo;		//���ڱ��浱ǰͼ��֡��֡ͷ��Ϣ

int	            m_iDispFrameNum;	//���ڼ�¼��ǰ�Ѿ���ʾ��ͼ��֡������
float           m_fDispFps;			//��ʾ֡��
float           m_fCapFps;			//����֡��
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
			//����õ�ԭʼ����ת����RGB��ʽ�����ݣ�ͬʱ����ISPģ�飬��ͼ����н��룬������������ɫУ���ȴ���
			//�ҹ�˾�󲿷��ͺŵ������ԭʼ���ݶ���Bayer��ʽ��
			status = CameraImageProcess(hCamera, pbyBuffer, m_pFrameBuffer, &sFrameInfo);//����ģʽ

			//�ֱ��ʸı��ˣ���ˢ�±���
			if (m_sFrInfo.iWidth != sFrameInfo.iWidth || m_sFrInfo.iHeight != sFrameInfo.iHeight)
			{
				m_sFrInfo.iWidth = sFrameInfo.iWidth;
				m_sFrInfo.iHeight = sFrameInfo.iHeight;
				//ͼ���С�ı䣬֪ͨ�ػ�
			}

			if (status == CAMERA_STATUS_SUCCESS)
			{
				//����SDK��װ�õ���ʾ�ӿ�����ʾͼ��,��Ҳ���Խ�m_pFrameBuffer�е�RGB����ͨ��������ʽ��ʾ������directX,OpengGL,�ȷ�ʽ��
				//CameraImageOverlay(hCamera, m_pFrameBuffer, &sFrameInfo);
#if 0
				if (iplImage)
				{
					cvReleaseImageHeader(&iplImage);
				}
				iplImage = cvCreateImageHeader(cvSize(sFrameInfo.iWidth, sFrameInfo.iHeight), IPL_DEPTH_8U, sFrameInfo.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? 1 : 3);
				cvSetData(iplImage, m_pFrameBuffer, sFrameInfo.iWidth*(sFrameInfo.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? 1 : 3));
				cvShowImage(g_CameraName, iplImage);//չʾ��ʼ����ͼ��
				//�Բ���ͼ����д���		
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

			//�ڳɹ�����CameraGetImageBuffer�󣬱������CameraReleaseImageBuffer���ͷŻ�õ�buffer��
			//�����ٴε���CameraGetImageBufferʱ�����򽫱�����֪�������߳��е���CameraReleaseImageBuffer���ͷ���buffer
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

	//ö���豸������豸�б�
	iCameraNums = 10;//����CameraEnumerateDeviceǰ��������iCameraNums = 10����ʾ���ֻ��ȡ10���豸�������Ҫö�ٸ�����豸�������sCameraList����Ĵ�С��iCameraNums��ֵ

	if (CameraEnumerateDevice(sCameraList, &iCameraNums) != CAMERA_STATUS_SUCCESS || iCameraNums == 0)
	{
		printf("No camera was found!");
		return FALSE;
	}

	//��ʾ���У�����ֻ����������һ���������ˣ�ֻ��ʼ����һ�������(-1,-1)��ʾ�����ϴ��˳�ǰ����Ĳ���������ǵ�һ��ʹ�ø�����������Ĭ�ϲ���.
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
	CameraGetCapability(m_hCamera, &sCameraInfo);//"��ø��������������"

	m_pFrameBuffer = (BYTE *)CameraAlignMalloc(sCameraInfo.sResolutionRange.iWidthMax*sCameraInfo.sResolutionRange.iWidthMax * 3, 16);

	if (sCameraInfo.sIspCapacity.bMonoSensor)
	{
		CameraSetIspOutFormat(m_hCamera, CAMERA_MEDIA_TYPE_MONO8);
	}

	strcpy_s(g_CameraName, sCameraList[0].acFriendlyName);
	//������ת
	CameraSetMirror(m_hCamera, 0, 1);
	CameraSetRotate(m_hCamera, 2);
	CameraCreateSettingPage(m_hCamera, NULL,
		g_CameraName, NULL, NULL, 0);//"֪ͨSDK�ڲ��������������ҳ��";

#ifdef USE_CALLBACK_GRAB_IMAGE //���Ҫʹ�ûص�������ʽ������USE_CALLBACK_GRAB_IMAGE�����
	//Set the callback for image capture
	CameraSetCallbackFunction(m_hCamera, GrabImageCallback, 0, NULL);//"����ͼ��ץȡ�Ļص�����";
#else
	m_hDispThread = (HANDLE)_beginthreadex(NULL, 0, &uiDisplayThread, (PVOID)m_hCamera, 0, &m_threadID);
#endif

	CameraPlay(m_hCamera);

	CameraShowSettingPage(m_hCamera, TRUE);//TRUE��ʾ������ý��档FALSE�����ء�

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
	//ת������Ƶ��ʽ
	AVCodecID codec_id = AV_CODEC_ID_H264;

	char filename_out[] = "ds.h264";

#endif

	int in_w = 480, in_h = 272;

	int framenum = 100;
	//ע�����б�����
	avcodec_register_all();
	//���ұ�����
	pCodec = avcodec_find_encoder(codec_id);

	if (!pCodec) {

		printf("Codec not found\n");

		return -1;

	}
	//����AVcodecContext�ռ�
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


	//frame����ռ�
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
	pointers[4]������ͼ��ͨ���ĵ�ַ�������RGB����ǰ����ָ��ֱ�ָ��R, G, B���ڴ��ַ�����ĸ�ָ�뱣������
		linesizes[4]������ͼ��ÿ��ͨ�����ڴ����Ĳ�������һ�еĶ����ڴ�Ŀ�ȣ���ֵ��С����ͼ���ȡ�
	    w : Ҫ�����ڴ��ͼ���ȡ�
		h : Ҫ�����ڴ��ͼ��߶ȡ�
		pix_fmt : Ҫ�����ڴ��ͼ������ظ�ʽ��
		align : �����ڴ�����ֵ��
		����ֵ����������ڴ�ռ���ܴ�С������Ǹ�ֵ����ʾ����ʧ�ܡ�
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