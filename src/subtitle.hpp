#pragma once

#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <regex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
}

#include "decoder.hpp"
#include "encoder.hpp"

namespace potamos {

class Subtitle {
 public:
  Subtitle() : begin_timestamp(0, 1), end_timestamp(0, 1) {}

  static std::string FromAss(const std::string& ass) {
    std::smatch ass_match;
    std::regex regex_ass(
        "[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,(.*)");
    if (std::regex_match(ass, ass_match, regex_ass)) {
      return ass_match[1];
    }
    return "";
  }

 public:
  std::string text;
  Rational<int64_t> begin_timestamp;
  Rational<int64_t> end_timestamp;
};

class SubtitleDecoder {
 public:
  SubtitleDecoder(Decoder& decoder) : decoder_(decoder) {}

  std::optional<Subtitle> Read() {
    std::optional<AVSubtitle> frame = decoder_.ReadSub();
    if (!frame) return std::nullopt;
    std::unique_ptr<AVSubtitle, decltype(&avsubtitle_free)> av_sum_deleter(
        &*frame, avsubtitle_free);
    Subtitle subtitle;
    if (frame->num_rects != 1) {
      std::clog << __FILE__ << ":" << __LINE__ << " "
                << "frame->num_rects  = " << frame->num_rects << std::endl;
      return std::nullopt;
    }
    switch (frame->rects[0]->type) {
      case SUBTITLE_TEXT:
        subtitle.text = frame->rects[0]->text;
        break;
      case SUBTITLE_ASS:
        subtitle.text = Subtitle::FromAss(frame->rects[0]->ass);
        break;
      default:
        return std::nullopt;
    }

    subtitle.begin_timestamp =
        Rational<int64_t>(frame->start_display_time, 1000);
    subtitle.end_timestamp = Rational<int64_t>(frame->end_display_time, 1000);
    if (frame->pts != AV_NOPTS_VALUE) {
      subtitle.begin_timestamp += Rational<int64_t>(frame->pts, AV_TIME_BASE);
      subtitle.end_timestamp += Rational<int64_t>(frame->pts, AV_TIME_BASE);
    }
    return subtitle;
  }

 protected:
  Decoder& decoder_;
  std::optional<Frame> frame_;
};

}  // namespace potamos