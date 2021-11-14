#include "audio.h"
#include <Windows.h>

using namespace std;

audio::audio() {

	// 注册设备
	avdevice_register_all();
	avformat_network_init();
}


audio::~audio() {

	av_packet_free(&packet);

	av_packet_free(&outPacket);

	av_frame_free(&frame);


	av_audio_fifo_free(fifo);

	swr_close(swrCtx);
	// 关闭转码器
	avcodec_close(codecCtx);
	// 关闭转码器
	avcodec_close(outCodecCtx);

	// 关闭流
	avformat_close_input(&formatContext);
	// 关闭流
	avformat_close_input(&outFormatContext);

}

static char* WStringToString(const std::wstring wstr)
{

	int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
	char*buffer = new char[len + 1];

	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
	buffer[len] = '\0';

	return buffer;
}

int audio::initFormatContext(string input) {
	int ret = 0;

	const AVInputFormat *pAVInputFormat = av_find_input_format("dshow");

	AVDictionary* options = NULL;

	
	char * str = WStringToString(L"audio=立体声混音 (Realtek(R) Audio)");
	//av_dict_set(&options, "list_devices", "true", 0);

	if ((ret = avformat_open_input(&formatContext, str, pAVInputFormat, &options)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot open input stream.\n");
		return ret;
	}
	if ((ret = avformat_find_stream_info(formatContext, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Couldn't find stream information.\n");
		return ret;
	}

	return ret;
}

int audio::initOutFormatContext() {
	avformat_alloc_output_context2(&outFormatContext, nullptr, nullptr, "test.aac");

	outStream = avformat_new_stream(outFormatContext, NULL);

	audioOutIndex = outStream->index;

	
	return 0;
}


int audio::buildCodecContext() {
	int ret = 0;

	audioIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	cout << audioIndex << endl;
	// 获取流信息 AVStream是存储每一个视频/音频流信息的结构体
	AVStream *stream = formatContext->streams[audioIndex];
	// 查找解码器 
	codec = avcodec_find_decoder(stream->codecpar->codec_id);
	if (codec == NULL)
	{
		av_log(NULL, AV_LOG_ERROR, "Codec not found.\n");
		return -1;
	}
	
	AVCodecParameters *avCodeParams = stream->codecpar;

	codecCtx = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(codecCtx, avCodeParams);
	cout << avcodec_get_name(codecCtx->codec_id) << endl;
	cout << av_get_sample_fmt_name(codecCtx->sample_fmt) << endl;

	if ((ret = avcodec_open2(codecCtx, codec, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not open codec.\n");
		return -1;
	}

	packet = av_packet_alloc();
	frame = av_frame_alloc();

	return ret;
}

int audio::buildEncodecContext() {

	int ret = 0;
	// 编码器
	//outCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	outCodec = avcodec_find_encoder(outFormatContext->oformat->audio_codec);
	
	outCodecCtx = avcodec_alloc_context3(outCodec);

	outCodecCtx->codec_id = AV_CODEC_ID_AAC;
	// 码率
	outCodecCtx->bit_rate = 96000;
	//outCodecCtx->sample_rate = 44100;
	outCodecCtx->sample_rate = codecCtx->sample_rate;

	//outCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
	outCodecCtx->sample_fmt = outCodec->sample_fmts[0];

	outCodecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	outStream->time_base.num = 1;
	outStream->time_base.den = codecCtx->sample_rate;

	if (outFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
	{
		outCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}
	
	ret = avcodec_parameters_from_context(outStream->codecpar, outCodecCtx);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Could not initialize stream parameters\n");
		return ret;
	}

	outCodecCtx->channels = 2;
	outCodecCtx->channel_layout = av_get_default_channel_layout(2); 

	// 打开编解码器
	if ((ret = avcodec_open2(outCodecCtx, outCodec, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not open codec.\n");
		exit(1);
	}

	//将codecCtx中的参数传给音频输出流
	ret = avcodec_parameters_from_context(outStream->codecpar, outCodecCtx);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Output audio avcodec_parameters_from_context,error code:%d",ret);
		return -1;
	}

	//打开输出文件
	if (!(outFormatContext->oformat->flags & AVFMT_NOFILE))
	{
		if (avio_open(&outFormatContext->pb, "test.aac", AVIO_FLAG_WRITE) < 0)
		{
			printf("can not open output file handle!\n");
			return -1;
		}
	}
	//写文件头
	if (avformat_write_header(outFormatContext, nullptr) < 0)
	{
		printf("can not write the header of the output file!\n");
		return -1;
	}

	outPacket = av_packet_alloc();
	return ret;
}

void audio::initAudioBuffer()
{
	samples = outCodecCtx->frame_size;
	if (!samples)
	{

		av_log(NULL, AV_LOG_ERROR, "samples==0");
		samples = 1024;
	}
	fifo = av_audio_fifo_alloc(outCodecCtx->sample_fmt, outCodecCtx->channels, 1);
	if (!fifo)
	{
		av_log(NULL, AV_LOG_ERROR, "av_audio_fifo_alloc failed");
		return;
	}
}

int audio::buildSwrContext() {
	int ret = 0;

	swrCtx = swr_alloc();

	swrCtx = swr_alloc_set_opts(NULL,
		av_get_default_channel_layout(outCodecCtx->channels ),
		outCodecCtx->sample_fmt,
		outCodecCtx->sample_rate,
		av_get_default_channel_layout(codecCtx->channels),
		codecCtx->sample_fmt,
		codecCtx->sample_rate,
		0,
		NULL);
	 
	if ((ret = swr_init(swrCtx)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "swr_init failed");
		return ret;
	}
	return ret;
}


int audio::record() {


	int ret = 0;
	int pts = 0;
	int j = 0;
	for (int i = 0; i < 250; i++) {
		

		int finished = 0;
		
		tempOutFrameDate = av_frame_alloc();
		while (av_audio_fifo_size(fifo) < outCodecCtx->frame_size) {
			fifoWrite();
		}

		//while (av_audio_fifo_size(fifo) >= samples || (finished && av_audio_fifo_size(fifo) > 0)){

		while (av_audio_fifo_size(fifo) >= samples){
			
			const int frameSize = FFMIN(av_audio_fifo_size(fifo),
				outCodecCtx->frame_size);
			initAudioOutFrame();

			if (outFrame) {
				outFrame->pts = pts;

				pts += outFrame->nb_samples;
			}
			av_audio_fifo_read(fifo, (void **)outFrame->data, frameSize);

			fifoRead();

			av_frame_free(&outFrame);
			
		}
		av_frame_free(&tempOutFrameDate);
		


		
	}

	av_write_trailer(outFormatContext);

	return ret;
}


int audio::fifoWrite() {

	int ret;
	ret = av_read_frame(formatContext, packet);
	if (ret < 0) {
		return -1;
	}

	ret = avcodec_send_packet(codecCtx, packet);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Error sending a packet for decoding\n");
		return -1;
	}

	if (packet->stream_index != audioIndex) {
		av_log(NULL, AV_LOG_ERROR, "not audioIndex\n");
		
	}

	int finished = 0;

	//读取解码数据
	ret = avcodec_receive_frame(codecCtx, frame);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		if (ret == AVERROR_EOF) {
			finished = 1;
		}
		
	}
	else if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Error during decoding\n");
		exit(1);
	}
	 
	ret = av_samples_alloc(tempOutFrameDate->data, NULL, outCodecCtx->channels,
		frame->nb_samples, outCodecCtx->sample_fmt, 0);

	swr_convert(swrCtx, tempOutFrameDate->data, frame->nb_samples, (const uint8_t **)frame->extended_data, frame->nb_samples);

	if ((ret = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frame->nb_samples)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Could not reallocate FIFO\n");
		return ret;
	}

	if (av_audio_fifo_write(fifo, (void **)tempOutFrameDate->data, frame->nb_samples) < frame->nb_samples)
	{
		av_log(NULL, AV_LOG_ERROR, "av_audio_fifo_write\n");
		return ret;
	}
	return ret;

}

int audio::initAudioOutFrame() {
	int ret;
	outFrame = av_frame_alloc();
	outFrame->nb_samples = outCodecCtx->frame_size;
	outFrame->format = outCodecCtx->sample_fmt;
	outFrame->channel_layout = outCodecCtx->channel_layout;
	

	ret = av_frame_get_buffer(outFrame, 0);


	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "nerror in filling image array");
		return -1;
	}
	return ret;
}

int audio::fifoRead() {

	int ret;
	

	ret = avcodec_send_frame(outCodecCtx, outFrame);

	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Error sending frame to encoding\n");
		return -1;
	}

	ret = av_frame_make_writable(outFrame);
	if (ret < 0) {
		return -1;
	}

	while (ret >= 0) {

		ret = avcodec_receive_packet(outCodecCtx, outPacket);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			break;
		}
		else if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Error during encoding\n");
			exit(1);
		}

		//printf("Write frame %3d (size= %d)\n", j++, outPacket->size);

		av_write_frame(outFormatContext, outPacket);

		// 释放outPacket
		av_packet_unref(outPacket);
	}
}