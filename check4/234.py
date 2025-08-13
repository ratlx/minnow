import re
import matplotlib.pyplot as plt
def count_icmp_seq(file_path):
    icmp_seq_list = []

    with open(file_path, 'r') as file:
        for line in file:
            match = re.search(r'icmp_seq=(\d+)', line)
            if match:
                icmp_seq = int(match.group(1))
                icmp_seq_list.append(icmp_seq)

    icmp_seq_list.sort()
    min_seq = icmp_seq_list[0] if icmp_seq_list else None
    max_seq = icmp_seq_list[-1] if icmp_seq_list else None
    total_count = max_seq - min_seq + 1

    missing_seq = []
    if icmp_seq_list:
        for i in range(min_seq, max_seq + 1):
            if i not in icmp_seq_list:
                missing_seq.append(i)

    return {
        'total_count': total_count,
        'min_seq': min_seq,
        'max_seq': max_seq,
        'missing_seq': missing_seq,
        'all_seq': sorted(icmp_seq_list)
    }

def longest_consecutive_pings(missing_seq):
    res = 0
    for i, j in zip(missing_seq, missing_seq[1:]):
        res = max(res, j - i)
    return res

def longest_losses(missing_seq):
    res = 0
    for i in missing_seq:
        if i - 1 in missing_seq:
            continue
        j = i
        len = 1
        while j + 1 in missing_seq:
            len += 1
            j += 1
        res = max(res, len)
    return res

def autocorrelation_of_packet_loss(icmp_seq, missing_seq, k):
    missing_set = set(missing_seq)
    received_set = set(icmp_seq)
    max_seq = max(icmp_seq + missing_seq)

    success_k = [0] * (2 * k + 1)
    loss_k = [0] * (2 * k + 1)

    for i in icmp_seq:
        if i <= k or i > max_seq - k:
            continue
        for j in range(-k, k + 1):
            if (i + j) in received_set:
                success_k[j + k] += 1

    for i in missing_seq:
        if i <= k or i > max_seq - k:
            continue
        for j in range(-k, k + 1):
            if (i + j) in missing_set:
                loss_k[j + k] += 1

    success_prob = [count / success_k[k] if success_k[k] > 0 else 0 for count in success_k]
    loss_prob = [count / loss_k[k] if loss_k[k] > 0 else 0 for count in loss_k]

    return success_prob, loss_prob

def plot_probabilities(success_prob, loss_prob, k):
    x_values = list(range(-k, k + 1))

    plt.figure(figsize=(10, 6))

    plt.plot(x_values, success_prob, label='Success Probability', marker='o')
    plt.plot(x_values, loss_prob, label='Loss Probability', marker='s')

    plt.xlabel('Offset')
    plt.ylabel('Probability')
    plt.title('Autocorrelation of Packet Loss')
    plt.legend()
    plt.grid(True)
    plt.xticks(x_values)

    plt.show()

file_path = 'data.txt'
result = count_icmp_seq(file_path)

print(f'最长连续ping数：{longest_consecutive_pings(result["missing_seq"])}')
print(f'最长连续丢包数：{longest_losses(result["missing_seq"])}')

print(f'总成功概率：{len(result["all_seq"]) / result["total_count"]}')

k = 10
success_prob, loss_prob = autocorrelation_of_packet_loss(result['all_seq'], result['missing_seq'], k)
plot_probabilities(success_prob, loss_prob, k)
print(success_prob, loss_prob)