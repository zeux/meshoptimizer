.SUFFIXES:
MAKEFLAGS+=-r

config=debug
files=demo/pirate.obj

BUILD=build/$(config)

LIBRARY_SOURCES=$(wildcard src/*.cpp)
LIBRARY_OBJECTS=$(LIBRARY_SOURCES:%=$(BUILD)/%.o)

DEMO_SOURCES=$(wildcard demo/*.c demo/*.cpp)
DEMO_OBJECTS=$(DEMO_SOURCES:%=$(BUILD)/%.o)

OBJECTS=$(LIBRARY_OBJECTS) $(DEMO_OBJECTS)

LIBRARY=$(BUILD)/libmeshoptimizer.a
EXECUTABLE=$(BUILD)/meshoptimizer

CFLAGS=-g -Wall -Wextra -Werror -std=c89
CXXFLAGS=-g -Wall -Wextra -Wno-missing-field-initializers -Werror -std=c++98
LDFLAGS=

ifeq ($(config),release)
	CXXFLAGS+=-O3 -DNDEBUG
endif

ifeq ($(config),coverage)
	CXXFLAGS+=-coverage
	LDFLAGS+=-coverage
endif

ifeq ($(config),sanitize)
	CXXFLAGS+=-fsanitize=address,undefined -fno-sanitize-recover=all
	LDFLAGS+=-fsanitize=address,undefined
endif

ifeq ($(config),analyze)
	CXXFLAGS+=--analyze
endif

all: $(EXECUTABLE)

test: $(EXECUTABLE)
	$(EXECUTABLE) $(files)

dev: $(EXECUTABLE)
	$(EXECUTABLE) -d $(files)

format:
	clang-format -i $(SOURCES)

meshencoder: $(LIBRARY) $(BUILD)/demo/objparser.cpp.o tools/meshencoder.cpp
	$(CXX) tools/meshencoder.cpp $(LIBRARY) $(BUILD)/demo/objparser.cpp.o $(CXXFLAGS) $(LDFLAGS) -o meshencoder

$(EXECUTABLE): $(DEMO_OBJECTS) $(LIBRARY)
	$(CXX) $(DEMO_OBJECTS) $(LIBRARY) $(LDFLAGS) -o $@

$(LIBRARY): $(LIBRARY_OBJECTS)
	ar rcs $@ $^

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $< $(CXXFLAGS) -c -MMD -MP -o $@

$(BUILD)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $< $(CFLAGS) -c -MMD -MP -o $@

-include $(OBJECTS:.o=.d)

clean:
	rm -rf $(BUILD)

.PHONY: all clean format
