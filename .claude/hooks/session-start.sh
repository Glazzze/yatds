#!/usr/bin/env bash
# 容器重建后 ~/.claude 会被清空，本脚本在每次会话启动时把
# 会话记录 / auto-memory（以及缺失时的用户级设置）重新链接到工作区持久卷。
# 幂等：已经是符号链接时不做任何事。
set -u

WS="${CLAUDE_PROJECT_DIR:-/home/yatcc-ds}/.claude"
HOME_DIR="${HOME:-/root}"
D="$HOME_DIR/.claude/projects"
S="$HOME_DIR/.claude/settings.json"

mkdir -p "$WS/projects" "$HOME_DIR/.claude"

# 1) ~/.claude/projects -> 工作区
if [ ! -L "$D" ]; then
  # 若本地已存在真实目录，先合并进工作区（不覆盖已有文件），再替换为链接
  if [ -d "$D" ]; then
    cp -an "$D"/. "$WS/projects"/ 2>/dev/null || true
  fi
  rm -rf "$D"
  ln -s "$WS/projects" "$D"
fi

# 2) ~/.claude/settings.json -> 工作区（仅在完全不存在时；存在则不动）
if [ ! -e "$S" ] && [ ! -L "$S" ]; then
  ln -s "$WS/settings.json" "$S"
fi

exit 0