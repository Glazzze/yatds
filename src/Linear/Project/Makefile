# 祖玛综合项目构建文件
#
# 一条命令完成编译：make      （生成 zuma 与 test）
# 运行样例比对：        make check（遍历 inputs/，逐字节比对 outputs/）
# core 算法自测：       make selftest
# 链式栈专项测试：     make stacktest
# core 随机差分测试：   make difftest（默认 10^5 局；DIFF_N=1000 可缩短）
# driver 集成测试：     make drivertest
# 重新生成标准输出：    make samples（维护用，生成后须人工核对）
# 网页样例内嵌：        make web-samples（inputs/ → web/assets/samples-data.js）
# 网页双实现交叉验证：  make webcheck（JS 协议 vs C++ --plain）
# 网页 JS 单元测试：    make webtest
# 网页静态一致性检查：  make webstatic（id/脚本顺序/资源/语法，无浏览器）
# 网页 DOM 集成冒烟：   make webdom（无浏览器环境的 app/view 主流程）
# 网页全套验证：        make webverify（web-samples + webcheck + webrandom + webtest
#                                       + webstatic + webdom）
# 真实浏览器验收：      make browsercheck（可选，需先安装 Playwright +
#                                       Chromium；不在 webverify 内）
# 清理构建产物：        make clean
#
# CLI 渲染（阶段 3）：./zuma 默认在终端下进入交互渲染（--ui 强制、
# --plain 纯协议、NO_COLOR=1 关色）；重定向/管道时为纯协议输出，
# test 的逐字节比对不受颜色影响。
#
# 撤销栈在 src/core/linked_stack.hpp 内完整定义，不依赖项目外源码。

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

CORE_SRCS = src/core/track.cpp \
            src/core/game.cpp
CORE_HEADERS = $(wildcard src/core/*.hpp)
DRIVER_HEADERS = $(wildcard src/driver/*.hpp)

ZUMA_SRCS = src/main.cpp \
            src/driver/protocol_io.cpp \
            src/driver/cli_render.cpp

SELFTEST_SRCS = tools/core_selftest.cpp
DIFFTEST_SRCS = tools/random_diff.cpp
DRIVERTEST_SRCS = tools/driver_test.cpp
TEST_SRCS = tools/test_runner.cpp

INCLUDES = -Isrc -Isrc/core

TARGET = zuma
SELFTEST = tools/selftest
DIFFTEST = tools/random_diff
DIFF_N ?= 100000
DRIVERTEST = tools/driver_test
TEST = test
STACKTEST = tools/linked_stack_test
SCALEPROBE = tools/scale_probe
NODE = node

.PHONY: all check samples selftest stacktest difftest drivertest clean scalecheck \
        web-samples webcheck webrandom webtest webstatic webdom webverify browsercheck

all: $(TARGET) $(TEST)

$(TARGET): $(ZUMA_SRCS) $(CORE_SRCS) $(CORE_HEADERS) $(DRIVER_HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(ZUMA_SRCS) $(CORE_SRCS) -o $(TARGET)

# core 层 27 场景算法自测（独立程序，与协议 driver 解耦）
$(SELFTEST): $(SELFTEST_SRCS) $(CORE_SRCS) $(CORE_HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SELFTEST_SRCS) $(CORE_SRCS) -o $(SELFTEST)

selftest: $(SELFTEST)
	./$(SELFTEST)

$(STACKTEST): tools/linked_stack_test.cpp src/core/linked_stack.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) tools/linked_stack_test.cpp -o $(STACKTEST)

stacktest: $(STACKTEST)
	./$(STACKTEST)

# core 与独立字符串参考差分（固定种子 20251020；撤销还原 / 非法零副作用 / 重放确定性）
$(DIFFTEST): $(DIFFTEST_SRCS) tools/reference_game.hpp $(CORE_SRCS) $(CORE_HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(DIFFTEST_SRCS) $(CORE_SRCS) -o $(DIFFTEST)

difftest: $(DIFFTEST)
	./$(DIFFTEST) -n $(DIFF_N)

# driver 协议集成测试（27 用例 + 颜色开关断言，子进程方式跑 zuma）
$(DRIVERTEST): $(DRIVERTEST_SRCS) tools/executable_path.hpp $(TARGET)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(DRIVERTEST_SRCS) -o $(DRIVERTEST)

drivertest: $(DRIVERTEST) $(TARGET)
	./$(DRIVERTEST)

# 阶段 4：test 程序遍历 inputs/ 并与 outputs/ 逐字节比对（T4.3）
$(TEST): $(TEST_SRCS) tools/executable_path.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(TEST_SRCS) -o $(TEST)

check: $(TARGET) $(TEST)
	./$(TEST)

# 重新生成 outputs/（首版由程序产出，随后人工核对语义，T4.2）
samples: $(TARGET)
	@for f in inputs/*.txt; do \
		echo "生成 outputs/$$(basename $$f)"; \
		NO_COLOR=1 ./$(TARGET) --plain < $$f > outputs/$$(basename $$f); \
	done

# 阶段 5b：网页样例、双实现交叉验证与 JS 引擎测试
web-samples:
	$(NODE) tools/gen_web_samples.js

webcheck: $(TARGET)
	$(NODE) tools/web_crosscheck.js

webrandom: $(TARGET)
	$(NODE) tools/web_random_crosscheck.js

$(SCALEPROBE): tools/scale_probe.cpp $(CORE_SRCS) $(CORE_HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) tools/scale_probe.cpp $(CORE_SRCS) -o $(SCALEPROBE)

scalecheck: $(SCALEPROBE)
	$(NODE) tools/scale_observe.js

webtest:
	$(NODE) tools/web_engine_test.js

webstatic:
	$(NODE) tools/web_static_check.js

webdom:
	$(NODE) tools/web_dom_smoke.js

webverify: web-samples webcheck webrandom webtest webstatic webdom

# 真实 Chromium 验收（可选依赖 Playwright，故不并入 webverify）：
#   npm install --no-save playwright
#   npx playwright install chromium
#   npx playwright install-deps chromium   # 容器首次运行
browsercheck:
	$(NODE) tools/web_browser_acceptance.js

clean:
	rm -f $(TARGET) $(SELFTEST) $(DIFFTEST) $(DRIVERTEST) $(TEST) $(STACKTEST) $(SCALEPROBE)
