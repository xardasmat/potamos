#include "ipstream.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

namespace potamos {
namespace {

TEST(iPipeStreamTest, SimplePipeline) {
  iPipeStream pipe("echo hello world!");
  std::string line = "?";
  std::getline(pipe, line);
  EXPECT_EQ(line, "hello world!");
}

TEST(iPipeStreamTest, MultipleLines) {
  iPipeStream pipe("echo welcome;echo to the; echo real world");
  std::string line = "?";
  std::getline(pipe, line);
  EXPECT_EQ(line, "welcome");
  std::getline(pipe, line);
  EXPECT_EQ(line, "to the");
  std::getline(pipe, line);
  EXPECT_EQ(line, "real world");
}

}  // namespace
}  // namespace potamos