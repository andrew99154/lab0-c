import subprocess
import re

NUM_RUNS = 10
delta_times = []

for i in range(NUM_RUNS):
    result = subprocess.run(["./qtest", "-f", "traces/trace-sort.cmd"],
                            capture_output=True, text=True)
    output = result.stdout

    matches = re.findall(r"Delta time\s*=\s*([\d\.]+)", output)
    if len(matches) < 2:
        print(f"{i+1}th test: No Delta time finded.")
        continue

    delta_time = float(matches[1])
    delta_times.append(delta_time)
    print(f"{i+1}th test: Delta time = {delta_time}")

if delta_times:
    avg = sum(delta_times) / len(delta_times)
    print(f"\naverage Delta time of {len(delta_times)} of sorting: {avg}")
else:
    print("No Delta time finded.")
