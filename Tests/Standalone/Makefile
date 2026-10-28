# Standalone tests for the engine-independent core (VedriVPToolkitEditor/Private/Core).
# Needs only a C++20 compiler: Xcode command line tools on the Mac, clang or gcc on Linux.
#
#   make            build and run the tests with clang++ (the default), then check that every
#                   validation code is catalogued in docs/VALIDATION_CODES.md (needs python3)
#   make CXX=g++    the same with gcc
#   make sanitize   run under AddressSanitizer + UndefinedBehaviorSanitizer
#   make all-compilers  clang++ and g++ (if both are installed)
#   make presets    regenerate VedriVPToolkit/Resources/Presets/*.json from BundledPresetDefinitions.h
#
# Flags mirror Unreal's defaults for the code under test: C++20, no exceptions, no RTTI, warnings as
# errors, plus shadowing and conversion warnings (MSVC C4456-C4459 and C4244/C4267 equivalents).

ifeq ($(origin CXX),default)
CXX := clang++
endif

REPO_ROOT := $(abspath ../..)
CORE_INCLUDE := $(REPO_ROOT)/VedriVPToolkit/Source/VedriVPToolkitEditor/Private
PRESET_DIR := $(REPO_ROOT)/VedriVPToolkit/Resources/Presets
BUILD_DIR := build/$(notdir $(CXX))

WARNINGS := -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wsign-conversion -Wnon-virtual-dtor \
	-Wold-style-cast -Wcast-qual -Wundef -Wdouble-promotion -Wimplicit-fallthrough
ifneq (,$(findstring clang,$(shell $(CXX) --version 2>/dev/null)))
WARNINGS += -Wshadow-all -Wcomma -Wextra-semi
endif

CXXFLAGS_BASE := -std=c++20 -fno-exceptions -fno-rtti $(WARNINGS) \
	-I$(CORE_INCLUDE) -I. -include PoisonMacros.h \
	-DVEDRI_PRESET_DIR='"$(PRESET_DIR)"'
CXXFLAGS ?= -O1 -g

TEST_SOURCES := TestMain.cpp TestJson.cpp TestTiming.cpp TestWallLayout.cpp TestPreset.cpp TestChangeSet.cpp
CORE_HEADERS := $(wildcard $(CORE_INCLUDE)/Core/*.h)
DEPS := $(CORE_HEADERS) TestHarness.h PoisonMacros.h BundledPresetDefinitions.h

.PHONY: test sanitize all-compilers presets clean

test: $(BUILD_DIR)/vp_core_tests
	./$(BUILD_DIR)/vp_core_tests
	python3 -I check_validation_codes.py $(REPO_ROOT)

$(BUILD_DIR)/vp_core_tests: $(TEST_SOURCES) $(DEPS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS_BASE) $(CXXFLAGS) $(TEST_SOURCES) -o $@

sanitize:
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS_BASE) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer $(TEST_SOURCES) -o $(BUILD_DIR)/vp_core_tests_san
	./$(BUILD_DIR)/vp_core_tests_san

all-compilers:
	$(MAKE) test CXX=clang++
	@if command -v g++ >/dev/null 2>&1; then $(MAKE) test CXX=g++; else echo "g++ not installed, skipped"; fi

presets: $(BUILD_DIR)/write_bundled_presets
	./$(BUILD_DIR)/write_bundled_presets $(PRESET_DIR)

$(BUILD_DIR)/write_bundled_presets: Tools/WriteBundledPresets.cpp $(DEPS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS_BASE) $(CXXFLAGS) Tools/WriteBundledPresets.cpp -o $@

clean:
	rm -rf build
