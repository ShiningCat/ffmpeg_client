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
}

using namespace std;

class screen
{
public:
	screen();
	~screen();

	// ��ʼ������context;
	int initFormatContext(string input, string url);

	// ��ʼ�����context;
	int initOutFormatContext();

	// ���������
	int buildCodecContext();

	// ���������
	int buildEncodecContext();

	// ����ͼ��ת��context
	int buildSwsContext();

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

	SwsContext *swsCtx;

	AVStream *outStream;

	int videoIndex;

};