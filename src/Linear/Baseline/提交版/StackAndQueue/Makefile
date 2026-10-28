# 顶层复现入口：每个检查失败时立即返回非零状态。
BASELINES = baseline1 baseline2 baseline3
CXX = g++
SANFLAGS = -std=c++17 -Wall -O1 -g -fsanitize=address,undefined
BUILD_DIR = .build
SPACE_BIN = $(BUILD_DIR)/space_probe
SAN_BINS = $(addprefix $(BUILD_DIR)/,b1_asan b2_asan b3_asan)
HEADERS := $(shell find src baseline1/src baseline2/src baseline3/src -name "*.hpp")

all: check oracle asan

build:
	@for b in $(BASELINES); do $(MAKE) -C $$b all || exit $$?; done

check:
	@for b in $(BASELINES); do $(MAKE) -C $$b check || exit $$?; done

oracle:
	@for b in $(BASELINES); do $(MAKE) -C $$b oracle || exit $$?; done

unit:
	$(MAKE) -C baseline1 extend
	$(MAKE) -C baseline2 methods extend

regression:
	python3 tools/test_build_failures.py
	python3 tools/test_perf_stats.py

asan: $(SAN_BINS)
	@cd baseline1 && $(abspath $(BUILD_DIR)/b1_asan) > /dev/null && echo "baseline1 ASan/UBSan exit=0"
	@cd baseline2 && $(abspath $(BUILD_DIR)/b2_asan) > /dev/null && echo "baseline2 ASan/UBSan exit=0"
	@cd baseline3 && $(abspath $(BUILD_DIR)/b3_asan) > /dev/null && echo "baseline3 ASan/UBSan exit=0"

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/b1_asan: baseline1/src/main_test.cpp src/array_stack/stackar.cpp src/linked_stack/stackli.cpp $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(SANFLAGS) $(filter %.cpp,$^) -o $@

$(BUILD_DIR)/b2_asan: baseline2/src/main_test.cpp baseline2/src/linked_queue.cpp src/queue/queue.cpp $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(SANFLAGS) $(filter %.cpp,$^) -o $@

$(BUILD_DIR)/b3_asan: baseline3/src/main_test.cpp baseline3/src/stack_by_queue.cpp baseline3/src/queue_by_stack.cpp baseline3/src/vector_containers.cpp $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(SANFLAGS) $(filter %.cpp,$^) -o $@

perf: build
	mkdir -p results
	python3 tools/perf_stats.py baseline1 ./test 30 --ref std_stack --csv results/stack.csv
	python3 tools/perf_stats.py baseline2 ./test 30 --ref std_queue --csv results/queue.csv
	python3 tools/perf_stats.py baseline2 ./queue_methods 30 --ref circular_queue --csv results/queue_methods.csv
	python3 tools/perf_stats.py baseline3 ./test 30 --csv results/conversion.csv

space: $(SPACE_BIN)
	@for m in array linked stdstack circular linkedq stdqueue; do $(SPACE_BIN) $$m 1000000 || exit $$?; done

$(SPACE_BIN): tools/space_probe.cpp src/array_stack/stackar.cpp src/linked_stack/stackli.cpp src/queue/queue.cpp baseline2/src/linked_queue.cpp $(HEADERS) | $(BUILD_DIR)
	$(CXX) -std=c++17 -O2 -Wall $(filter %.cpp,$^) -o $@

clean:
	rm -rf $(BUILD_DIR)
	@for b in $(BASELINES); do $(MAKE) -C $$b clean || exit $$?; done

.PHONY: all build check oracle unit regression asan perf space clean
