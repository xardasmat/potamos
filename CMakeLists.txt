cmake_minimum_required(VERSION 3.23)

project(Potamos)

find_package(GTest REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(AVCODEC		REQUIRED IMPORTED_TARGET libavcodec)
pkg_check_modules(AVFORMAT		REQUIRED IMPORTED_TARGET libavformat)
pkg_check_modules(AVFILTER		REQUIRED IMPORTED_TARGET libavfilter)
pkg_check_modules(AVDEVICE		REQUIRED IMPORTED_TARGET libavdevice)
pkg_check_modules(AVUTIL		REQUIRED IMPORTED_TARGET libavutil)
pkg_check_modules(SWRESAMPLE		REQUIRED IMPORTED_TARGET libswresample)
pkg_check_modules(SWSCALE		REQUIRED IMPORTED_TARGET libswscale)
add_library(FFmpeg INTERFACE IMPORTED GLOBAL)
target_link_libraries(FFmpeg INTERFACE
	PkgConfig::AVCODEC
	PkgConfig::AVFORMAT
	PkgConfig::AVFILTER
	PkgConfig::AVDEVICE
	PkgConfig::AVUTIL
	PkgConfig::SWRESAMPLE
	PkgConfig::SWSCALE
)

enable_testing()

add_executable(
  unit_tests
  src/demux_test.cc
  src/mux_test.cc
  src/subtitle_test.cc
)

file(COPY test_data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(
  unit_tests
  FFmpeg
  GTest::gtest_main
  GTest::gmock_main
)

