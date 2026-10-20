import sys
import pandas as pd

def main():
    if len(sys.argv) < 2:
        print("Usage: parse.py cpu_M{M}.txt", file=sys.stderr)
        sys.exit(1)

    filename = sys.argv[1]
    rows = []
    idle_idx = None

    with open(filename, encoding="utf-8", errors="replace") as f:
        for line in f:
            parts = line.split()
            if not parts:
                continue

            if "CPU" in parts and "%idle" in parts:
                idle_idx = parts.index("%idle")
                continue

            if parts[0] in ("Linux", "Average:", "Среднее:"):
                continue
            if "CPU" in parts[:3]:
                continue

            if len(parts) < 3:
                continue

            cpu = parts[2]

            try:
                idle = float(parts[idle_idx]) if idle_idx is not None else float(parts[-1])
            except (ValueError, IndexError):
                continue

            usage = 100.0 - idle
            rows.append([cpu, usage])

    if not rows:
        print(f"Warning: no data parsed from {filename}", file=sys.stderr)
        sys.exit(1)

    df = pd.DataFrame(rows, columns=["CPU", "Usage"])
    df["Time"] = df.groupby("CPU").cumcount()
    df = df[["Time", "CPU", "Usage"]]

    csv_filename = filename.rsplit(".", 1)[0] + ".csv"
    df.to_csv(csv_filename, index=False)
    print(f"{filename} -> {csv_filename} сохранён.")


if __name__ == "__main__":
    main()
