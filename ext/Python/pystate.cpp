#include "runtime.h"

namespace python {

struct PyInterpreterState;
struct PyThreadState;
typedef enum { PyGILState_LOCKED, PyGILState_UNLOCKED } PyGILState_STATE;

extern "C" PyGILState_STATE PyGILState_Ensure(void) {
  UNIMPLEMENTED("PyGILState_Ensure");
}

extern "C" PyThreadState* PyGILState_GetThisThreadState(void) {
  UNIMPLEMENTED("PyGILState_GetThisThreadState");
}

extern "C" void PyGILState_Release(PyGILState_STATE /* e */) {
  UNIMPLEMENTED("PyGILState_Release");
}

extern "C" void PyInterpreterState_Clear(PyInterpreterState* /* p */) {
  UNIMPLEMENTED("PyInterpreterState_Clear");
}

extern "C" void PyInterpreterState_Delete(PyInterpreterState* /* p */) {
  UNIMPLEMENTED("PyInterpreterState_Delete");
}

extern "C" int PyState_AddModule(PyObject* /* e */,
                                 struct PyModuleDef* /* f */) {
  UNIMPLEMENTED("PyState_AddModule");
}

extern "C" PyObject* PyState_FindModule(struct PyModuleDef* /* e */) {
  UNIMPLEMENTED("PyState_FindModule");
}

extern "C" int PyState_RemoveModule(struct PyModuleDef* /* f */) {
  UNIMPLEMENTED("PyState_RemoveModule");
}

extern "C" void PyThreadState_Clear(PyThreadState* /* e */) {
  UNIMPLEMENTED("PyThreadState_Clear");
}

extern "C" void PyThreadState_Delete(PyThreadState* /* e */) {
  UNIMPLEMENTED("PyThreadState_Delete");
}

extern "C" void PyThreadState_DeleteCurrent() {
  UNIMPLEMENTED("PyThreadState_DeleteCurrent");
}

extern "C" PyThreadState* PyThreadState_Get(void) {
  UNIMPLEMENTED("PyThreadState_Get");
}

extern "C" PyObject* PyThreadState_GetDict(void) {
  UNIMPLEMENTED("PyThreadState_GetDict");
}

extern "C" PyThreadState* PyThreadState_New(PyInterpreterState* /* p */) {
  UNIMPLEMENTED("PyThreadState_New");
}

extern "C" int PyThreadState_SetAsyncExc(unsigned long /* d */,
                                         PyObject* /* c */) {
  UNIMPLEMENTED("PyThreadState_SetAsyncExc");
}

extern "C" PyThreadState* PyThreadState_Swap(PyThreadState* /* s */) {
  UNIMPLEMENTED("PyThreadState_Swap");
}

extern "C" int _PyState_AddModule(PyObject* /* e */,
                                  struct PyModuleDef* /* f */) {
  UNIMPLEMENTED("_PyState_AddModule");
}

extern "C" void _PyThreadState_Init(PyThreadState* /* e */) {
  UNIMPLEMENTED("_PyThreadState_Init");
}

extern "C" PyThreadState* _PyThreadState_Prealloc(PyInterpreterState* /* p */) {
  UNIMPLEMENTED("_PyThreadState_Prealloc");
}

extern "C" PyInterpreterState* PyInterpreterState_Head(void) {
  UNIMPLEMENTED("PyInterpreterState_Head");
}

extern "C" PyInterpreterState* PyInterpreterState_Next(
    PyInterpreterState* /* p */) {
  UNIMPLEMENTED("PyInterpreterState_Next");
}

extern "C" PyThreadState* PyInterpreterState_ThreadHead(
    PyInterpreterState* /* p */) {
  UNIMPLEMENTED("PyInterpreterState_ThreadHead");
}

}  // namespace python
