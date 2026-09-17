from pathlib import Path

root = Path(__file__).resolve().parents[1]
tests = (root / "tests/HapticsTest.cpp").read_text(encoding="utf-8")

# HapticsTest.cpp uses std::numbers::pi_v directly. Do not rely on another
# standard-library header to include <numbers> transitively: MSVC does not.
assert "std::numbers::pi_v<float>" in tests
assert "#include <numbers>" in tests

# These types are also used directly in this translation unit and should have
# their owning headers included explicitly rather than transitively.
assert "#include <string>" in tests
assert "#include <vector>" in tests

print("PASS v0.2.73 HapticsTest has explicit MSVC-safe standard-library includes")
