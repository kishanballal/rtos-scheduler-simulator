"""Plot timelines exported by `rtos_sim --export timeline.csv` as Gantt charts.

Usage:
    pip install matplotlib
    python scripts/plot_timeline.py timeline.csv [output.png]

One subplot per scheduler, one row per task. Uses only the standard library
plus matplotlib.
"""

import csv
import sys
from collections import OrderedDict

import matplotlib.pyplot as plt


def load(path):
    """Return {scheduler: [(tick, task_name), ...]} keeping file order."""
    runs = OrderedDict()
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            runs.setdefault(row["scheduler"], []).append((int(row["tick"]), row["task_name"]))
    return runs


def to_segments(ticks):
    """Merge consecutive ticks of the same task into (task, start, length) bars."""
    segments = []
    for tick, name in ticks:
        if name == "idle":
            continue
        if segments and segments[-1][0] == name and segments[-1][1] + segments[-1][2] == tick:
            task, start, length = segments[-1]
            segments[-1] = (task, start, length + 1)
        else:
            segments.append((name, tick, 1))
    return segments


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    runs = load(sys.argv[1])
    tasks = sorted({name for ticks in runs.values() for _, name in ticks if name != "idle"})
    colors = {t: plt.cm.tab10(i % 10) for i, t in enumerate(tasks)}

    fig, axes = plt.subplots(len(runs), 1, figsize=(12, 1.2 + 0.5 * len(tasks) * len(runs)),
                             sharex=True, squeeze=False)
    for ax, (scheduler, ticks) in zip(axes[:, 0], runs.items()):
        for task, start, length in to_segments(ticks):
            ax.broken_barh([(start, length)], (tasks.index(task) - 0.4, 0.8),
                           facecolors=colors[task], edgecolor="black", linewidth=0.5)
        ax.set_yticks(range(len(tasks)))
        ax.set_yticklabels(tasks)
        ax.invert_yaxis()
        ax.set_title(scheduler, loc="left", fontsize=10)
        ax.grid(axis="x", linestyle=":", alpha=0.6)
    axes[-1, 0].set_xlabel("tick")
    fig.tight_layout()

    if len(sys.argv) > 2:
        fig.savefig(sys.argv[2], dpi=150)
        print("saved", sys.argv[2])
    else:
        plt.show()


if __name__ == "__main__":
    main()
