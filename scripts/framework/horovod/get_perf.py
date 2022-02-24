import sys

def main(argv):
    file_name = argv[1]
    print('collect performance data in file: ' + file_name)

    warm_up_iter = 10
    logs = [[]]
    rank_size = 1
    batch_size = -1
    is_hvd = False
    step_offset = 5
    time_offset = 6
    with open(file_name) as f:
        for line in f:
            if line.startswith('[0]'):
                is_hvd = True
                step_offset = 6
                time_offset = 7
            if 'input_batch_size' in line:
                batch_size = int(line.strip().split(' ')[-1])

            line = line.strip()
            if 'INFO:tensorflow:loss' in line:
                rank_id = 0
                ls = line.split(' ')
                step_id = int(ls[step_offset])
                if step_id < warm_up_iter:
                    continue
                if len(ls) <= time_offset:
                    continue
                ts = float(ls[time_offset][1:])
                if is_hvd:
                    rank_id = int(ls[0][1:-1])
                    if rank_id + 1 > rank_size:
                        rank_size = rank_id + 1
                while len(logs) < rank_size:
                    logs.append([])
                logs[rank_id].append(ts * 1000)

    for i in range(rank_size):
        ts = sum(logs[i]) / len(logs[i])
        print('Rank ' + str(i) + ' avg time per batch is : ' + str(round(ts, 2)) + ' ms')
        print('Throught is ' + str(round(batch_size / ts * 1000, 2)) + ' images/second')

if __name__ == '__main__':
  main(argv=sys.argv)
