import re
import datetime
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

def extract_time_values(file_path):
    timestamps = []
    RTTs = []

    with open(file_path, 'r') as file:
        for line in file:
            timestamp_match = re.search(r'\[(\d+\.\d+)\]', line)
            rtt_match = re.search(r'time=(\d+) ms', line)

            if timestamp_match and rtt_match:
                timestamp = float(timestamp_match.group(1))
                rtt = int(rtt_match.group(1))

                timestamps.append(timestamp)
                RTTs.append(rtt)

    return timestamps, RTTs

def convert_timestamp_to_datetime(timestamp):
    return datetime.datetime.fromtimestamp(timestamp)

file_path = 'data.txt'
timestamps, RTTs = extract_time_values(file_path)

print(f'min RTT:, {min(RTTs)}')
print(f'max RTT:, {max(RTTs)}')

datetimes = [convert_timestamp_to_datetime(ts) for ts in timestamps]

plt.figure(figsize=(12, 6))
plt.plot(datetimes, RTTs, marker='o', linestyle='-', markersize=2, linewidth=0.5)
plt.xlabel('Time of Day')
plt.ylabel('RTT (ms)')
plt.title('RTT as a Function of Time')
plt.grid(True)

plt.gca().xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
plt.gca().xaxis.set_major_locator(mdates.MinuteLocator(interval=5))
plt.gcf().autofmt_xdate()

plt.text(0.02, 0.98, f'Min RTT: {min(RTTs)} ms\nMax RTT: {max(RTTs)} ms\nAvg RTT: {sum(RTTs)/len(RTTs):.2f} ms',
         transform=plt.gca().transAxes, verticalalignment='top',
         bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))

plt.tight_layout()
plt.show()
