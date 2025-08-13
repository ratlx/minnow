#!/usr/bin/env bash
set -euo pipefail

HOST="${1:-www.canterbury.ac.nz}"   # 目标
OUT="${2:-burst_results.csv}"       # 输出CSV

# 注意：-s 不要超过 1400
SIZES=(512 1024 1400)
# 间隔（秒），小于0.2时请用sudo运行整段脚本
INTERVALS=(0.05 0.02 0.005 0.002)

# 每组实验的持续时间（秒）。我们用 -c ≈ duration/interval 来控制次数
DURATION=8

echo "host,size_bytes,interval_s,transmitted,replied,start_epoch,end_epoch,duration_s,req_rate_Bps,rep_rate_Bps" > "$OUT"

for SZ in "${SIZES[@]}"; do
  for INT in "${INTERVALS[@]}"; do
    COUNT=$(python3 - <<PY
import math
print(max(1, math.floor($DURATION / $INT)))
PY
)
    # 计时（整段命令的墙钟时间）
    START=$(date +%s.%N)
    # -D 打印时间戳；-n 不做反查；-s 负载字节；-i 间隔；-c 次数
    # 有些系统ping在stdout输出统计，某些在stderr，这里两者都抓
    OUTLOG=$(mktemp)
    if ping -D -n -s "$SZ" -i "$INT" -c "$COUNT" "$HOST" >"$OUTLOG" 2>&1; then
      : # 正常
    else
      : # 即使非零退出也继续解析
    fi
    END=$(date +%s.%N)

    # 解析 transmitted / received
    TX=$(grep -Eo '[0-9]+ packets transmitted' "$OUTLOG" | awk '{print $1}' | tail -1)
    RX=$(grep -Eo '[0-9]+ received' "$OUTLOG" | awk '{print $1}' | tail -1)

    # 计算持续时间（墙钟）
    DUR=$(python3 - <<PY
from decimal import Decimal
print( (Decimal("$END") - Decimal("$START")).quantize(Decimal("0.000001")) )
PY
)

    # 数据率（Bytes/s）
    REQ_RATE=$(python3 - <<PY
sz=$SZ
tx=${TX:-0}
dur=float("$DUR") if float("$DUR")>0 else 1.0
print( sz*tx/dur )
PY
)
    REP_RATE=$(python3 - <<PY
sz=$SZ
rx=${RX:-0}
dur=float("$DUR") if float("$DUR")>0 else 1.0
print( sz*rx/dur )
PY
)

    echo "$HOST,$SZ,$INT,${TX:-0},${RX:-0},$START,$END,$DUR,$REQ_RATE,$REP_RATE" >> "$OUT"
    rm -f "$OUTLOG"
    sleep 1  # 小憩，避免太密集
  done
done

echo "Done -> $OUT"