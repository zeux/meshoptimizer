.SUFFIXES:
MAKEFLAGS+=-r

config=release

BUILD=build/$(config)

SOURCES=$(wildcard src/*.cpp demo/*.cpp)
OBJECTS=$(SOURCES:%=$(BUILD)/%.o)

EXECUTABLE=$(BUILD)/meshoptimizer

CXXFLAGS=-g -Wall -Wextra -Wno-missing-field-initializers -Werror -std=c++11
LDFLAGS=

ifeq ($(config),release)
	CXXFLAGS+=-O3 -DNDEBUG
endif

ifeq ($(config),coverage)
	CXXFLAGS+=-coverage
	LDFLAGS+=-coverage
endif

ifeq ($(config),sanitize)
	CXXFLAGS+=-fsanitize=address
	LDFLAGS+=-fsanitize=address

	ifneq ($(shell uname),Darwin)
		CXXFLAGS+=-fsanitize=undefined
		LDFLAGS+=-fsanitize=undefined
	endif
endif

ifeq ($(config),analyze)
	CXXFLAGS+=--analyze
endif

all: $(EXECUTABLE)

test: $(EXECUTABLE)
	$(EXECUTABLE) demo/bunny.obj

format:
	clang-format -i $(SOURCES)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILD)/%.o: %
	@mkdir -p $(dir $@)
	$(CXX) $< $(CXXFLAGS) -c -MMD -MP -o $@

-include $(OBJECTS:.o=.d)
clean:
	rm -rf $(BUILD)

.PHONY: all clean format
