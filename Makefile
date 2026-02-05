
rebuild_all:
	cmake -DCMAKE_BUILD_TYPE=Release -H. -Bbuild && cmake --build build -- -j4

rebuild_debug:
	cmake -DCMAKE_BUILD_TYPE=Debug -H. -Bdebug && cmake --build debug -- -j4

test: rebuild_all
	cd build && ./unit_tests

debug_test: rebuild_debug
	cd debug && ./unit_tests
