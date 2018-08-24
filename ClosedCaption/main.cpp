// mpegvideoCC.cpp : 定义控制台应用程序的入口点。
//

#include <stdlib.h>
#include <iostream>
#include <iostream>
#include <ostream>
#include <fstream>

using namespace std;

extern "C"
{
// ffmpeg version: 4.0.1
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h" 
#include "libavutil/timestamp.h"
#include "libavutil/log.h"
#include "mp2cc.h"
#include "wv_expGolobm.h"
}

int getCC(char * inputFileName,  char * outputFileName)
{
	AVFormatContext *ifmt_ctx = NULL;
	AVPacket pkt;
	int ret;
	int stream_index = 0;
	wv_encodedFrame inputFrame;

	ofstream out;
	out.open(outputFileName, ios::binary);
	if (!out.is_open())
	{
		printf("%s can't open\n", outputFileName);
		goto end;
	}

	if ((ret = avformat_open_input(&ifmt_ctx, inputFileName, 0, 0)) < 0) {
		fprintf(stderr, "Could not open input file '%s'", inputFileName);
		goto end;
	}

	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
		fprintf(stderr, "Failed to retrieve input stream information");
		goto end;
	}

	av_dump_format(ifmt_ctx, 0, inputFileName, 0);

	wvcc_InitParserCtx(&inputFrame);

	while (1) {
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0)
			break;

		/* get cc data */
		if (AVMEDIA_TYPE_VIDEO == ifmt_ctx->streams[pkt.stream_index]->codecpar->codec_type)
		{			
			inputFrame.data = pkt.data;
			inputFrame.size = pkt.size;

			switch(ifmt_ctx->streams[pkt.stream_index]->codecpar->codec_id)
			{
				case AV_CODEC_ID_MPEG2VIDEO:
					inputFrame.encodeType = WV_VIDEO_TYPE_MPEG2;
					break;
				case AV_CODEC_ID_H264:
					inputFrame.encodeType = WV_VIDEO_TYPE_H264;
					break;
				default:
					printf("Unkown video format!\n");
					break;
			}

			uint8_t CCBuffer[128];
			uint32_t CCLen = 0;
			uint32_t PicOrder = 0;
			WV_PICTURE_TYPE type;

			wvcc_getPictureType(&inputFrame, &type);
			wvcc_getPictureOrder(&inputFrame, &PicOrder);

			if (wvcc_findCCContent(&inputFrame, CCBuffer, sizeof(CCBuffer), &CCLen))
			{
				//printf("find cc chunk %d\n", CCLen);
				out.write((char *)CCBuffer, CCLen);
			}
			else
			{
				//printf("can't find cc chunk!\n");
			}
		}

		av_packet_unref(&pkt);
	}
end:

	wvcc_DelParserCtx(&inputFrame);
	avformat_close_input(&ifmt_ctx);
	out.close();
	//system("pause");
	return 0;
}

int insertCC(char * in_filename, char * out_filename, char * cc_filename)
{
	AVOutputFormat *ofmt = NULL;
	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;
	int ret, i;
	int stream_index = 0;
	int *stream_mapping = NULL;
	int stream_mapping_size = 0;
	ifstream in;

	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
		fprintf(stderr, "Could not open input file '%s'", in_filename);
		goto end;
	}

	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
		fprintf(stderr, "Failed to retrieve input stream information");
		goto end;
	}

	in.open(cc_filename, ios::binary);
	if (!in.is_open())
	{
		printf("%s can't open\n", cc_filename);
		goto end;
	}

	av_dump_format(ifmt_ctx, 0, in_filename, 0);

	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
	if (!ofmt_ctx) {
		fprintf(stderr, "Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}

	stream_mapping_size = ifmt_ctx->nb_streams;
	stream_mapping = (int *)av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
	if (!stream_mapping) {
		ret = AVERROR(ENOMEM);
		goto end;
	}

	ofmt = ofmt_ctx->oformat;

	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		AVStream *out_stream;
		AVStream *in_stream = ifmt_ctx->streams[i];
		AVCodecParameters *in_codecpar = in_stream->codecpar;

		if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
			in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
			in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
			stream_mapping[i] = -1;
			continue;
		}

		stream_mapping[i] = stream_index++;

		out_stream = avformat_new_stream(ofmt_ctx, NULL);
		if (!out_stream) {
			fprintf(stderr, "Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}

		ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
		if (ret < 0) {
			fprintf(stderr, "Failed to copy codec parameters\n");
			goto end;
		}
		out_stream->codecpar->codec_tag = 0;
	}
	av_dump_format(ofmt_ctx, 0, out_filename, 1);

	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			fprintf(stderr, "Could not open output file '%s'", out_filename);
			goto end;
		}
	}

	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		fprintf(stderr, "Error occurred when opening output file\n");
		goto end;
	}

	while (1) {
		AVStream *in_stream, *out_stream;

		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0)
			break;

		in_stream = ifmt_ctx->streams[pkt.stream_index];
		if (pkt.stream_index >= stream_mapping_size ||
			stream_mapping[pkt.stream_index] < 0) {
			av_packet_unref(&pkt);
			continue;
		}

#if 1
		/* get cc data */
		if (AVMEDIA_TYPE_VIDEO == ifmt_ctx->streams[pkt.stream_index]->codecpar->codec_type)
		{
			wv_encodedFrame inputFrame;
			inputFrame.data = pkt.data;
			inputFrame.size = pkt.size;
			inputFrame.encodeType = WV_VIDEO_TYPE_MPEG2;

			uint8_t CCBuffer[128];
			uint32_t CCLen = 0;

			in.read((char *)CCBuffer, 6);

			if (wvcc_insertCCContent(&inputFrame, CCBuffer, 6))
			{
				printf("insert cc chunk %d\n", CCLen);
			}
			else
			{
				//printf("can't find cc chunk!\n");
			}

			av_grow_packet(&pkt, inputFrame.size - pkt.size);
			memcpy(pkt.data, inputFrame.data, inputFrame.size);
			free(inputFrame.data);
		}
#endif

		pkt.stream_index = stream_mapping[pkt.stream_index];
		out_stream = ofmt_ctx->streams[pkt.stream_index];
		//log_packet(ifmt_ctx, &pkt, "in");

		/* copy packet */
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		//log_packet(ofmt_ctx, &pkt, "out");

		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
		if (ret < 0) {
			fprintf(stderr, "Error muxing packet\n");
			break;
		}
		av_packet_unref(&pkt);
	}

	av_write_trailer(ofmt_ctx);
end:

	avformat_close_input(&ifmt_ctx);

	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_closep(&ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);

	av_freep(&stream_mapping);

	if (ret < 0 && ret != AVERROR_EOF) {
		//fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
		return 1;
	}

	//system("pause");

	return 0;
}


int main(int argc, char **argv)
{
	/*  从ts中读取CC并保存到一个二进制文件中 */
	//getCC("mpeg_720x480.ts", "cc.data"); 
	getCC("h264_720x480x5994i.ts", "cc.data");
	//expGolobmTest();

	/* 从二进制文件中读取CC，并插入ts流中 */
	//insertCC("mpeg_720x480x5994i_nocc.ts", "mepg_insertcc.ts", "cc.data");

	/*  从ts中读取CC并保存到一个二进制文件中 */
	//getCC("h264_720x480x5994i.ts", "cc.data");

	system("pause");

	return 0;
}


