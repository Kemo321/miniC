#!/usr/bin/env python3
"""End-to-end runner for miniC: compile -> nasm -> gcc -> compare stdout."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PROJECT_ROOT = ROOT.parent


def find_compiler(explicit: str | None) -> Path:
    if explicit:
        path = Path(explicit).resolve()
        if not path.is_file():
            raise FileNotFoundError(f"Compiler not found: {path}")
        return path

    candidates = [
        PROJECT_ROOT / "build" / "src" / "minic",
        PROJECT_ROOT / "build" / "src" / "Release" / "minic.exe",
        PROJECT_ROOT / "build" / "src" / "Debug" / "minic.exe",
        PROJECT_ROOT / "build-tests" / "src" / "Release" / "minic.exe",
        PROJECT_ROOT / "build-tests" / "src" / "Debug" / "minic.exe",
        PROJECT_ROOT / "build" / "minic",
    ]
    for cand in candidates:
        if cand.is_file():
            return cand
    raise FileNotFoundError(
        "Could not find 'minic' binary. Build the project or pass --compiler PATH."
    )


def normalize_output(text: str) -> str:
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    # Trim a single trailing newline for stable comparison, then re-add one
    lines = [line.rstrip() for line in text.split("\n")]
    while lines and lines[-1] == "":
        lines.pop()
    return "\n".join(lines) + ("\n" if lines else "")


def run_cmd(cmd: list[str], cwd: Path, env: dict | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        cwd=cwd,
        env=env,
        text=True,
        capture_output=True,
        check=False,
    )


def run_one_local(
    compiler: Path,
    source: Path,
    expected: Path,
    work: Path,
) -> tuple[bool, str]:
    name = source.stem
    asm_path = work / f"{name}.asm"
    obj_path = work / f"{name}.o"
    bin_path = work / name

    # miniC always writes cwd/output.asm
    compile_proc = run_cmd([str(compiler), str(source)], cwd=work)
    if compile_proc.returncode != 0:
        return False, f"compile failed:\n{compile_proc.stderr or compile_proc.stdout}"

    produced = work / "output.asm"
    if not produced.is_file():
        return False, "compiler did not produce output.asm"
    produced.replace(asm_path)

    nasm = shutil.which("nasm")
    gcc = shutil.which("gcc")
    if not nasm or not gcc:
        return False, "nasm and/or gcc not found on PATH (use --docker on non-Linux hosts)"

    nasm_proc = run_cmd([nasm, "-f", "elf64", str(asm_path), "-o", str(obj_path)], cwd=work)
    if nasm_proc.returncode != 0:
        return False, f"nasm failed:\n{nasm_proc.stderr or nasm_proc.stdout}"

    link_proc = run_cmd([gcc, "-no-pie", str(obj_path), "-o", str(bin_path)], cwd=work)
    if link_proc.returncode != 0:
        return False, f"gcc link failed:\n{link_proc.stderr or link_proc.stdout}"

    run_proc = run_cmd([str(bin_path)], cwd=work)
    if run_proc.returncode != 0:
        return (
            False,
            f"program exited with {run_proc.returncode}\n"
            f"stdout:\n{run_proc.stdout}\nstderr:\n{run_proc.stderr}",
        )

    actual = normalize_output(run_proc.stdout)
    expect = normalize_output(expected.read_text(encoding="utf-8"))
    if actual != expect:
        return (
            False,
            f"stdout mismatch\n--- expected ---\n{expect}--- actual ---\n{actual}",
        )
    return True, "ok"


def run_docker(image: str, filter_substr: str) -> int:
    docker = shutil.which("docker")
    if not docker:
        print("ERROR: docker not found on PATH")
        return 2

    build_proc = run_cmd(
        [docker, "build", "-t", image, "-f", str(ROOT / "Dockerfile"), str(PROJECT_ROOT)],
        cwd=PROJECT_ROOT,
    )
    if build_proc.returncode != 0:
        print(build_proc.stdout)
        print(build_proc.stderr)
        print("ERROR: docker build failed")
        return 2

    cmd = [
        docker,
        "run",
        "--rm",
        "-v",
        f"{PROJECT_ROOT}:/src:ro",
        "-v",
        f"{ROOT}:/e2e:ro",
        image,
        "python3",
        "/e2e/run_e2e.py",
        "--inside-docker",
    ]
    if filter_substr:
        cmd.extend(["--filter", filter_substr])
    proc = subprocess.run(cmd)
    return proc.returncode


def build_minic_inside_docker_workdir() -> Path:
    """Used when script runs inside the E2E Docker image."""
    build_dir = Path("/tmp/minic-build")
    build_dir.mkdir(parents=True, exist_ok=True)
    configure = run_cmd(
        [
            "cmake",
            "-S",
            "/src",
            "-B",
            str(build_dir),
            "-DPRODUCTION=ON",
            "-DBUILD_TESTS=OFF",
            "-DBUILD_DOCS=OFF",
        ],
        cwd=Path("/tmp"),
    )
    if configure.returncode != 0:
        raise RuntimeError(configure.stderr or configure.stdout)
    build = run_cmd(["cmake", "--build", str(build_dir), "-j"], cwd=Path("/tmp"))
    if build.returncode != 0:
        raise RuntimeError(build.stderr or build.stdout)
    binary = build_dir / "src" / "minic"
    if not binary.is_file():
        raise FileNotFoundError("minic binary missing after Docker build")
    return binary


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", help="Path to minic executable")
    parser.add_argument(
        "--docker",
        action="store_true",
        help="Build/run E2E inside Docker (recommended on Windows/macOS)",
    )
    parser.add_argument(
        "--inside-docker",
        action="store_true",
        help=argparse.SUPPRESS,
    )
    parser.add_argument("--image", default="minic-e2e", help="Docker image name")
    parser.add_argument(
        "--filter",
        default="",
        help="Only run tests whose file name contains this substring",
    )
    args = parser.parse_args()

    if args.docker and not args.inside_docker:
        return run_docker(args.image, args.filter)

    if args.inside_docker:
        compiler = build_minic_inside_docker_workdir()
        test_root = Path("/e2e")
    else:
        compiler = find_compiler(args.compiler)
        test_root = ROOT

    tests: list[tuple[Path, Path]] = []
    for src in sorted(test_root.glob("*.mc")):
        expected = src.with_suffix(".expected")
        if not expected.is_file():
            print(f"WARN: missing expected file for {src.name}, skipping")
            continue
        if args.filter and args.filter not in src.name:
            continue
        tests.append((src, expected))

    if not tests:
        print("No E2E tests found")
        return 1

    print(f"Compiler: {compiler}")
    print(f"Running {len(tests)} E2E test(s)\n")

    passed = 0
    failed = 0
    with tempfile.TemporaryDirectory(prefix="minic-e2e-") as tmp:
        work_root = Path(tmp)
        for source, expected in tests:
            work = work_root / source.stem
            work.mkdir()
            # Copy source into workdir so relative paths are simple
            local_src = work / source.name
            shutil.copy2(source, local_src)
            ok, msg = run_one_local(compiler, local_src, expected, work)
            status = "PASS" if ok else "FAIL"
            print(f"[{status}] {source.name}")
            if not ok:
                print(msg)
                failed += 1
            else:
                passed += 1

    print(f"\nSummary: {passed} passed, {failed} failed, {passed + failed} total")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
