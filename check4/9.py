import re
import numpy as np
import matplotlib.pyplot as plt

def extract_rtt_values(file_path):
    RTTs = []

    with open(file_path, 'r') as file:
        for line in file:
            match = re.search(r'time=(\d+) ms', line)
            if match:
                rtt_value = int(match.group(1))
                RTTs.append(rtt_value)

    return RTTs

file_path = 'data.txt'
RTTs = extract_rtt_values(file_path)

x = RTTs[:-1]
y = RTTs[1:]

plt.figure(figsize=(10, 8))
plt.scatter(x, y, alpha=0.5, s=10)
plt.xlabel('RTT of ping #N (ms)')
plt.ylabel('RTT of ping #N+1 (ms)')
plt.title('Correlation between consecutive RTTs')

z = np.polyfit(x, y, 1)
p = np.poly1d(z)
plt.plot(x, p(x), "r--", alpha=0.8, linewidth=2, label=f'Trend line')

correlation_coefficient = np.corrcoef(x, y)[0, 1]
plt.text(0.05, 0.95, f'Correlation coefficient: {correlation_coefficient:.3f}',
         transform=plt.gca().transAxes, fontsize=12,
         verticalalignment='top', bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))

plt.grid(True, alpha=0.3)
plt.legend()
plt.tight_layout()
plt.show()