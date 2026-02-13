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
    while (!frame_) {
      frame_ = decoder_.Read();
      if (!frame_) return std::nullopt;
      if (frame_->data()->flags & AV_FRAME_FLAG_DISCARD) {
        frame_ = std::nullopt;
        continue;
      }
      size_ = frame_->data()->nb_samples;
      int64_t skip_ = 0;
      AVFrameSideData* sd =
          av_frame_get_side_data(frame_->data(), AV_FRAME_DATA_SKIP_SAMPLES);
      if (sd) {
        uint32_t* skip = (uint32_t*)sd->data;
        skip_ += skip[0];
        size_ -= skip[1];
      }
      while (skip_ > frame_->data()->nb_samples) {
        skip_ -= frame_->data()->nb_samples;
        frame_ = decoder_.Read();
        if (!frame_) return std::nullopt;
        size_ = frame_->data()->nb_samples;
      }
      index_ = skip_;
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
    if (index_ >= size_) {
      frame_ = std::nullopt;
    }
    return sample;
  }

  int Channels() const { return decoder_.data()->ch_layout.nb_channels; }

 protected:
  Decoder& decoder_;
  bool planar_;
  int64_t index_;
  int64_t size_;
  std::optional<Frame> frame_;
};

template <typename SampleType>
class AudioEncoder {
 public:
  AudioEncoder(Encoder& encoder)
      : encoder_(encoder),
        planar_(av_sample_fmt_is_planar(encoder_.data()->sample_fmt)) {
    encoder_.data()->time_base = av_make_q(1, encoder_.data()->sample_rate);
  }

  void Write(const AudioSample<SampleType>& sample) {
    if (!frame_) {
      frame_ = encoder_.MakeFrame();
      index_ = 0;
    }
    if (planar_) {
      for (int i = 0; i < Channels(); ++i) {
        ((SampleType*)frame_->data()->data[i])[index_] = sample.sample(i);
      }
    } else {
      for (int i = 0; i < Channels(); ++i) {
        ((SampleType*)frame_->data()->data[0])[index_ * Channels() + i] =
            sample.sample(i);
      }
    }

    ++index_;
    if (index_ >= frame_->data()->nb_samples) {
      WriteCurrentFrame();
    }
  }

  void Flush() {
    if (!frame_) {
      int ret = encoder_.Flush();
      if (ret < 0)
        std::cerr << "Flushing encoder failed = " << ret << std::endl;
    }
    WriteCurrentFrame();
    int ret = encoder_.Flush();
    if (ret < 0) std::cerr << "Flushing encoder failed = " << ret << std::endl;
  }

  int Channels() const { return encoder_.data()->ch_layout.nb_channels; }

 protected:
  void WriteCurrentFrame() {
    frame_->data()->nb_samples = std::min(index_, frame_->data()->nb_samples);
    frame_->data()->pts = samples_written_;
    encoder_.Write(*frame_);
    frame_ = std::nullopt;
    samples_written_ += index_;
  }

  Encoder& encoder_;
  int index_;
  std::optional<Frame> frame_;
  bool planar_;
  int64_t samples_written_ = 0;
};

}  // namespace potamos