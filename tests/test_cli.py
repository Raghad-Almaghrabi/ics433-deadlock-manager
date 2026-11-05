"""End-to-end checks for loading, rejected input, mutation and stub honesty."""
import subprocess

def run(text):
    return subprocess.run(["./build/deadlock_manager"], input=text, text=True,
                          capture_output=True, check=True, timeout=5).stdout

output = run("3\n2\nscenarios/safe.txt\n5\n1\n1 0 0\n7\n8\n9\n4\n1\n0 0 0\n0\n")
assert "Enter or load a valid scenario first." in output
assert "Resources released." in output
assert output.count("not implemented yet") == 4
assert "Safe sequence:" not in output
assert "Deadlocked processes:" not in output
output = run("2\nscenarios/safe.txt\n2\nscenarios/invalid.txt\n3\n0\n")
assert "Scenario rejected:" in output and "Previous state preserved." in output
assert output.count("Processes: 5") == 2
output = run("1\n2\n1\n3\n3\n1\n0\n4\n3\n1\n0\n2\n3\n0\n")
assert "Scenario loaded." in output and "Processes: 2" in output
output = run("word\n99\n0\n")
assert output.count("Enter one menu number") == 2
# Truncated scenario, extra token, negative and nonnumeric file values.
import tempfile
from pathlib import Path
for contents in ("2 1\n", "1 1 1 1 0 0 0 EXTRA", "1 1 -1 1 0 0 0", "1 1 X"):
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / "bad.txt"
        path.write_text(contents)
        assert "Scenario rejected:" in run(f"2\n{path}\n0\n")
print("CLI and malformed scenario checks passed.")
