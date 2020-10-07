"""Load benchmark module file so bytecode is compiled and cached."""
import runpy
import sys


def main():
    runpy.run_path(sys.argv[1])


if __name__ == "__main__":
    main()
