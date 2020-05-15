#ifndef STRUCTMEMBER_H
#define STRUCTMEMBER_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "cpython-types.h"

typedef struct PyMemberDef {
  const char* name;
  int type;
  Py_ssize_t offset;
  int flags;
  const char* doc;
} PyMemberDef;

/* C struct member types */
#define T_SHORT 0
#define T_INT 1
#define T_LONG 2
#define T_FLOAT 3
#define T_DOUBLE 4
#define T_STRING 5
#define T_OBJECT 6
#define T_CHAR 7
#define T_BYTE 8
#define T_UBYTE 9
#define T_USHORT 10
#define T_UINT 11
#define T_ULONG 12
#define T_STRING_INPLACE 13
#define T_BOOL 14
#define T_OBJECT_EX 16
#define T_LONGLONG 17
#define T_ULONGLONG 18
#define T_PYSSIZET 19
#define T_NONE 20

/* C struct member flags */
#define READONLY 1
#define READ_RESTRICTED 2
#define PY_WRITE_RESTRICTED 4
#define RESTRICTED (READ_RESTRICTED | PY_WRITE_RESTRICTED)

#ifdef __cplusplus
}
#endif
#endif
