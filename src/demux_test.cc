#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>
#include <functional>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
}

#include "demux.hpp"
#include "audio.hpp"

TEST(DemuxTest, BasicTest) {
  const std::string input_file_name = "test_data/orders.mp3";
  std::ifstream input_file(input_file_name);
  Demux demux(input_file);

  EXPECT_THAT(demux.StreamsCount(), 1);
}

TEST(DemuxTest, DISABLED_ReadAllFrames) {
  const std::string input_file_name = "test_data/orders.mp3";
  std::ifstream input_file(input_file_name);
  Demux demux(input_file);

  int got_frame_ptr;
  std::clog << "demux.GetDecoder(0)" << std::endl;
  auto codec = demux.GetDecoder(0);
  AudioDecoder<float> audio_codec(codec);

  std::clog << "While (auto packet = demux.read())" << std::endl;
  while (auto packet = demux.read()) {
    // std::clog << "index = " << pkt->stream_index
    //           << "; duration = " << pkt->duration << std::endl;
    if (packet->StreamIndex() != 0) continue;
    std::clog << "codec.Write(*packet)" << std::endl;
    ASSERT_FALSE(codec.Write(*packet));
    while (auto sample = audio_codec.Read()) {
      std::clog << sample->sample(0) << " ";
    }
    std::clog << std::endl;
  }
}
