import os
from pathlib import Path
import subprocess
import sys


REPO = Path(__file__).resolve().parent.parent
DATA_DIR = REPO / "data"
LOW_BIN = REPO / "low_layer" / "bin" / "pfsp_sdst"

ALGO_DEFS = {
    "neh": ["neh"],
    "simulated_annealing": ["simulated_annealing"],
    "neh + simulated_annealing": ["neh", "simulated_annealing"]
}

ALGORITHMS = list(ALGO_DEFS.items())


def list_data_files():
    files = sorted([p for p in DATA_DIR.glob("*.txt")])
    print("\nAvailable data files in data/:")
    for i, p in enumerate(files):
        print(f"  [{i}] {p.name}")
    return files


def list_algorithms():
    print("\nAvailable algorithms:")
    for i, (canon, _info) in enumerate(ALGORITHMS):
        print(f"  [{i}] {canon}")
    return ALGORITHMS


def input_index(prompt, max_index):
    while True:
        s = input(prompt).strip()
        if s == '':
            return None
        if s.isdigit():
            v = int(s)
            if 0 <= v <= max_index:
                return v
            print("Index out of range")
        else:
            return s


def run_build():
    build_script = REPO / 'low_layer' / 'build.sh'
    print("Building...")
    rc = subprocess.run(['bash', str(build_script)])
    return rc.returncode == 0


def run_binary(data_path: str, algorithm_parts):
    run_script = REPO / 'low_layer' / 'run.sh'
    cmd = ['bash', str(run_script), data_path] + list(algorithm_parts)
    print("Running...")
    p = subprocess.run(cmd)
    return p.returncode


def main():
    print("Interactive low_layer runner")
    files = list_data_files()
    visible_algorithms = list_algorithms()

    print("\nSelect data file by index. Press Enter to cancel file selection.")
    sel = input_index("Data file index: ", len(files)-1)
    if sel is None:
        print("Canceled."); return
    if not isinstance(sel, int):
        print("Please enter a data file index (number).")
        return
    data_path = str(files[sel].resolve())

    print("\nSelect algorithm by index. Press Enter to cancel algorithm selection.")
    alg_sel = input_index("Algorithm index: ", len(visible_algorithms)-1)
    if alg_sel is None:
        print("Canceled."); return
    if not isinstance(alg_sel, int):
        print("Please enter an algorithm index (number).")
        return
    algorithm_parts = visible_algorithms[alg_sel][1]

    ans = input('\nBuild before run? (Y/n): ')
    if ans == '' or ans.startswith('y'):
        ok = run_build()
        if not ok:
            print('Build failed — aborting')
            return

    rc = run_binary(data_path, algorithm_parts)
    print(f"\nProcess exited with code {rc}")

if __name__ == '__main__':
    main()
