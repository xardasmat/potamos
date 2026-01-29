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

#include "mux.hpp"
#include "audio.hpp"

using testing::ElementsAre;
using testing::NotNull;

TEST(MuxTest, WriteBoingWav) {
  const std::string output_file_name = "test_data/boing.wav";
  std::ofstream output_file(output_file_name, std::ios::out | std::ios::trunc);
  //   std::vector<const AVCodec*> codecs =
  //   {avcodec_find_encoder_by_name("libmp3lame")}; ASSERT_THAT(codecs,
  //   ElementsAre(NotNull()));


  std::clog << "Codec" << std::endl;
  // const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
  const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
  
  AVCodecContext* ctx = avcodec_alloc_context3(codec);

  AVCodecParameters* params = avcodec_parameters_alloc();
  std::clog << "avcodec_parameters_from_context" << std::endl;
  avcodec_parameters_from_context(params, ctx);
  std::clog << "avcodec_free_context" << std::endl;
  avcodec_free_context(&ctx);

  params->bit_rate = 128000;
  params->sample_rate = 44100;
  params->format = AVSampleFormat::AV_SAMPLE_FMT_S16;
  av_channel_layout_default(&params->ch_layout, 1);
  params->bits_per_coded_sample = 16;
  params->block_align = 2;
  // params->ch_layout.nb_channels = 1;
  // params->ch_layout.order = AV_CHANNEL_ORDER_NATIVE;
  // params->ch_layout.opaque 

  std::clog << "Mux" << std::endl;
  Mux mux(output_file, "wav", {params});
  std::clog << "Encoder" << std::endl;
  Encoder encoder = mux.MakeEncoder(0);
  std::clog << "AudioEncoder" << std::endl;
  AudioEncoder<int16_t> audio(encoder);

  for (int i=0;i<44100*1;++i) {
    AudioSample<int16_t> sample(1);
    sample.sample(0) = int16_t(sin(float(i)/44100*3.14*2*1000)/exp(i/44100.0*10)*20000);
    // std::clog << i << " :: " << sample.sample(0) << std::endl;
    audio.Write(sample);
    if (auto pkt = encoder.Read()) {
      std::clog << "New packet :D" << std::endl;
      mux.Write(std::move(*pkt));
    } else {
      // std::clog << i << ":" << " *" << std::endl;
    }
  }
  audio.Flush();

  // EXPECT_THAT(mux ??? );
}


TEST(MuxTest, WriteBoingMp3) {
  const std::string output_file_name = "test_data/boing.mp3";
  std::ofstream output_file(output_file_name, std::ios::out | std::ios::trunc);
  //   std::vector<const AVCodec*> codecs =
  //   {avcodec_find_encoder_by_name("libmp3lame")}; ASSERT_THAT(codecs,
  //   ElementsAre(NotNull()));


  std::clog << "Codec" << std::endl;
  // const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
  const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
  
  AVCodecContext* ctx = avcodec_alloc_context3(codec);

  AVCodecParameters* params = avcodec_parameters_alloc();
  std::clog << "avcodec_parameters_from_context" << std::endl;
  avcodec_parameters_from_context(params, ctx);
  std::clog << "avcodec_free_context" << std::endl;
  avcodec_free_context(&ctx);

  params->bit_rate = 128000;
  params->sample_rate = 44100;
  params->format = AVSampleFormat::AV_SAMPLE_FMT_S16;
  av_channel_layout_default(&params->ch_layout, 1);
  // params->bits_per_coded_sample = 16;
  // params->block_align = 2;
  // params->ch_layout.nb_channels = 1;
  // params->ch_layout.order = AV_CHANNEL_ORDER_NATIVE;
  // params->ch_layout.opaque 

  std::clog << "Mux" << std::endl;
  Mux mux(output_file, "mp3", {params});
  std::clog << "Encoder" << std::endl;
  Encoder encoder = mux.MakeEncoder(0);
  std::clog << "AudioEncoder" << std::endl;
  AudioEncoder<int16_t> audio(encoder);

  for (int i=0;i<44100*1;++i) {
    AudioSample<int16_t> sample(1);
    sample.sample(0) = int16_t(sin(float(i)/44100*3.14*2*1000)/exp(i/44100.0*10)*20000);
    // std::clog << i << " :: " << sample.sample(0) << std::endl;
    audio.Write(sample);
    if (auto pkt = encoder.Read()) {
      std::clog << "New packet :D" << std::endl;
      mux.Write(std::move(*pkt));
    } else {
      // std::clog << i << ":" << " *" << std::endl;
    }
  }
  audio.Flush();
}