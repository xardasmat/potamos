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
#include "mux.hpp"

using testing::ElementsAre;
using testing::NotNull;

TEST(MuxTest, WriteBoingWav) {
  const std::string output_file_name = "test_data/boing.wav";
  std::ofstream output_file(output_file_name, std::ios::out | std::ios::trunc);

  const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
  AVCodecContext* ctx = avcodec_alloc_context3(codec);
  AVCodecParameters* params = avcodec_parameters_alloc();
  avcodec_parameters_from_context(params, ctx);
  avcodec_free_context(&ctx);

  params->bit_rate = 128000;
  params->sample_rate = 44100;
  params->format = AVSampleFormat::AV_SAMPLE_FMT_S16;
  av_channel_layout_default(&params->ch_layout, 1);
  params->bits_per_coded_sample = 16;
  params->block_align = 2;

  Mux mux(output_file, "wav", {params});
  Encoder encoder = mux.MakeEncoder(0);
  AudioEncoder<int16_t> audio(encoder);

  for (int i = 0; i < 44100 * 1; ++i) {
    AudioSample<int16_t> sample(1);
    sample.sample(0) = int16_t(sin(float(i) / 44100 * 3.14 * 2 * 1000) /
                               exp(i / 44100.0 * 10) * 20000);
    audio.Write(sample);
    if (auto pkt = encoder.Read()) {
      mux.Write(std::move(*pkt));
    }
  }
  audio.Flush();
}

TEST(MuxTest, WriteBoingMp3) {
  const std::string output_file_name = "test_data/boing.mp3";
  std::ofstream output_file(output_file_name, std::ios::out | std::ios::trunc);
  const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MP3);

  AVCodecContext* ctx = avcodec_alloc_context3(codec);

  AVCodecParameters* params = avcodec_parameters_alloc();
  avcodec_parameters_from_context(params, ctx);
  avcodec_free_context(&ctx);

  params->bit_rate = 128000;
  params->sample_rate = 44100;
  params->format = AVSampleFormat::AV_SAMPLE_FMT_S16;
  av_channel_layout_default(&params->ch_layout, 1);

  Mux mux(output_file, "mp3", {params});
  Encoder encoder = mux.MakeEncoder(0);
  AudioEncoder<int16_t> audio(encoder);

  for (int i = 0; i < 44100 * 1; ++i) {
    AudioSample<int16_t> sample(1);
    sample.sample(0) = int16_t(sin(float(i) / 44100 * 3.14 * 2 * 1000) /
                               exp(i / 44100.0 * 10) * 20000);
    audio.Write(sample);
    while (auto pkt = encoder.Read()) {
      mux.Write(std::move(*pkt));
    }
  }
  audio.Flush();
  while (auto pkt = encoder.Read()) {
    mux.Write(std::move(*pkt));
  }
}
