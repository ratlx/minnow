import csv
from collections import defaultdict
import matplotlib.pyplot as plt

xs = []  # req rate (Bytes/s)
ys = []  # rep rate (Bytes/s)
labels = []  # annotate by (size, interval)

with open("burst_results.csv", newline="") as f:
    r = csv.DictReader(f)
    for row in r:
        req = float(row["req_rate_Bps"])
        rep = float(row["rep_rate_Bps"])
        xs.append(req)
        ys.append(rep)
        labels.append(f's={row["size_bytes"]}, i={row["interval_s"]}')

plt.figure()
plt.scatter(xs, ys)
for x, y, lab in zip(xs, ys, labels):
    plt.annotate(lab, (x, y), xytext=(5, 5), textcoords="offset points")

# 画 y=x 参考线（理想无丢包、不限速时回包率≈发包率）
lim = max(xs + ys) if xs and ys else 1.0
plt.plot([0, lim], [0, lim])

plt.xlabel("Echo request data rate (Bytes/s)")
plt.ylabel("Echo reply data rate (Bytes/s)")
plt.title("ICMP Reply Throughput vs Request Throughput (<10s bursts)")
plt.grid(True)
plt.show()