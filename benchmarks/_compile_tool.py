"""Load benchmark module file so bytecode is compiled and cached."""
import py_compile
import runpy
import sys


def main():
    main_file = sys.argv[1]
    runpy.run_path(main_file)
    # The tool that runs _compile_tool uses the stdout of this process to
    # determine where the result .pyc is located.
    print(py_compile.compile(main_file))


if __name__ == "__main__":
    main()
