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


def invoke_initiator(executable, cpu, port):
    return subprocess.Popen(["./" + executable,
                             "--smp", "1",
                             "--cpuset", str(cpu),
                             "-l", "127.0.0.1:{}".format(port),
                             "--minimum-peers", str(multiprocessing.cpu_count() - 1),
                             "--initiator", "1"])


def invoke_peer(executable, cpu, initiator_addr, port):
    return subprocess.Popen(["./" + executable,
                             "--smp", "1",
                             "--cpuset", str(cpu),
                             "-l", "127.0.0.1:{}".format(port),
                             "--peers", initiator_addr])


def main(arguments):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('-b', '--benchmark', help="Benchmark name to run")
    args = parser.parse_args(arguments)

    executable = find_bench(args.benchmark)

    initiator_port = 5000
    initiator = invoke_initiator(executable, 0, initiator_port)

    peers = list()
    for i in range(1, multiprocessing.cpu_count()):
        peers.append(invoke_peer(executable, i, "127.0.0.1:{}".format(initiator_port), 5000 + i))

    initiator.wait()

    for peer in peers:
        peer.kill()


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
