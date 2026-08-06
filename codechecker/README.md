# CodeChecker for use in Zephyr builds

CodeChecker is a static analysis infrastructure.
It executes analysis tools available on the build system,
such as Clang-Tidy, Clang Static Analyzer and Cppcheck.
Refer to the analyzer’s websites for installation instructions.

## Installing CodeChecker

CodeChecker itself is a python package available on pypi.

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install codechecker
```

## Building with CodeChecker
To run CodeChecker, west build should be called with a -DZEPHYR_SCA_VARIANT=codechecker parameter, e.g.

Note: paths in `CODECHECKER_ANALYZE_OPTS` are resolved from the build directory
(`build/`), not from the workspace root. Use `../` for relative paths, or use
absolute paths.

```bash
west build -b nucleo_f429zi -p always bacnet/zephyr/samples/profiles/b-ss -- -DZEPHYR_SCA_VARIANT=codechecker -DCODECHECKER_EXPORT=html -DCODECHECKER_ANALYZE_OPTS="--config;../bacnet/codechecker/.codechecker.yml;--ignore;../bacnet/codechecker/skip.txt"
```

Absolute path example:

```bash
west build -b nucleo_f429zi -p always bacnet/zephyr/samples/profiles/b-ss -- -DZEPHYR_SCA_VARIANT=codechecker -DCODECHECKER_EXPORT=html -DCODECHECKER_ANALYZE_OPTS="--config;$PWD/bacnet/codechecker/.codechecker.yml;--ignore;$PWD/bacnet/codechecker/skip.txt"
```
