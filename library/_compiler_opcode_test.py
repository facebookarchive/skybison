try:
    # These are PyRo-specific libraries
    import _bytecode_utils
    import _compiler_opcode
except ImportError:
    pass

import opcode
import unittest

from test_support import pyro_only


@pyro_only
class OpcodeTests(unittest.TestCase):
    def test_all_compiler_opcodes_exist_in_runtime(self):
        compiler_opmap = _compiler_opcode.opcode.opmap
        builtin_opmap = _bytecode_utils.opmap
        for opname, opnum in compiler_opmap.items():
            self.assertIn(opname, builtin_opmap, f"{opname} not found in builtin opmap")
            self.assertEqual(
                builtin_opmap[opname],
                opnum,
                f"expected {opname} to have value {opnum} but found {builtin_opmap[opname]}",
            )

    def test_all_opcode_opcodes_exist_in_runtime(self):
        opcode_opmap = opcode.opmap
        builtin_opmap = _bytecode_utils.opmap
        for opname, opnum in opcode_opmap.items():
            self.assertIn(opname, builtin_opmap, f"{opname} not found in builtin opmap")
            self.assertEqual(
                builtin_opmap[opname],
                opnum,
                f"expected {opname} to have value {opnum} but found {builtin_opmap[opname]}",
            )


if __name__ == "__main__":
    unittest.main()
