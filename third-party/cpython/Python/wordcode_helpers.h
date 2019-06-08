/* This file contains code shared by the compiler and the peephole
   optimizer.
 */

#ifdef WORDS_BIGENDIAN
#  define PACKOPARG(opcode, oparg) ((_Py_CODEUNIT)(((opcode) << 8) | (oparg)))
#else
#  define PACKOPARG(opcode, oparg) ((_Py_CODEUNIT)(((oparg) << 8) | (opcode)))
#endif

/* Minimum number of code units necessary to encode instruction with
   EXTENDED_ARGs */
static int
instrsize(unsigned int oparg)
{
    return oparg <= 0xff ? 1 :
        oparg <= 0xffff ? 2 :
        oparg <= 0xffffff ? 3 :
        4;
}
