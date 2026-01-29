
rebuild_all:
	cmake -DCMAKE_BUILD_TYPE=Release -H. -Bbuild && cmake --build build -- -j4

test: rebuild_all
	cd build && ./unit_tests
