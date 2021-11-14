#pragma once

#include <iostream>
#include <string>
#include <stdio.h>
#include <fstream>


extern "C"
{
#include <libavformat\avformat.h>
#include <libavdevice\avdevice.h>
#include <libavcodec\avcodec.h>
#include <libswscale\swscale.h>
#include <libavutil\imgutils.h>
#include <libavutil\avutil.h>
#include <libswresample\swresample.h>
#include <libavutil\channel_layout.h>
#include <libavutil\audio_fifo.h>

}

using namespace std;

class audio
{
public:
	audio();
	~audio();

	// ��ʼ������context;
	int initFormatContext(string input);

	// ��ʼ�����context;
	int initOutFormatContext();

	void initAudioBuffer();

	// ���������
	int buildCodecContext();

	// ���������
	int buildEncodecContext();

	int buildSwrContext();

	// ¼��
	int record();

private:

	AVFormatContext	*formatContext = NULL;

	AVFormatContext *outFormatContext = NULL;

	AVCodecContext *codecCtx;

	AVCodecContext *outCodecCtx;

	const AVCodec *codec;

	const AVCodec *outCodec;

	AVPacket *packet;

	AVFrame *frame;

	AVPacket *outPacket;

	AVFrame *outFrame;

	AVDictionary *options;

	SwrContext *swrCtx;

	int audioIndex;

	int audioOutIndex;

	AVStream *outStream;

	AVAudioFifo *fifo;

	int samples;

	AVFrame *tempOutFrameDate;

	uint8_t **dataOutData = NULL;

	int initAudioOutFrame();

	int fifoWrite();

	int fifoRead();
};