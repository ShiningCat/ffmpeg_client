#include "screen.h"

using namespace std;

screen::screen() {

	// ע���豸
	avdevice_register_all();
	avformat_network_init();
}


screen::~screen() {


	av_packet_free(&packet);

	av_packet_free(&outPacket);

	av_frame_free(&frame);

	av_frame_free(&outFrame);

	sws_freeContext(swsCtx);
	// �ر�ת����
	avcodec_close(codecCtx);
	// �ر�ת����
	avcodec_close(outCodecCtx);


	avformat_close_input(&outFormatContext);
	// �ر���
	avformat_close_input(&formatContext);
		
}


int screen::initFormatContext(string input, string url) {
	int ret = 0;
	// �ṹ��������һ��ý���ļ���ý�����Ĺ��ɺͻ�����Ϣ avformat_open_input�Զ����� AVFormatContext
	// avformat_alloc_context();

	// �����豸 Ϊ gdigrab  mac:avfoundation
	const AVInputFormat *pAVInputFormat = av_find_input_format(input.c_str());
	
	// AVDictionary��һ����ֵ�Դ洢���ߣ�������c++�е�map��ffmpeg���кܶ� API ͨ���������ݲ����� ¼�Ʋ�������  ת���������
	AVDictionary* options = NULL;

	// �������� desktop title={��������}
	if ((ret = avformat_open_input(&formatContext, url.c_str(), pAVInputFormat, &options)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot open input stream.\n");
		return ret;
	}
	// ��ȡ����Ϣ ����AVStream�ṹ��
	if ((ret = avformat_find_stream_info(formatContext, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Couldn't find stream information.\n");
		return ret;
	}

	return ret;
}

int screen::initOutFormatContext() {
	int ret = 0;
	
	avformat_alloc_output_context2(&outFormatContext, NULL, NULL, "test.h264");
	const AVOutputFormat *outformat = outFormatContext->oformat;

	
	outStream = avformat_new_stream(outFormatContext, NULL);

	//av_dump_format(outFormatContext, 0, "test.h264", 1);

	return ret;
}

int screen::buildCodecContext() {
	int ret = 0;
	// 
	videoIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	//cout << videoIndex << endl;
	// ��ȡ����Ϣ AVStream�Ǵ洢ÿһ����Ƶ/��Ƶ����Ϣ�Ľṹ��
	AVStream *stream = formatContext->streams[videoIndex];
	// ���ҽ����� 
	codec = avcodec_find_decoder(stream->codecpar->codec_id);
	if (codec == NULL)
	{
		av_log(NULL, AV_LOG_ERROR, "Codec not found.\n");
		return -1;
	}
	// ����ͨ���л�ȡ����������������� �������ǽ� H264 �� AAC �����洢ΪMP4�ļ���ʱ�򣬾���Ҫ�� MP4�ļ�������������ͨ����һ���洢Video��H264��һ���洢Audio��AAC��������H264��AACֻ����������ͨ������
	AVCodecParameters *avCodeParams = stream->codecpar;
	// ���ý�����
	codecCtx = avcodec_alloc_context3(codec);
	// ��Ƶ���л�ȡ��������������Ƶ� AVCodecContext
	avcodec_parameters_to_context(codecCtx, avCodeParams);
	// bmp
	cout << avcodec_get_name(codecCtx->codec_id) << endl;
	// AV_PIX_FMT_ABGR
	cout << codecCtx->pix_fmt << endl;
	// �򿪽�����
	if ((ret = avcodec_open2(codecCtx, codec, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not open codec.\n");
		return -1;
	}

	// �������豸��ȡ����ԭ������������Ҳ���Ǿ����豸����֮������ݡ� ��Ҫ�Ƚ�ԭ�����ݽ��н��룬��ɳ���ɶ������ݣ��ڱ��������豸��ʶ�������
    // AVPacket ����ǰ AVFrame ��ʾ��������Ƶ֡
    // �ýṹ�洢ѹ������ ������Ƶ����ͨ��Ӧ����һ��ѹ��֡ ������Ƶ�������ܰ�������ѹ��֡
	packet = av_packet_alloc();
	frame = av_frame_alloc();

	return ret;
}

int screen::buildEncodecContext() {
	int ret = 0;
	// ������
	outCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	// ���ý�����
	outCodecCtx = avcodec_alloc_context3(outCodec);
	outCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	// ����
	outCodecCtx->bit_rate = 2000000;
	// ÿ25֡����һ���ؼ�֡ I֡
	outCodecCtx->gop_size = 250;
	outCodecCtx->width = 1920;
	outCodecCtx->height = 1080;
	// ��ת�� AV_PIX_FMT_YUV420P
	outCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	outCodecCtx->codec_id = AV_CODEC_ID_H264;
	outCodecCtx->time_base.num = 1;
	outCodecCtx->time_base.den = 25;

	// fps
	outCodecCtx->framerate.num = 25;
	// ���������id��h264
	if (outCodecCtx->codec_id == AV_CODEC_ID_H264) {
		// preset��ʾ����һ��Ԥ���趨�õĲ�������������slow��slow��ʾѹ���ٶ������ģ����Ŀ��Ա�֤��Ƶ�������ÿ�Ļή����Ƶ����
		av_opt_set(outCodecCtx->priv_data, "preset", "slow", 0);
	}

	// �򿪱������
	if ((ret = avcodec_open2(outCodecCtx, outCodec, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not open codec.\n");
		exit(1);
	}

	outFrame = av_frame_alloc();
	outFrame->format = outCodecCtx->pix_fmt;
	outFrame->width = outCodecCtx->width;
	outFrame->height = outCodecCtx->width;
	//outFrame->f
	outFrame->pts = 0;
	//  av_frame_get_buffer�������Ϊ�Զ�ΪAVFrame����ռ䣬��av_image_fill_arrays�������Ϊ�ֶ���䡣
	//  �����һ��AVFrame��ʼ����Ϊ�˼̳�swsת�������ݣ�����ѡ��av_frame_get_buffer���г�ʼ�����������ͷ�ʱֱ�ӵ���av_frame_free���ɣ����õ����ڴ�й©���⡣
	// Ϊʲô��32? rgb 8:8:8:8 32bp
	ret = av_frame_get_buffer(outFrame, 32);

	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "nerror in filling image array");
		return -1;
	}

	// ������õ����ݱ�����AVPacket
	outPacket = av_packet_alloc();

	if (avio_open(&outFormatContext->pb, "test.h264", AVIO_FLAG_READ_WRITE) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to open output file! \n");
		return -1;
	}
	outFormatContext->streams[0]->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	outFormatContext->streams[0]->codecpar->width = 1920;
	outFormatContext->streams[0]->codecpar->height = 1080;

	ret = avformat_write_header(outFormatContext, NULL);
	if (ret < 0) {
		cout << "\nerror in writing the header context";
		return ret;
	}


	return ret;
}

int screen::buildSwsContext() {
	int ret = 0;
	// ������Ƶ�ز���������
	// ����1����ת��Դ�Ŀ� ����2����ת��Դ�ĸ� 
	// ����3����ת��Դ�ĸ�ʽ��eg��YUV��RGB������ö�ٸ�ʽ��Ҳ����ֱ����ö�ٵĴ��ű�ʾeg��AV_PIX_FMT_YUV420P��Щö�ٵĸ�ʽ��libavutil / pixfmt.h���г���
	// ����4��ת����ָ���Ŀ� ����5��ת����ָ���ĸ� 
	// ����6��ת����ָ���ĸ�ʽͬ����3�ĸ�ʽ ����7��ת����ʹ�õ��㷨��
	swsCtx = sws_getContext(codecCtx->width,
		codecCtx->height,
		codecCtx->pix_fmt,
		outCodecCtx->width,
		outCodecCtx->height,
		outCodecCtx->pix_fmt,
		SWS_BICUBIC, NULL, NULL, NULL);
	return ret;
}


int screen::record() {
	
	//ofstream outfile("test.h264", ios::binary);

	int ret = 0;
	int count = 0;
	int j = 0;
	int pts = 0;
	while (av_read_frame(formatContext, packet) >= 0) {
		//AVPacket orig_pkt = packet;
		if (count++ > 498) {
			break;
		}
		if (packet->stream_index != videoIndex) {
			av_log(NULL, AV_LOG_ERROR, "not videoIndex\n");
			continue;
		}

		// ʹ��avcodec_send_packet�������豸�����ݷ������������н���
		ret = avcodec_send_packet(codecCtx, packet);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Error sending a packet for decoding\n");
			return -1;
		}
		
		while (ret >= 0) {

			//��ȡ��������
			ret = avcodec_receive_frame(codecCtx, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				break;
			}
			else if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "Error during decoding\n");
				exit(1);
			}
			// pCodecCtx->pix_fmt AV_PIX_FMT_ABGR
			// ת��
			int ret = sws_scale(swsCtx, frame->data, frame->linesize, 0, codecCtx->height, outFrame->data, outFrame->linesize);
			// ���� send_frame ֮ǰ���� outFrame �� pts��
			outFrame->pts = pts ++;
			//outFrame->pts = count++ * (outCodecCtx->time_base.num * 1000 / outCodecCtx->time_base.den);
			// ת����ɺ������ �ٴα���
			//cout << outFrame->pts << endl;
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

				//д��H264�ļ�
				// outfile.write((char*)outPacket->data, outPacket->size);
				printf("Write frame %3d (size= %2d)\n", j++, outPacket->size / 1000);
				ret = av_interleaved_write_frame(outFormatContext, outPacket);

				// �ͷ�outPacket
				av_packet_unref(outPacket);

			}

		}
		// �ͷ�Packet
		av_packet_unref(packet);

	}
	//outfile.close();
	//Write file trailer
	av_write_trailer(outFormatContext);
	return ret;
}