#pragma once

#include <fstream>
#include <functional>
#include <iostream>
#include <optional>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
}

#include "decoder.hpp"
#include "encoder.hpp"

template <typename SampleType>
class AudioSample {
 public:
  AudioSample(int channels) : samples_(channels, SampleType()) {}

  SampleType& sample(int channel) { return samples_[channel]; }
  const SampleType& sample(int channel) const { return samples_[channel]; }

 private:
  std::vector<SampleType> samples_;
};

template <typename SampleType>
class AudioDecoder {
 public:
  AudioDecoder(Decoder& decoder) : decoder_(decoder) {}
  std::optional<AudioSample<SampleType>> Read() {
    if (!frame_) {
      frame_ = decoder_.Read();
      index_ = 0;
      if (!frame_) return std::nullopt;
    }
    AudioSample<SampleType> sample(Channels());
    for (int i = 0; i < Channels(); ++i) {
      sample.sample(i) = ((SampleType*)frame_->data()->data[i])[index_];
    }
    ++index_;
    if (index_ >= frame_->data()->nb_samples) {
      frame_ = std::nullopt;
    }
    return sample;
  }

  int Channels() const { return decoder_.data()->ch_layout.nb_channels; }

 protected:
  Decoder& decoder_;
  int index_;
  std::optional<Frame> frame_;
};

template <typename SampleType>
class AudioEncoder {
 public:
  AudioEncoder(Encoder& encoder) : encoder_(encoder) {}

  void Write(const AudioSample<SampleType>& sample) {
    if (!frame_) {
      frame_ = encoder_.MakeFrame();
      index_ = 0;
    }
    for (int i = 0; i < Channels(); ++i) {
      ((SampleType*)frame_->data()->data[i])[index_] = sample.sample(i);
    }
    ++index_;
    if (index_ >= frame_->data()->nb_samples) {
      // frame_->data()->nb_samples = index_;
      encoder_.Write(*frame_);
      frame_ = std::nullopt;
    }
  }

  void Flush() {
    if (!frame_) {
      return;
    }
    frame_->data()->nb_samples = index_;
    encoder_.Write(*frame_);
    frame_ = std::nullopt;
  }

  int Channels() const { return encoder_.data()->ch_layout.nb_channels; }

 protected:
  Encoder& encoder_;
  int index_;
  std::optional<Frame> frame_;
};
