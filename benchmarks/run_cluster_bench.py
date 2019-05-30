#!/usr/bin/env python

"""Run a specific benchmark in cluster mode
"""

import os
import stat
import sys
import multiprocessing
import subprocess
import argparse


def find_bench(name):
    name = name + "_clustered"
    executable = stat.S_IEXEC | stat.S_IXGRP | stat.S_IXOTH
    for filename in os.listdir('.'):
        if os.path.isfile(filename):
            st = os.stat(filename)
            mode = st.st_mode
            if mode & executable and filename.endswith(name):
                print("Found benchmark executable at {}".format(filename))
                return filename
    print("Could not find benchmark named {}".format(name))
    exit(1)


def invoke_initiator(executable, cpus, port):
    cpustr = ','.join(map(str, cpus))

    invocation = ["./" + executable,
                  "--smp", str(len(cpus)),
                  "--cpuset", cpustr,
                  "-l", "127.0.0.1:{}".format(port),
                  "--minimum-peers", str(multiprocessing.cpu_count() / len(cpus) - 1),
                  "--initiator", "1"]

    print("\033[1m\033[94mStarting bootstrap instance: {}\033[0m".format(" ".join(invocation)))
    return subprocess.Popen(invocation)


def invoke_peer(executable, cpus, initiator_addr, port):
    cpustr = ','.join(map(str, cpus))
    invocation = ["./" + executable,
                  "--smp", str(len(cpus)),
                  "--cpuset", cpustr,
                  "-l", "127.0.0.1:{}".format(port),
                  "--minimum-peers", str(multiprocessing.cpu_count() / len(cpus) - 1),
                  "--peers", initiator_addr]

    print("\033[1m\033[94mStarting instance: {}\033[0m".format(" ".join(invocation)))
    return subprocess.Popen(invocation)


def main(arguments):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('-b', '--benchmark', help="Benchmark name to run")
    parser.add_argument('-c', '--smp', help="Number of thread per instance. Must be divisible by system cpu count",
                        type=int, default=1)
    args = parser.parse_args(arguments)

    executable = find_bench(args.benchmark)

    initiator_port = 5000
    initiator = invoke_initiator(executable, list(range(0, args.smp)), initiator_port)

    peers = list()

    for i in range(args.smp, multiprocessing.cpu_count(), args.smp):
        peers.append(
            invoke_peer(executable, list(range(i, i + args.smp)), "127.0.0.1:{}".format(initiator_port), 5000 + i))

    initiator.wait()

    for peer in peers:
        peer.kill()


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
