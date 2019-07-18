#!/usr/bin/env python3
"""
Utility script that provides default arguments for executing a command
with various performance measurement tools.
"""
import logging
import os
import re
import subprocess
import tempfile
from abc import ABC, abstractmethod


log = logging.getLogger(__name__)


def run(cmd, **kwargs):
    env = dict(os.environ)
    env["PYTHONHASHSEED"] = "0"
    env["PYRO_ENABLE_CACHE"] = "1"
    log.info(f">>> {' '.join(cmd)}")
    return subprocess.run(cmd, encoding="UTF-8", env=env, **kwargs)


def pin_to_cpus():
    if not os.path.exists("/sys/devices/system/cpu/isolated"):
        return []
    completed_process = run(
        ["cat", "/sys/devices/system/cpu/isolated"], stdout=subprocess.PIPE
    )
    if completed_process.returncode != 0:
        return []
    isolated_cpus = completed_process.stdout.strip()
    if isolated_cpus == "":
        return []
    return ["taskset", "--cpu-list", isolated_cpus]


class PerformanceTool(ABC):
    # Read any optional command line arguments to set the internal defaults
    # Input: A dictionary with command line arguments
    def __init__(self, args):
        pass

    # The main function to execute the specified performance tool.
    # Input: run.Interpreter, run.Benchmark
    # Output: A dictionary with the values to be reported
    @abstractmethod
    def execute(self, interpreter, benchmark):
        pass

    # Specify the name of the tool along with a description
    @staticmethod
    @abstractmethod
    def add_tool():
        return ""

    # Add any optional command line arguments to tune the tool
    @staticmethod
    def add_optional_arguments(parser):
        return parser


class TimeTool(PerformanceTool):
    NAME = "time"

    def execute(self, interpreter, benchmark):
        command = pin_to_cpus()
        command.extend(
            [
                interpreter.binary,
                f"{os.path.dirname(os.path.abspath(__file__))}/_time_tool.py",
                benchmark.filepath(),
            ]
        )
        completed_process = run(command, stdout=subprocess.PIPE)
        if completed_process.returncode != 0:
            log.error(f"Couldn't run: {completed_process.args}")
            return {}
        time_output = completed_process.stdout.strip()
        events = [event.split(" , ") for event in time_output.split("\n")]
        result = {event[0]: event[1] for event in events}
        if "time_sec" in result:
            result["time_sec"] = float(result["time_sec"])
        if "time_sec_mean" in result:
            result["time_sec_mean"] = float(result["time_sec_mean"])
            result["time_sec_stdev"] = float(result["time_sec_stdev"])
        return result

    @staticmethod
    def add_tool():
        return f"""
'{TimeTool.NAME}': Use the 'time' command to measure execution time
"""


class PerfStat(PerformanceTool):
    NAME = "perfstat"

    def __init__(self, args):
        pass

    def execute(self, interpreter, benchmark):
        command = pin_to_cpus()
        command += ["perf", "stat"]
        command += ["--field-separator", ";"]
        command += ["--repeat", "5"]
        command += ["--event", "task-clock"]
        command += ["--event", "cycles"]
        command += ["--event", "instructions"]
        command += ["--event", "branches"]
        command += ["--event", "branch-misses"]
        command += [interpreter.binary, benchmark.filepath()]
        completed_process = run(command, stderr=subprocess.PIPE)
        if completed_process.returncode != 0:
            log.error(f"Couldn't run: {completed_process.args}")
            return {}
        perfstat_output = completed_process.stderr.strip()
        if ";" not in perfstat_output:
            log.error(f"perf stat returned an error: {perfstat_output}")
            return {}
        events = [e.split(";") for e in perfstat_output.split("\n") if ";" in e]
        return {event[2]: event[0] for event in events}

    @staticmethod
    def add_tool():
        return f"""
'{PerfStat.NAME}': Use `perf stat` to measure the execution time of
a benchmark. This repeats the run 10 times to find a significant result
"""


class Callgrind(PerformanceTool):
    NAME = "callgrind"

    def __init__(self, args):
        pass

    def execute(self, interpreter, benchmark):
        with tempfile.NamedTemporaryFile(prefix="callgrind_") as temp_file:
            command = [
                "valgrind",
                "--quiet",
                "--tool=callgrind",
                f"--callgrind-out-file={temp_file.name}",
                interpreter.binary,
                benchmark.filepath(),
            ]
            completed_process = run(command)
            if completed_process.returncode != 0:
                log.error(f"Couldn't run: {completed_process.args}")
                return {}

            with open(temp_file.name) as fd:
                r = re.compile(r"summary:\s*(.*)")
                for line in fd:
                    m = r.match(line)
                    if m:
                        instructions = int(m.group(1))
            return {"cg_instructions": instructions}

    @classmethod
    def add_tool(cls):
        return f"""
'{cls.NAME}': Measure executed instructions with `valgrind`/`callgrind`.
"""


class Size(PerformanceTool):
    NAME = "size"

    def __init__(self, args):
        pass

    def execute(self, interpreter, benchmark):
        command = ["size", "--format=sysv", interpreter.binary]
        completed_process = run(command, stdout=subprocess.PIPE)
        if completed_process.returncode != 0:
            log.error(f"Couldn't run: {completed_process.args}")
            return {}
        size_output = completed_process.stdout.strip()
        size = 0
        r = re.compile(r"([a-zA-Z0-9_.]+)\s+([0-9]+)\s+[0-9a-fA-F]+$")
        for line in size_output.splitlines():
            m = r.match(line)
            if not m:
                continue
            section_name = m.group(1)
            section_size = m.group(2)
            if section_name == ".text" or section_name == "__text":
                size += int(section_size)
        if size == 0:
            log.error(f"Could not determine text segment size of {interpreter.binary}")
            return {}
        return {"size_text": size}

    @classmethod
    def add_tool(cls):
        return f"""
'{cls.NAME}': Use `size` to measure the size of the interpreters text segment.
"""


def add_tools_arguments(parser):
    measure_tools_help = "The measurement tool to use. Available Tools: \n"
    for tool in TOOLS:
        measure_tools_help += tool.add_tool()

    available_tools = [tool.NAME for tool in TOOLS]
    parser.add_argument(
        "--tool",
        "-t",
        metavar="TOOL",
        dest="tools",
        type=str,
        action="append",
        default=[],
        choices=available_tools,
        help=measure_tools_help,
    )

    for tool in TOOLS:
        parser = tool.add_optional_arguments(parser)

    return parser


# Use this to register any new tools
TOOLS = [TimeTool, PerfStat, Callgrind, Size]
