"""End-to-end checks of the terminal menu and integrated module behavior."""
import re
import subprocess
import tempfile
from pathlib import Path


def run(text):
    return subprocess.run(["./build/deadlock_manager"], input=text, text=True,
                          capture_output=True, check=True, timeout=5).stdout


output = run("3\n2\nscenarios/safe.txt\n5\n1\n1 0 0\n7\n8\n9\n4\n1\n0 0 0\n0\n")
assert "Enter or load a valid scenario first." in output
assert "Resources released." in output
assert "not implemented yet" not in output
assert "Safe sequence:" in output
assert "Request granted." in output
assert "Deadlocked processes:" not in output
assert "No confirmed deadlock under the current-request completion assumption." in output
output = run("2\nscenarios/safe.txt\n2\nscenarios/invalid.txt\n3\n0\n")
assert "Scenario rejected:" in output and "Previous state preserved." in output
assert output.count("Processes: 5") == 2
output = run("1\n2\n1\n3\n3\n1\n0\n4\n3\n1\n0\n2\n3\n0\n")
assert "Scenario loaded." in output and "Processes: 2" in output
output = run("2\nscenarios/unsafe.txt\n7\n8\n9\n0\n")
assert "Unsafe: no guaranteed safe sequence." in output
assert "No confirmed deadlock under the current-request completion assumption." in output
assert "Terminated P" not in output

# Grant updates resource accounting.
output = run("2\nscenarios/safe.txt\n4\n1\n1 0 2\n0\n")
assert "Request granted." in output
assert re.search(r"Available\s+2\s+3\s+0", output)

# An unsafe allocation is rolled back, with the outstanding request retained.
output = run("2\nscenarios/safe.txt\n4\n4\n3 3 0\n0\n")
assert "Request deferred: tentative allocation is unsafe." in output
assert len(re.findall(r"Available\s+3\s+3\s+2", output)) == 2
pending = output.rsplit("Request (currently outstanding)", 1)[1]
assert re.search(r"P4\s+3\s+3\s+0", pending)

# An unavailable request waits; after another process completes, retry it.
output = run("2\nscenarios/safe.txt\n4\n0\n4 0 0\n6\n1\n4\n0\n4 0 0\n0\n")
assert "Request deferred: insufficient available resources." in output
assert "Request granted." in output
assert re.search(r"Available\s+1\s+3\s+2", output)
pending = output.rsplit("Request (currently outstanding)", 1)[1]
assert re.search(r"P0\s+0\s+0\s+0", pending)

# A second recovery call must retain the first action, and a new scenario resets it.
output = run("2\nscenarios/deadlocked.txt\n8\n9\n8\n9\n10\n2\nscenarios/safe.txt\n10\n0\n")
assert "Deadlocked processes: P0 P1" in output
assert "No confirmed deadlock under the current-request completion assumption." in output
assert output.count("Terminated P1; work lost: 2; resources returned: 0 1") == 3
assert output.count("No recovery actions recorded.") == 1

output = run("2\nscenarios/safe.txt\n5\n1\n99 0 0\n4\n1\n-1 0 0\n4\n1\n99 0 0\n0\n")
assert "Release must be nonnegative and cannot exceed allocation." in output
assert "Invalid vector." in output
assert "Request exceeds remaining Need. State unchanged." in output
output = run("word\n99\n0\n")
assert output.count("Enter one menu number") == 2
for contents in ("2 1\n", "1 1 1 1 0 0 0 EXTRA", "1 1 -1 1 0 0 0", "1 1 X"):
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / "bad.txt"
        path.write_text(contents)
        assert "Scenario rejected:" in run(f"2\n{path}\n0\n")
print("CLI scenario, avoidance, detection, recovery history, and input checks passed.")
