import re
import matplotlib.pyplot as plt
import numpy as np

def extract_rtt_values(file_path):
    RTTs = []

    with open(file_path, 'r') as file:
        for line in file:
            match = re.search(r'time=(\d+) ms', line)
            if match:
                rtt_value = int(match.group(1))
                RTTs.append(rtt_value)

    return RTTs

def plot_cdf(data, title="Cumulative Distribution Function"):
    sorted_data = np.sort(data)

    y = np.arange(1, len(sorted_data) + 1) / len(sorted_data)

    plt.figure(figsize=(10, 6))
    plt.plot(sorted_data, y, linewidth=2)
    plt.xlabel('RTT (ms)')
    plt.ylabel('Cumulative Probability')
    plt.title(title)
    plt.grid(True, alpha=0.3)

    mean_val = np.mean(sorted_data)
    median_val = np.median(sorted_data)
    min_val = np.min(sorted_data)
    max_val = np.max(sorted_data)

    plt.axvline(mean_val, color='r', linestyle='--',
                label=f'Mean: {mean_val:.2f} ms')
    plt.axvline(median_val, color='g', linestyle='--',
                label=f'Median: {median_val:.2f} ms')

    plt.legend()

    stats_text = f'Statistics:\nMin: {min_val} ms\nMax: {max_val} ms\nMean: {mean_val:.2f} ms\nMedian: {median_val:.2f} ms'
    plt.text(0.02, 0.98, stats_text, transform=plt.gca().transAxes,
             verticalalignment='top', bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))

    plt.tight_layout()
    plt.show()

    return sorted_data, y

file_path = 'data.txt'
RTTs = extract_rtt_values(file_path)

sorted_rtt, cumulative_prob = plot_cdf(RTTs, "Cumulative Distribution Function of RTTs")

mean_rtt = np.mean(RTTs)
median_rtt = np.median(RTTs)

if mean_rtt > median_rtt:
    print("\nDistribution shape: Right-skewed (positively skewed)")
    print("This indicates that most RTT values are relatively low,")
    print("but there are some high RTT values that pull the mean up.")
elif mean_rtt < median_rtt:
    print("\nDistribution shape: Left-skewed (negatively skewed)")
    print("This indicates that most RTT values are relatively high,")
    print("but there are some low RTT values that pull the mean down.")
else:
    print("\nDistribution shape: Symmetric")
    print("This indicates a relatively symmetric distribution of RTT values.")
