#!/usr/bin/env python3
"""
Utility script that provides default arguments for executing a command
with various performance measurement tools.
"""
import logging
import os
import subprocess
from abc import ABC, abstractmethod


log = logging.getLogger(__name__)


def run(cmd, **kwargs):
    env = dict(os.environ)
    env["PYTHONHASHSEED"] = "0"
    env["PYRO_ENABLE_CACHE"] = "1"
    log.info(f">>> {' '.join(cmd)}")
    return subprocess.run(cmd, encoding="UTF-8", env=env, **kwargs)


def pin_to_cpus():
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
    @abstractmethod
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
            result["time_sec"] = format(round(float(result["time_sec"]), 6), "0.6f")
        if "time_sec_mean" in result:
            result["time_sec_mean"] = format(
                round(float(result["time_sec_mean"]), 6), "0.6f"
            )
            result["time_sec_stdev"] = format(
                round(float(result["time_sec_stdev"]), 6), "0.6f"
            )
        return result

    @staticmethod
    def add_tool():
        return f"""
'{TimeTool.NAME}': Use the 'time' command to measure execution time
"""

    @staticmethod
    def add_optional_arguments(parser):
        return parser


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
TOOLS = [TimeTool]
