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
#include "ipstream.hpp"

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
      "ffmpeg -i test_data/orders.mp3 -v quiet -f f32le -y -";

  iPipeStream pipe(cmd);
  ASSERT_TRUE(pipe.good())
      << " failed to initialize the pipeline for the reference stream";

  std::ifstream input_file(input_file_name);
  Demux demux(input_file);

  int got_frame_ptr;
  auto codec = demux.GetDecoder(0);
  AudioDecoder<float> audio_codec(codec);

  int index = 0;
  float raw_float = 1337;
  const float start_time_seconds = 0.050113;
  while (auto sample = audio_codec.Read()) {
    pipe.read((char*)&raw_float, 4);
    ASSERT_EQ(pipe.gcount(), 4) << "end of bytes at " << index;
    ASSERT_EQ(sample->sample(0), raw_float) << "samples mismatch at " << index;

    ASSERT_THAT(double(sample->time()),
                testing::DoubleNear(index / 22050.0 + start_time_seconds, 1e-6))
        << "time missmatch at " << index;
    ++index;
  }

  pipe.peek();
  ASSERT_TRUE(pipe.eof()) << " there are still samples in the reference stream";
}

TEST(DemuxTest, ReadFlacFile) {
  const std::string input_file_name = "test_data/orders.flac";
  static const std::string convert =
      "ffmpeg -v quiet -i test_data/orders.mp3 -y test_data/orders.flac";
  std::system(convert.c_str());
  static const std::string cmd =
      "ffmpeg -i test_data/orders.flac -v quiet -f s32le -y -";

  iPipeStream pipe(cmd);
  ASSERT_TRUE(pipe.good())
      << " failed to initialize the pipeline for the reference stream";

  std::ifstream input_file(input_file_name);
  Demux demux(input_file);

  int got_frame_ptr;
  auto codec = demux.GetDecoder(0);
  AudioDecoder<int> audio_codec(codec);

  int index = 0;
  int32_t raw_sample = 1337;
  const float start_time_seconds = 0.0;
  while (auto sample = audio_codec.Read()) {
    pipe.read((char*)&raw_sample, 4);
    ASSERT_EQ(pipe.gcount(), 4) << "end of bytes at " << index;
    ASSERT_EQ(sample->sample(0), raw_sample)
        << "samples mismatch at " << index << " : " << std::hex
        << sample->sample(0) << " vs " << raw_sample;

    ASSERT_THAT(double(sample->time()),
                testing::DoubleNear(index / 22050.0 + start_time_seconds, 1e-6))
        << "time missmatch at " << index;
    ++index;
  }

  pipe.peek();
  ASSERT_TRUE(pipe.eof()) << " there are still samples in the reference stream";
}

TEST(DemuxTest, ReadAacFile) {
  const std::string input_file_name = "test_data/orders.aac";
  static const std::string convert =
      "ffmpeg -v quiet -i test_data/orders.mp3 -y test_data/orders.aac";
  std::system(convert.c_str());
  static const std::string cmd =
      "ffmpeg -i test_data/orders.aac -v quiet -f f32le -y -";

  iPipeStream pipe(cmd);
  ASSERT_TRUE(pipe.good())
      << " failed to initialize the pipeline for the reference stream";

  std::ifstream input_file(input_file_name);
  Demux demux(input_file);

  int got_frame_ptr;
  auto codec = demux.GetDecoder(0);
  AudioDecoder<float> audio_codec(codec);

  int index = 0;
  float raw_sample = 1337;
  const float start_time_seconds = 0.0;
  while (auto sample = audio_codec.Read()) {
    pipe.read((char*)&raw_sample, 4);
    ASSERT_EQ(pipe.gcount(), 4) << "end of bytes at " << index;
    ASSERT_EQ(sample->sample(0), raw_sample)
        << "samples mismatch at " << index << " : " << std::hex
        << sample->sample(0) << " vs " << raw_sample;

    ASSERT_THAT(double(sample->time()),
                testing::DoubleNear(index / 22050.0 + start_time_seconds, 1e-6))
        << "time missmatch at " << index;
    ++index;
  }

  pipe.peek();
  ASSERT_TRUE(pipe.eof()) << " there are still samples in the reference stream";
}

TEST(DemuxTest, ReadStereoMp3File) {
  const std::string input_file_name = "test_data/kirov.mp3";
  static const std::string cmd =
      "ffmpeg -i test_data/kirov.mp3 -v quiet -f f32le -y -";

  iPipeStream pipe(cmd);
  ASSERT_TRUE(pipe.good())
      << " failed to initialize the pipeline for the reference stream";

  std::ifstream input_file(input_file_name);
  Demux demux(input_file);

  int got_frame_ptr;
  auto codec = demux.GetDecoder(0);
  AudioDecoder<float> audio_codec(codec);

  int index = 0;
  float raw_float[2] = {1, 2};
  const float start_time_seconds = 0.025057;
  while (auto sample = audio_codec.Read()) {
    pipe.read((char*)raw_float, 8);
    ASSERT_EQ(pipe.gcount(), 8) << "end of bytes at " << index;
    ASSERT_EQ(sample->sample(0), raw_float[0])
        << "samples mismatch at " << index;
    ASSERT_EQ(sample->sample(1), raw_float[1])
        << "samples mismatch at " << index;

    ASSERT_THAT(double(sample->time()),
                testing::DoubleNear(index / 44100.0 + start_time_seconds, 1e-6))
        << "time missmatch at " << index;
    ++index;
  }

  pipe.peek();
  ASSERT_TRUE(pipe.eof()) << " there are still samples in the reference stream";
}

}  // namespace potamos