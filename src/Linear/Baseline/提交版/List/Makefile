# 顶层便捷入口：一条命令编译 / 测试 / 内存检查三个 Baseline
#
#   make        为三个 Baseline 分别编译
#   make check  三个 Baseline 依次运行样例比对
#   make unit   三个 Baseline 依次运行单元/手动验证
#   make asan   三个 Baseline 依次运行 ASan/LSan 内存检查
#   make clean  清理三个 Baseline 的构建产物
#
# 各 Baseline 亦可单独进入其目录执行同名目标。提交材料本身相互独立，
# 本文件仅为审查便利而设，不改变各 Baseline 的自包含性。

BASELINES = baseline1 baseline2 baseline3

.PHONY: all check unit asan clean $(BASELINES)

all:
	@for b in $(BASELINES); do \
		echo "===== $$b: build ====="; \
		$(MAKE) -C $$b all || exit 1; \
	done

check:
	@for b in $(BASELINES); do \
		echo "===== $$b: check ====="; \
		$(MAKE) -C $$b check || exit 1; \
	done

unit:
	@for b in $(BASELINES); do \
		echo "===== $$b: unit ====="; \
		$(MAKE) -C $$b unit || exit 1; \
	done

asan:
	@for b in $(BASELINES); do \
		echo "===== $$b: asan ====="; \
		$(MAKE) -C $$b asan || exit 1; \
	done

clean:
	@for b in $(BASELINES); do $(MAKE) -C $$b clean; done