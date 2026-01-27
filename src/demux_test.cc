#include <iostream>
#include <fstream>
#include <functional>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
}

#include "demux.hpp"

TEST(DemuxTest, BasicTest) {
  const std::string input_file_name = "test_data/orders.mp3";
  std::ifstream input_file(input_file_name);
  Demux demux(input_file);

  EXPECT_THAT(demux.StreamsCount(), 1);
}

TEST(DemuxTest, ReadAllFrames) {
  const std::string input_file_name = "test_data/orders.mp3";
  std::ifstream input_file(input_file_name);
  Demux demux(input_file);
  
  int got_frame_ptr;
  auto codec = demux.Codec(0);
  std::clog << "codec name = " << codec->codec_descriptor->name << "; codec sample_fmt = " << codec->sample_fmt << std::endl;
  while (auto pkt_opt = demux.read()) {
    auto *pkt = *pkt_opt;
    std::clog << "index = " << pkt->stream_index << "; duration = " << pkt->duration << std::endl;
    if (pkt->stream_index != 0) continue;
    int ret1 = avcodec_send_packet(codec, pkt);
    ASSERT_THAT(ret1, testing::Ge(0)) << " error code = " << ret1;
    int ret2 = 0;
    do {
      AVFrame* frame = av_frame_alloc();
      ret2 = avcodec_receive_frame(codec, frame);
      if (ret2 >= 0) {
        std::clog << "$$$ samples=" << frame->nb_samples << " // channels=" << frame->ch_layout.nb_channels << std::endl;
        if (codec->sample_fmt == AV_SAMPLE_FMT_FLTP) {
          float *buffer = (float*)frame->data[0];
          for (int i = 0; i< frame->nb_samples; ++i) {
            std::clog << " " << buffer[i];
          }
        }
        std::clog << std::endl;
      }
      av_frame_unref(frame);
    } while (ret2 >= 0);
  }
}
