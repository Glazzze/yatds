# StackAndQueue 顶层复现入口
#
# make check    三个 baseline 的样例比对（make check）
# make oracle   三个 baseline 的独立 oracle 校验（非自洽）
# make asan     三个 baseline 的 ASan/UBSan 内存与 UB 检查
# make perf     性能：均值 ± 总体标准差 + Welch t 检验（不等方差两样本）
# make space    内存开销探针：数组 / 链表 / std 的 bytes-per-element
# make all      依次跑 check / oracle / asan
# make clean    清理顶层构建产物（各 baseline 产物各自 make clean）

BASELINES = baseline1 baseline2 baseline3
CXX       = g++
CXXFLAGS  = -std=c++17 -Wall -O2 -flto
SANFLAGS  = -std=c++17 -Wall -O1 -g -fsanitize=address,undefined
SPACE_BIN = /tmp/space_probe

all: check oracle asan

check:
	@for b in $(BASELINES); do echo "===== $$b ====="; $(MAKE) -C $$b check; done

oracle:
	@for b in $(BASELINES); do echo "===== $$b ====="; $(MAKE) -C $$b oracle; done

asan: /tmp/_sq_b1_asan /tmp/_sq_b2_asan /tmp/_sq_b3_asan
	@cd baseline1 && /tmp/_sq_b1_asan >/dev/null; echo "baseline1 ASan/UBSan exit=$$?"
	@cd baseline2 && /tmp/_sq_b2_asan >/dev/null; echo "baseline2 ASan/UBSan exit=$$?"
	@cd baseline3 && /tmp/_sq_b3_asan >/dev/null; echo "baseline3 ASan/UBSan exit=$$?"

/tmp/_sq_b1_asan: baseline1/src/main_test.cpp src/array_stack/stackar.cpp src/linked_stack/stackli.cpp
	$(CXX) $(SANFLAGS) $^ -o $@

/tmp/_sq_b2_asan: baseline2/src/main_test.cpp baseline2/src/linked_queue.cpp src/queue/queue.cpp
	$(CXX) $(SANFLAGS) $^ -o $@

/tmp/_sq_b3_asan: baseline3/src/main_test.cpp baseline3/src/stack_by_queue.cpp baseline3/src/queue_by_stack.cpp baseline3/src/vector_containers.cpp
	$(CXX) $(SANFLAGS) $^ -o $@

perf:
	python3 tools/perf_stats.py baseline1 ./test 30 --ref std_stack
	python3 tools/perf_stats.py baseline2 ./test 30 --ref std_queue
	python3 tools/perf_stats.py baseline2 ./queue_methods 30 --ref circular_queue
	python3 tools/perf_stats.py baseline3 ./test 30

space: $(SPACE_BIN)
	@echo "===== 栈 ====="
	@for m in array linked stdstack; do $(SPACE_BIN) $$m 1000000; done
	@echo "===== 队列 ====="
	@for m in circular linkedq stdqueue; do $(SPACE_BIN) $$m 1000000; done

$(SPACE_BIN): tools/space_probe.cpp src/array_stack/stackar.cpp src/linked_stack/stackli.cpp src/queue/queue.cpp baseline2/src/linked_queue.cpp
	$(CXX) -std=c++17 -O2 -Wall $^ -o $@

clean:
	rm -f /tmp/_sq_b1_asan /tmp/_sq_b2_asan /tmp/_sq_b3_asan $(SPACE_BIN)

.PHONY: all check oracle asan perf space clean