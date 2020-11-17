"""Load benchmark module file so bytecode is compiled and cached."""
import os
import py_compile
import runpy
import sys


def pyc_file(filename):
    """Put the interpreter-tagged .pyc next to the .py so that __file__
    directives in benchmarks work."""
    basename = os.path.basename(filename)
    stem, _ = os.path.splitext(basename)
    containing_dir = os.path.dirname(filename)
    return os.path.join(containing_dir, f"{stem}.{sys.implementation.cache_tag}.pyc")


def main():
    main_file = sys.argv[1]
    runpy.run_path(main_file)
    pyc = pyc_file(main_file)
    try:
        # There's no good cache invalidation mechanism that takes into account
        # both the interpreter running and the benchmark running. Remove the
        # cached bytecode just in case.
        os.remove(pyc)
    except FileNotFoundError:
        pass
    # The tool that runs _compile_tool uses the stdout of this process to
    # determine where the result .pyc is located.
    print(py_compile.compile(main_file, pyc))


if __name__ == "__main__":
    main()
