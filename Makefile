CMAKE_COMMON_FLAGS ?=
CMAKE_DEBUG_FLAGS ?=
CMAKE_RELEASE_FLAGS ?=

NPROCS ?= $(shell nproc)

# NOTE: use Makefile.local for customization
-include Makefile.local
# My own Build configurations

###
###
###

build_log: clear-build-log
	@mkdir -p build_log
	@cp scripts/run_compile.sh build_log
	@cd build_log && \
	  cmake -DCAMKE_BUILD_TYPE=Release -DCMAKE_OPTIMIZE=None $(CMAKE_COMMON_FLAGS) .. && \
	  bash run_compile.sh

#debug_build:
#	@mkdir -p build_debug
#	@cp scripts/run_compile.sh build_debug
#	@cd build_debug && \
# 	  cmake -DCMAKE_BUILD_TYPE=Debug $(CMAKE_COMMON_FLAGS) $(CMAKE_DEBUG_FLAGS) .. && \
# 	  bash run_compile.sh

build_O3: clear-build-O3
	@mkdir -p build_O3
	@cp scripts/run_compile.sh build_O3
	@cd build_O3 && \
	  cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_OPTIMIZE=O3 $(CMAKE_COMMON_FLAGS) .. && \
	  bash run_compile.sh

build_O2: clear-build-O2
	@mkdir -p build_O2
	@cp scripts/run_compile.sh build_O2
	@cd build_O2 && \
	  cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_OPTIMIZE=O2 $(CMAKE_COMMON_FLAGS) .. && \
	  bash run_compile.sh

build_O1: clear-build-01
	@mkdir -p build_O1
	@cp scripts/run_compile.sh build_O1
	@cd build_O1 && \
	  cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_OPTIMIZE=O1 $(CMAKE_COMMON_FLAGS) .. && \
	  bash run_compile.sh

clear-build-log:
	@rm -rf build_log

clear-build-O3:
	@rm -rf build_O3

clear-build-O2:
	@rm -rf build_O2

clear-build-01:
	@rm -rf build_O1

clear:
	@rm -rf build_*

###
###
###