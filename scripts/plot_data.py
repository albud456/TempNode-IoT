"""Render temperature and humidity from data.csv to a PNG.

Usage: python plot.py <data.csv> <out.png> [days] [timezone]

data.csv has the header "dt,metric,value", where dt is a UTC epoch in seconds
and metric is "temperature" or "humidity".
"""
import csv
import sys
from datetime import datetime, timedelta, timezone
from zoneinfo import ZoneInfo

import matplotlib

matplotlib.use("Agg")  # headless runner: no display
import matplotlib.dates as mdates
import matplotlib.pyplot as plt

csv_path = sys.argv[1]
out_path = sys.argv[2]
days = int(sys.argv[3]) if len(sys.argv) > 3 else 7
tz_name = sys.argv[4] if len(sys.argv) > 4 else "UTC"
tz = ZoneInfo(tz_name)

cutoff = datetime.now(timezone.utc) - timedelta(days=days)

series = {"temperature": [], "humidity": []}
skipped = 0
with open(csv_path, newline="") as f:
    for row in csv.DictReader(f):
        try:
            metric = row["metric"]
            if metric not in series:
                continue
            when = datetime.fromtimestamp(int(row["dt"]), tz=timezone.utc)
            value = float(row["value"])
        except (KeyError, TypeError, ValueError):
            skipped += 1  # a malformed row must not stop the daily chart
            continue
        if when >= cutoff:
            series[metric].append((when.astimezone(tz), value))

if skipped:
    print(f"skipped {skipped} malformed row(s)")

if not any(series.values()):
    print("no data in range, not drawing a chart")
    sys.exit(0)

fig, axes = plt.subplots(2, 1, sharex=True, figsize=(9, 5))
panels = [
    ("temperature", "Temperature (°C)", "tab:red"),
    ("humidity", "Humidity (%)", "tab:blue"),
]
for ax, (metric, label, color) in zip(axes, panels):
    points = sorted(series[metric])
    if points:
        ax.plot(
            [p[0] for p in points],
            [p[1] for p in points],
            color=color,
            marker=".",
            linewidth=1,
        )
    else:
        ax.text(0.5, 0.5, "no data", ha="center", va="center", transform=ax.transAxes)
    ax.set_ylabel(label)
    ax.grid(True, alpha=0.3)

updated_utc = datetime.now(timezone.utc).strftime("%Y-%m-%d at %H:%M UTC")
axes[0].set_title(f"TempNode data -- {days} days -- last updated: {updated_utc}")
axes[1].xaxis.set_major_formatter(mdates.DateFormatter("%d.%m %H:%M", tz=tz))
axes[1].set_xlabel(tz_name)
fig.autofmt_xdate()
fig.tight_layout()
fig.savefig(out_path, dpi=120)
print(f"wrote {out_path}")