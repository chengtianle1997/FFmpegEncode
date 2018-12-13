
#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#include "iostream"

#include "MvCameraControl.h"

#ifdef _WIN64



//Windows

extern "C"

{

#include "libavcodec/avcodec.h"

#include "libavformat/avformat.h"

#include "libavformat/avio.h"

};

#else

//Linux...

#ifdef __cplusplus

extern "C"

{

#endif

#include <libavcodec/avcodec.h>

#include <libavformat/avformat.h>

#ifdef __cplusplus

};

#endif

#endif

void* handle = NULL;//������

//��ӡ����豸��Ϣ
bool  PrintDeviceInfo(MV_CC_DEVICE_INFO* pstMVDevInfo)
{
	if (pstMVDevInfo == NULL)
	{
		printf("The Pointer of pstMVDevInfo is NULL!\n");
		return false;
	}
	if (pstMVDevInfo->nTLayerType == MV_USB_DEVICE)
	{
		printf("UserDefinedName: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);
		printf("Serial Number: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chSerialNumber);
		printf("Device Number: %d\n\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.nDeviceNumber);
	}
	else
	{
		printf("Not Support!\n");
	}
	return true;
}


int main(int argc, char* argv[])

{
	int nRet = MV_OK;
	unsigned int g_nPayloadSize = 0;
	//void* handle = NULL;
	AVFormatContext* pFormatCtx;

	AVOutputFormat* fmt;

	AVStream* video_st;

	AVCodecContext* pCodecCtx;

	AVCodec* pCodec;

	uint8_t* picture_buf;

	AVFrame* picture;

	AVPacket pkt;

	int y_size;

	int got_picture = 0;

	int size;

	int framecount = 100;

	int ret = 0;

	int i = 0;

	//FILE *in_file = NULL;                            //YUV source

	int in_w, in_h ;                           //YUV's width and height

	const char* out_file = "encode.mjpeg";    //Output file

	//unsigned char * pData = NULL;

	unsigned char *pDataForYUV = NULL;

	AVCodecID codec_id = AV_CODEC_ID_MJPEG;

	//in_file = fopen("ds_480x272.yuv", "rb");

	//��ȡ�豸ö���б�
	MV_CC_DEVICE_INFO_LIST stDevList;
	memset(&stDevList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
	nRet = MV_CC_EnumDevices(MV_USB_DEVICE, &stDevList);

	if (MV_OK != nRet)
	{
		printf("Enum Devices fail! nRet [0x%x]\n", nRet);
	}

	if (stDevList.nDeviceNum > 0)
	{
		for (unsigned int i = 0; i < stDevList.nDeviceNum; i++)
		{
			printf("[device %d]:\n", i);
			//�豸��Ϣ
			MV_CC_DEVICE_INFO* pDeviceInfo = stDevList.pDeviceInfo[i];
			if (NULL == pDeviceInfo)
			{
				break;
			}
			PrintDeviceInfo(pDeviceInfo);
		}
	}
	else
	{
		printf("Find No Devices!\n");
	}


	//����������
	printf("Please Intput camera index:");
	unsigned int devNum = 0;
	scanf("%d", &devNum);
	//ȷ��������ȷ
	if (devNum >= stDevList.nDeviceNum)
	{
		printf("Intput error!\n");
	}
	printf("Please Intput the frame count:");
	scanf("%d", &framecount);

	//ѡ���豸���������
	nRet = MV_CC_CreateHandle(&handle, stDevList.pDeviceInfo[devNum]);
	if (MV_OK != nRet)
	{
		printf("Create Handle fail! nRet [0x%x]\n", nRet);
	}
	nRet = MV_CC_OpenDevice(handle);
	if (MV_OK != nRet)
	{
		printf("Open Device fail! nRet [0x%x]\n", nRet);
	}
	else {
		printf("Device is ready!\n");
	}

	//���ô���ģʽΪoff
	nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
	if (MV_OK != nRet)
	{
		printf("Set Trigger Mode fail! nRet [0x%x]\n", nRet);
	}

	//��ȡ���ݰ���С
	MVCC_INTVALUE stParam;
	memset(&stParam, 0, sizeof(MVCC_INTVALUE));
	nRet = MV_CC_GetIntValue(handle, "PayloadSize", &stParam);
	if (MV_OK != nRet)
	{
		printf("Get PayloadSize fail! nRet [0x%x]\n", nRet);
	}
	g_nPayloadSize = stParam.nCurValue;
	//Get the width and Height of the Camera
	MVCC_INTVALUE pIntValue;
	memset(&pIntValue, 0, sizeof(MVCC_INTVALUE_EX));
	MV_CC_GetIntValue(handle, "Width", &pIntValue);
	in_w = pIntValue.nCurValue;
	//memset(&pIntValue, 0, sizeof(MVCC_INTVALUE_EX));
	MV_CC_GetIntValue(handle, "Height", &pIntValue);
	in_h = pIntValue.nCurValue;
	//printf("%d*%d", in_w, in_h);

	//��ʼȡ��
	nRet = MV_CC_StartGrabbing(handle);
	if (MV_OK != nRet)
	{
		printf("Start Grabbing fail! nRet [0x%x]\n", nRet);
	}

	MV_FRAME_OUT stOutFrame = { 0 };
	memset(&stOutFrame, 0, sizeof(MV_FRAME_OUT));
	
	//Encode with FFmpeg
	av_register_all();
	//Method 1
	pFormatCtx = avformat_alloc_context();
	//Guess format
	fmt = av_guess_format("mjpeg", NULL, NULL);
	pFormatCtx->oformat = fmt;

	//Output URL
	if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0) {
		printf("Couldn't open output file.");
		return -1;
	}
	
	//Method 2. More simple

	//avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_file);

	//fmt = pFormatCtx->oformat;


	video_st = avformat_new_stream(pFormatCtx, 0);
	if (video_st == NULL) {
		return -1;
	}

	pCodecCtx = video_st->codec;

	pCodecCtx->codec_id = fmt->video_codec;

	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;

	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ422P;



	pCodecCtx->width = in_w;

	pCodecCtx->height = in_h;



	pCodecCtx->time_base.num = 1;

	pCodecCtx->time_base.den = 25;

	pCodecCtx->framerate.num = 25;

	pCodecCtx->framerate.den = 1;

	//Output some information

	av_dump_format(pFormatCtx, 0, out_file, 1);



	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);

	if (!pCodec) {

		printf("Codec not found.");

		return -1;

	}

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {

		printf("Could not open codec.");

		return -1;

	}

	avcodec_parameters_from_context(video_st->codecpar, pCodecCtx);


	picture = av_frame_alloc();

	size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

	picture->width = pCodecCtx->width;

	picture->height = pCodecCtx->height;

	picture->format = pCodecCtx->pix_fmt;

	picture_buf = (uint8_t *)av_malloc(size);

	if (!picture_buf)

	{

		return -1;

	}

	avpicture_fill((AVPicture *)picture, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

	//Write Header

	avformat_write_header(pFormatCtx, NULL);

	y_size = pCodecCtx->width * pCodecCtx->height;

	//int framecount = 0;

	while (1) {
		nRet = MV_CC_GetImageBuffer(handle, &stOutFrame, 1000);
		
		if (nRet == MV_OK)
		{
			pDataForYUV = (unsigned char*)malloc(stOutFrame.stFrameInfo.nWidth * stOutFrame.stFrameInfo.nHeight * 2);
			if (NULL == pDataForYUV)
			{
				printf("malloc pDataForRGB fail !\n");
				break;
			}
			unsigned int nDataSizeForRGB = stOutFrame.stFrameInfo.nWidth * stOutFrame.stFrameInfo.nHeight * 2;

			// ch:���ظ�ʽת�� | en:Convert pixel format 
			MV_CC_PIXEL_CONVERT_PARAM stConvertParam = { 0 };
			memset(&stConvertParam, 0, sizeof(MV_CC_PIXEL_CONVERT_PARAM));

			stConvertParam.nWidth = stOutFrame.stFrameInfo.nWidth;                 //ch:ͼ��� | en:image width
			stConvertParam.nHeight = stOutFrame.stFrameInfo.nHeight;               //ch:ͼ��� | en:image height
			stConvertParam.pSrcData = stOutFrame.pBufAddr;                            //ch:�������ݻ��� | en:input data buffer
			stConvertParam.nSrcDataLen = stOutFrame.stFrameInfo.nFrameLen;         //ch:�������ݴ�С | en:input data size
			stConvertParam.enSrcPixelType = stOutFrame.stFrameInfo.enPixelType;    //ch:�������ظ�ʽ | en:input pixel format
			stConvertParam.enDstPixelType = PixelType_Gvsp_YUV422_Packed; //ch:������ظ�ʽ | en:output pixel format
			stConvertParam.pDstBuffer = pDataForYUV;                    //ch:������ݻ��� | en:output data buffer
			stConvertParam.nDstBufferSize = nDataSizeForRGB;            //ch:��������С | en:output buffer size
			nRet = MV_CC_ConvertPixelType(handle, &stConvertParam);
			
			if (MV_OK != nRet)
			{
				printf("Convert Pixel Type fail! nRet [0x%x]\n", nRet);
				break;
			}

			av_new_packet(&pkt, y_size * 3);

			//printf("%d",sizeof(stOutFrame.pBufAddr));

			//Read YUV

			//memcpy(picture_buf, stOutFrame.pBufAddr, stOutFrame.stFrameInfo.nHeight*stOutFrame.stFrameInfo.nWidth);

			//picture->data[0] = picture_buf;              // Y

			//picture->data[1] = picture_buf + y_size;      // U 

			//picture->data[2] = picture_buf + y_size * 5 / 4;  // V

			uint8_t *pData;
			pData = (uint8_t*)malloc(stOutFrame.stFrameInfo.nWidth*stOutFrame.stFrameInfo.nHeight * 2);

			for (int j = 0; j < stOutFrame.stFrameInfo.nWidth*stOutFrame.stFrameInfo.nHeight; j++)
			{
				
				pData[j] = (uint8_t)atoi((char*)&pDataForYUV[j]);

			}
			

			picture->data[0] = pData;

			picture->data[1] = pData + y_size;

			picture->data[2] = pData + y_size*1/2;

			picture->pts = i;

			//Encode

			ret = avcodec_encode_video2(pCodecCtx, &pkt, picture, &got_picture);

			if (ret < 0) {

				printf("Encode Error.\n");

				return -1;

			}

			if (got_picture == 1) {

				pkt.stream_index = video_st->index;

				ret = av_write_frame(pFormatCtx, &pkt);

			}

			i++;

			av_free_packet(&pkt);

			free(pDataForYUV);
		}
		else
		{
			printf("No data[0x%x]\n", nRet);
		}
		if (NULL != stOutFrame.pBufAddr)
		{
			nRet = MV_CC_FreeImageBuffer(handle, &stOutFrame);
			if (nRet != MV_OK)
			{
				printf("Free Image Buffer fail! nRet [0x%x]\n", nRet);
			}
		}
		if (i > framecount)
			break;
	}

	av_write_trailer(pFormatCtx);



	printf("Encode Successful.\n");



	if (video_st) {

		avcodec_close(video_st->codec);

		av_free(picture);

		av_free(picture_buf);

	}

	avio_close(pFormatCtx->pb);

	avformat_free_context(pFormatCtx);

	return 0;

}
