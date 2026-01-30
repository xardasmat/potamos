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

class Subtitle {
 public:
  Subtitle() {}

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
  int64_t begin_timestamp;
  int64_t end_timestamp;
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

    subtitle.begin_timestamp = frame->start_display_time + frame->pts;
    subtitle.end_timestamp = frame->end_display_time + frame->pts;
    return subtitle;
  }

 protected:
  Decoder& decoder_;
  std::optional<Frame> frame_;
};
