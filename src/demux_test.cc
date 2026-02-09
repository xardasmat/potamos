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

#include "audio.hpp"
#include "demux.hpp"

namespace potamos {

TEST(DemuxTest, BasicTest) {
  const std::string input_file_name = "test_data/orders.mp3";
  std::ifstream input_file(input_file_name);
  Demux demux(input_file);

  EXPECT_THAT(demux.StreamsCount(), 1);
}

TEST(DemuxTest, ReadBasicMp3File) {
  const std::string input_file_name = "test_data/orders.mp3";
  static const std::string cmd =
      "ffmpeg -i test_data/orders.mp3 -f f32le -y - 2>/dev/null";

  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"),
                                                pclose);

  std::ifstream input_file(input_file_name);
  Demux demux(input_file);

  int got_frame_ptr;
  auto codec = demux.GetDecoder(0);
  AudioDecoder<float> audio_codec(codec);

  ASSERT_FALSE(feof(pipe.get()));
  ASSERT_FALSE(ferror(pipe.get()));

  int index = 0;
  float raw_float = 1337;
  const float start_time_seconds = 0.050113;
  while (auto sample = audio_codec.Read()) {
    ASSERT_EQ(fread(&raw_float, 1, 4, pipe.get()), 4)
        << "end of bytes at " << index;
    ASSERT_EQ(sample->sample(0), raw_float) << "samples mismatch at " << index;

    // For some reason first frame has only 47 samples, but pts are set just
    // like if all frames were full 576 samples
    if (index < 47) {
      ASSERT_THAT(
          double(sample->time()),
          testing::DoubleNear(index / 22050.0 + 0.026122448979591838, 1e-6))
          << "time missmatch at " << index;
    } else {
      ASSERT_THAT(double(sample->time()),
                  testing::DoubleNear(index / 22050.0 + start_time_seconds, 1e-6))
          << "time missmatch at " << index;
    }
    ++index;
  }

  ASSERT_EQ(fread(&raw_float, 1, 4, pipe.get()), 0) << "some bytes at the end ";
  EXPECT_TRUE(feof(pipe.get())) << "not all samples were decoded";
}

}  // namespace potamos