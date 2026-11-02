"""Fan the YZ1 search out across cores by sharding on the first character."""
import os, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(HERE, 'yzcrack.exe')
ALPHA_LEN = {0: 26, 1: 26, 2: 10, 3: 62, 4: 95, 5: 73}


def main():
    arc, cs, lo, hi = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4])
    nshard = int(sys.argv[5]) if len(sys.argv) > 5 else min(16, ALPHA_LEN[cs])
    nshard = max(1, min(nshard, ALPHA_LEN[cs]))

    print('sharding %s across %d workers (charset %d, lengths %d..%d)'
          % (os.path.basename(arc), nshard, cs, lo, hi), flush=True)

    procs = []
    t0 = time.time()
    for i in range(nshard):
        procs.append(subprocess.Popen(
            [EXE, arc, str(cs), str(lo), str(hi), str(i)],
            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True, creationflags=subprocess.IDLE_PRIORITY_CLASS))

    found = None
    for p in procs:
        for line in p.stdout:
            if 'PASSWORD FOUND' in line:
                found = line.strip()
                break
        if found:
            break

    for p in procs:
        if found:
            p.kill()
        try:
            p.wait(timeout=10)
        except Exception:
            p.kill()

    dt = time.time() - t0
    print('elapsed %.1fs' % dt, flush=True)
    if found:
        print(found)
        return 0
    print('not found in this space')
    return 1


if __name__ == '__main__':
    sys.exit(main())