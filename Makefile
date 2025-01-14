MAKEFLAGS+=-r -j

config=debug
files=demo/pirate.obj

BUILD=build/$(config)

LIBRARY_SOURCES=$(wildcard src/*.cpp)
LIBRARY_OBJECTS=$(LIBRARY_SOURCES:%=$(BUILD)/%.o)

DEMO_SOURCES=$(wildcard demo/*.c demo/*.cpp) tools/objloader.cpp
DEMO_OBJECTS=$(DEMO_SOURCES:%=$(BUILD)/%.o)

GLTFPACK_SOURCES=$(wildcard gltf/*.cpp)
GLTFPACK_OBJECTS=$(GLTFPACK_SOURCES:%=$(BUILD)/%.o)

OBJECTS=$(LIBRARY_OBJECTS) $(DEMO_OBJECTS) $(GLTFPACK_OBJECTS)

LIBRARY=$(BUILD)/libmeshoptimizer.a
DEMO=$(BUILD)/meshoptimizer

CFLAGS=-g -Wall -Wextra -std=c89
CXXFLAGS=-g -Wall -Wextra -Wshadow -Wno-missing-field-initializers -std=gnu++98
LDFLAGS=

$(GLTFPACK_OBJECTS): CXXFLAGS+=-std=c++11

ifdef BASISU
    $(GLTFPACK_OBJECTS): CXXFLAGS+=-DWITH_BASISU
    $(BUILD)/gltf/basis%.cpp.o: CXXFLAGS+=-I$(BASISU)
    gltfpack: LDFLAGS+=-lpthread

    ifeq ($(HOSTTYPE),x86_64)
        $(BUILD)/gltf/basislib.cpp.o: CXXFLAGS+=-msse4.1
    endif
endif

WASI_SDK?=/opt/wasi-sdk
WASMCC?=$(WASI_SDK)/bin/clang++
WASIROOT?=$(WASI_SDK)/share/wasi-sysroot

WASM_FLAGS=--target=wasm32-wasi --sysroot=$(WASIROOT)
WASM_FLAGS+=-Wall -Wextra
WASM_FLAGS+=-O3 -DNDEBUG -nostartfiles -nostdlib -Wl,--no-entry -Wl,-s
WASM_FLAGS+=-mcpu=mvp # make sure clang doesn't use post-MVP features like sign extension
WASM_FLAGS+=-fno-slp-vectorize -fno-vectorize -fno-unroll-loops
WASM_FLAGS+=-Wl,-z -Wl,stack-size=36864 -Wl,--initial-memory=65536
WASM_EXPORT_PREFIX=-Wl,--export

WASM_DECODER_SOURCES=src/vertexcodec.cpp src/indexcodec.cpp src/vertexfilter.cpp tools/wasmstubs.cpp
WASM_DECODER_EXPORTS=meshopt_decodeVertexBuffer meshopt_decodeIndexBuffer meshopt_decodeIndexSequence meshopt_decodeFilterOct meshopt_decodeFilterQuat meshopt_decodeFilterExp sbrk __wasm_call_ctors

WASM_ENCODER_SOURCES=src/vertexcodec.cpp src/indexcodec.cpp src/vertexfilter.cpp src/vcacheoptimizer.cpp src/vfetchoptimizer.cpp src/spatialorder.cpp tools/wasmstubs.cpp
WASM_ENCODER_EXPORTS=meshopt_encodeVertexBuffer meshopt_encodeVertexBufferBound meshopt_encodeVertexBufferLevel meshopt_encodeIndexBuffer meshopt_encodeIndexBufferBound meshopt_encodeIndexSequence meshopt_encodeIndexSequenceBound meshopt_encodeVertexVersion meshopt_encodeIndexVersion meshopt_encodeFilterOct meshopt_encodeFilterQuat meshopt_encodeFilterExp meshopt_optimizeVertexCache meshopt_optimizeVertexCacheStrip meshopt_optimizeVertexFetchRemap meshopt_spatialSortRemap sbrk __wasm_call_ctors

WASM_SIMPLIFIER_SOURCES=src/simplifier.cpp src/vfetchoptimizer.cpp tools/wasmstubs.cpp
WASM_SIMPLIFIER_EXPORTS=meshopt_simplify meshopt_simplifyWithAttributes meshopt_simplifyScale meshopt_simplifyPoints meshopt_optimizeVertexFetchRemap sbrk __wasm_call_ctors

WASM_CLUSTERIZER_SOURCES=src/clusterizer.cpp tools/wasmstubs.cpp
WASM_CLUSTERIZER_EXPORTS=meshopt_buildMeshletsBound meshopt_buildMeshlets meshopt_computeClusterBounds meshopt_computeMeshletBounds meshopt_optimizeMeshlet sbrk __wasm_call_ctors

ifneq ($(werror),)
	CFLAGS+=-Werror
	CXXFLAGS+=-Werror
endif

ifeq ($(config),iphone)
	IPHONESDK=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk
	CFLAGS+=-arch armv7 -arch arm64 -isysroot $(IPHONESDK)
	CXXFLAGS+=-arch armv7 -arch arm64 -isysroot $(IPHONESDK) -stdlib=libc++
	LDFLAGS+=-arch armv7 -arch arm64 -isysroot $(IPHONESDK) -L $(IPHONESDK)/usr/lib -mios-version-min=7.0
endif

ifeq ($(config),trace)
	CXXFLAGS+=-DTRACE=1
endif

ifeq ($(config),tracev)
	CXXFLAGS+=-DTRACE=2
endif

ifeq ($(config),release)
	CXXFLAGS+=-O3 -DNDEBUG
endif

ifeq ($(config),coverage)
	CXXFLAGS+=-coverage
	LDFLAGS+=-coverage
endif

ifeq ($(config),release-avx512)
	CXXFLAGS+=-O3 -DNDEBUG -mavx512vl -mavx512vbmi -mavx512vbmi2
endif

ifeq ($(config),release-scalar)
	CXXFLAGS+=-O3 -DNDEBUG -DMESHOPTIMIZER_NO_SIMD
endif

ifeq ($(config),coverage-scalar)
	CXXFLAGS+=-coverage -DMESHOPTIMIZER_NO_SIMD
	LDFLAGS+=-coverage
endif

ifeq ($(config),sanitize)
	CXXFLAGS+=-fsanitize=address,undefined -fsanitize-undefined-trap-on-error
	LDFLAGS+=-fsanitize=address,undefined
endif

ifeq ($(config),analyze)
	CXXFLAGS+=--analyze
endif

ifeq ($(config),fuzz)
    CXXFLAGS+=-O1 -fsanitize=address,fuzzer
    LDFLAGS+=-fsanitize=address,fuzzer

    $(GLTFPACK_OBJECTS): CXXFLAGS+=-DGLTFFUZZ
endif

all: $(DEMO)

test: $(DEMO)
	$(DEMO) $(files)

check: $(DEMO)
	$(DEMO)

dev: $(DEMO)
	$(DEMO) -d $(files)

nanite: $(DEMO)
	$(DEMO) -n $(files)

format:
	clang-format -i src/*.h src/*.cpp demo/*.cpp gltf/*.h gltf/*.cpp

formatjs:
	prettier -w js/*.js gltf/*.js demo/*.html js/*.ts

js: js/meshopt_decoder.js js/meshopt_decoder.module.js js/meshopt_encoder.js js/meshopt_encoder.module.js js/meshopt_simplifier.js js/meshopt_simplifier.module.js js/meshopt_clusterizer.js js/meshopt_clusterizer.module.js

symbols: $(BUILD)/amalgamated.so
	nm $< -U -g

gltfpack: $(BUILD)/gltfpack
	ln -fs $^ $@

ifeq ($(config),fuzz)
gltffuzz: $(BUILD)/gltfpack
	cp $^ $@
	mkdir -p /tmp/gltffuzz
	cp gltf/fuzz.glb /tmp/gltffuzz/
	./gltffuzz /tmp/gltffuzz -fork=16 -dict=gltf/fuzz.dict -ignore_crashes=1 -max_len=32768
endif

$(BUILD)/gltfpack: $(GLTFPACK_OBJECTS) $(LIBRARY)
	$(CXX) $^ $(LDFLAGS) -o $@

gltfpack.wasm: gltf/library.wasm

gltf/library.wasm: $(LIBRARY_SOURCES) $(GLTFPACK_SOURCES)
	$(WASMCC) $^ -o $@ -Wall -Os -DNDEBUG --target=wasm32-wasi --sysroot=$(WASIROOT) -nostartfiles -Wl,--no-entry -Wl,--export=pack -Wl,--export=malloc -Wl,--export=free -Wl,--export=__wasm_call_ctors -Wl,-s -Wl,--allow-undefined-file=gltf/wasistubs.txt

build/decoder_base.wasm: $(WASM_DECODER_SOURCES)
	@mkdir -p build
	$(WASMCC) $^ $(WASM_FLAGS) $(patsubst %,$(WASM_EXPORT_PREFIX)=%,$(WASM_DECODER_EXPORTS)) -o $@

build/decoder_simd.wasm: $(WASM_DECODER_SOURCES)
	@mkdir -p build
	$(WASMCC) $^ $(WASM_FLAGS) $(patsubst %,$(WASM_EXPORT_PREFIX)=%,$(WASM_DECODER_EXPORTS)) -o $@ -msimd128 -mbulk-memory

build/encoder.wasm: $(WASM_ENCODER_SOURCES)
	@mkdir -p build
	$(WASMCC) $^ $(WASM_FLAGS) $(patsubst %,$(WASM_EXPORT_PREFIX)=%,$(WASM_ENCODER_EXPORTS)) -lc -o $@

build/simplifier.wasm: $(WASM_SIMPLIFIER_SOURCES)
	@mkdir -p build
	$(WASMCC) $^ $(WASM_FLAGS) $(patsubst %,$(WASM_EXPORT_PREFIX)=%,$(WASM_SIMPLIFIER_EXPORTS)) -lc -o $@

build/clusterizer.wasm: $(WASM_CLUSTERIZER_SOURCES)
	@mkdir -p build
	$(WASMCC) $^ $(WASM_FLAGS) $(patsubst %,$(WASM_EXPORT_PREFIX)=%,$(WASM_CLUSTERIZER_EXPORTS)) -lc -o $@

js/meshopt_decoder.js: build/decoder_base.wasm build/decoder_simd.wasm tools/wasmpack.py
	sed -i "s#Built with clang.*#Built with $$($(WASMCC) --version | head -n 1 | sed 's/\s\+(.*//')#" $@
	sed -i "s#Built from meshoptimizer .*#Built from meshoptimizer $$(cat src/meshoptimizer.h | grep -Po '(?<=version )[0-9.]+')#" $@
	sed -i "s#\([\"']\).*\(;\s*//\s*embed! base\)#\\1$$(cat build/decoder_base.wasm | python3 tools/wasmpack.py)\\1\\2#" $@
	sed -i "s#\([\"']\).*\(;\s*//\s*embed! simd\)#\\1$$(cat build/decoder_simd.wasm | python3 tools/wasmpack.py)\\1\\2#" $@

js/meshopt_encoder.js: build/encoder.wasm tools/wasmpack.py
js/meshopt_simplifier.js: build/simplifier.wasm tools/wasmpack.py
js/meshopt_clusterizer.js: build/clusterizer.wasm tools/wasmpack.py

js/meshopt_encoder.js js/meshopt_simplifier.js js/meshopt_clusterizer.js:
	sed -i "s#Built with clang.*#Built with $$($(WASMCC) --version | head -n 1 | sed 's/\s\+(.*//')#" $@
	sed -i "s#Built from meshoptimizer .*#Built from meshoptimizer $$(cat src/meshoptimizer.h | grep -Po '(?<=version )[0-9.]+')#" $@
	sed -i "s#\([\"']\).*\(;\s*//\s*embed! wasm\)#\\1$$(cat $< | python3 tools/wasmpack.py)\\1\\2#" $@

js/%.module.js: js/%.js
	sed '\#// export!#q' <$< >$@
	sed -i "/use strict.;/d" $@
	sed -i "s#// export! \(.*\)#export { \\1 };#" $@

$(DEMO): $(DEMO_OBJECTS) $(LIBRARY)
	$(CXX) $^ $(LDFLAGS) -o $@

vcachetuner: tools/vcachetuner.cpp tools/objloader.cpp $(LIBRARY)
	$(CXX) $^ -fopenmp $(CXXFLAGS) -std=c++11 $(LDFLAGS) -o $@

codecbench: tools/codecbench.cpp $(LIBRARY)
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@

codecbench.js: tools/codecbench.cpp $(LIBRARY_SOURCES)
	emcc $^ -O3 -g -DNDEBUG -s TOTAL_MEMORY=268435456 -s SINGLE_FILE=1 -o $@

codecbench-simd.js: tools/codecbench.cpp $(LIBRARY_SOURCES)
	emcc $^ -O3 -g -DNDEBUG -s TOTAL_MEMORY=268435456 -s SINGLE_FILE=1 -msimd128 -o $@

codecbench.wasm: tools/codecbench.cpp $(LIBRARY_SOURCES)
	$(WASMCC) $^ -fno-exceptions --target=wasm32-wasi --sysroot=$(WASIROOT) -lc++ -lc++abi -O3 -g -DNDEBUG -o $@

codecbench-simd.wasm: tools/codecbench.cpp $(LIBRARY_SOURCES)
	$(WASMCC) $^ -fno-exceptions --target=wasm32-wasi --sysroot=$(WASIROOT) -lc++ -lc++abi -O3 -g -DNDEBUG -msimd128 -o $@

codectest: tools/codectest.cpp $(LIBRARY)
	$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@

codecfuzz: tools/codecfuzz.cpp src/vertexcodec.cpp src/indexcodec.cpp
	$(CXX) $^ -fsanitize=fuzzer,address,undefined -O1 -g -o $@

simplifyfuzz: tools/simplifyfuzz.cpp src/simplifier.cpp
	$(CXX) $^ -fsanitize=fuzzer,address,undefined -O1 -g -o $@

$(LIBRARY): $(LIBRARY_OBJECTS)
	ar rcs $@ $^

$(BUILD)/amalgamated.so: $(LIBRARY_SOURCES)
	@mkdir -p $(dir $@)
	cat $^ | $(CXX) $(CXXFLAGS) -x c++ - -I src/ -o $@ -shared -fPIC

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $< $(CXXFLAGS) -c -MMD -MP -o $@

$(BUILD)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $< $(CFLAGS) -c -MMD -MP -o $@

-include $(OBJECTS:.o=.d)

clean:
	rm -rf $(BUILD)

.PHONY: all clean format js
