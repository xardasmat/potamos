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
#include <libavutil/intreadwrite.h>
}

#include "decoder.hpp"
#include "encoder.hpp"
#include "rational.hpp"

namespace potamos {

template <typename SampleType>
class AudioSample {
 public:
  AudioSample(int channels) : samples_(channels, SampleType()), time_(0, 1) {}

  SampleType& sample(int channel) { return samples_[channel]; }
  const SampleType& sample(int channel) const { return samples_[channel]; }
  Rational<int64_t>& time() { return time_; }
  const Rational<int64_t>& time() const { return time_; }

 private:
  std::vector<SampleType> samples_;
  Rational<int64_t> time_;
};

template <typename SampleType>
class AudioDecoder {
 public:
  AudioDecoder(Decoder& decoder)
      : decoder_(decoder),
        planar_(av_sample_fmt_is_planar(decoder_.data()->sample_fmt)) {}
  std::optional<AudioSample<SampleType>> Read() {
    if (!frame_) {
      frame_ = decoder_.Read();
      index_ = 0;
      if (!frame_) return std::nullopt;
    }
    AudioSample<SampleType> sample(Channels());
    if (planar_) {
      for (int i = 0; i < Channels(); ++i) {
        sample.sample(i) = ((SampleType*)frame_->data()->data[i])[index_];
        sample.time() =
            Rational<int64_t>(frame_->data()->pts, 1) * decoder_.TimeBase() +
            Rational<int64_t>(index_, frame_->data()->sample_rate);
      }
    } else {
      for (int i = 0; i < Channels(); ++i) {
        sample.sample(i) =
            ((SampleType*)frame_->data()->data[0])[index_ * Channels() + i];
        sample.time() =
            Rational<int64_t>(frame_->data()->pts, 1) * decoder_.TimeBase() +
            Rational<int64_t>(index_, frame_->data()->sample_rate);
      }
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
  bool planar_;
  int64_t index_;
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
      encoder_.Write(*frame_);
      frame_ = std::nullopt;
    }
  }

  void Flush() {
    if (!frame_) {
      int ret = encoder_.Flush();
      if (ret < 0)
        std::cerr << "Flushing encoder failed = " << ret << std::endl;
    }
    frame_->data()->nb_samples = index_;
    encoder_.Write(*frame_);
    frame_ = std::nullopt;
    int ret = encoder_.Flush();
    if (ret < 0) std::cerr << "Flushing encoder failed = " << ret << std::endl;
  }

  int Channels() const { return encoder_.data()->ch_layout.nb_channels; }

 protected:
  Encoder& encoder_;
  int index_;
  std::optional<Frame> frame_;
};

}  // namespace potamos