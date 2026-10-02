# Contributing

This is a prototype. Issues and pull requests are welcome. There is no release schedule and no promised response time.

## Build and test

```bash
git clone https://github.com/cyber-physical-engineering/device_trust_shim.git
cd device_trust_shim
cmake -B build -S .
cmake --build build
ctest --test-dir build --output-on-failure
```

The tests are plain C++17 with no framework. `tests/test_audit_chain.cpp` counts failures with a `CHECK` macro that stays active in every build type.

## Before you open a pull request

1. Run the tests. A change to the entry format or the hashing must come with a test that fails without it.
2. Keep the change small and say what it fixes.
3. Build with `-Wall -Wextra -Wpedantic` and keep it warning-free.
4. If the change removes a limit listed in the README, update that section.

## Ideas that fit

- A signature or an outside anchor for the chain, so a full rewrite is caught.
- A pseudonym function for the clinical-trial adapter that uses a secret salt.
- Fuzzing of the entry parser.
