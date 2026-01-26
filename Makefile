
# main: main.cpp
# 	g++ main.cpp --std=c++20 -lavcodec -lavformat -lz -lavutil -o main

build_all:
	cmake --build build -- -j4

rebuild_all:
	cmake -DCMAKE_BUILD_TYPE=Release -H. -Bbuild && cmake --build build -- -j4

test: build_all
	./build/unit_tests