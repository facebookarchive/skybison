#include "runtime.h"

namespace python {

typedef enum { PyGILState_LOCKED, PyGILState_UNLOCKED } PyGILState_STATE;

PY_EXPORT int PyGILState_Check() { UNIMPLEMENTED("PyGILState_Check"); }

PY_EXPORT PyGILState_STATE PyGILState_Ensure() {
  UNIMPLEMENTED("PyGILState_Ensure");
}

PY_EXPORT PyThreadState* PyGILState_GetThisThreadState() {
  UNIMPLEMENTED("PyGILState_GetThisThreadState");
}

PY_EXPORT void PyGILState_Release(PyGILState_STATE /* e */) {
  UNIMPLEMENTED("PyGILState_Release");
}

PY_EXPORT void PyInterpreterState_Clear(PyInterpreterState* /* p */) {
  UNIMPLEMENTED("PyInterpreterState_Clear");
}

PY_EXPORT void PyInterpreterState_Delete(PyInterpreterState* /* p */) {
  UNIMPLEMENTED("PyInterpreterState_Delete");
}

PY_EXPORT int PyState_AddModule(PyObject* /* e */,
                                struct PyModuleDef* /* f */) {
  UNIMPLEMENTED("PyState_AddModule");
}

PY_EXPORT PyObject* PyState_FindModule(struct PyModuleDef* /* e */) {
  UNIMPLEMENTED("PyState_FindModule");
}

PY_EXPORT int PyState_RemoveModule(struct PyModuleDef* /* f */) {
  UNIMPLEMENTED("PyState_RemoveModule");
}

PY_EXPORT void PyThreadState_Clear(PyThreadState* /* e */) {
  UNIMPLEMENTED("PyThreadState_Clear");
}

PY_EXPORT void PyThreadState_Delete(PyThreadState* /* e */) {
  UNIMPLEMENTED("PyThreadState_Delete");
}

PY_EXPORT void PyThreadState_DeleteCurrent() {
  UNIMPLEMENTED("PyThreadState_DeleteCurrent");
}

PY_EXPORT PyThreadState* PyThreadState_Get() {
  UNIMPLEMENTED("PyThreadState_Get");
}

PY_EXPORT PyObject* PyThreadState_GetDict() {
  UNIMPLEMENTED("PyThreadState_GetDict");
}

PY_EXPORT PyThreadState* PyThreadState_New(PyInterpreterState* /* p */) {
  UNIMPLEMENTED("PyThreadState_New");
}

PY_EXPORT int PyThreadState_SetAsyncExc(long /* d */, PyObject* /* c */) {
  UNIMPLEMENTED("PyThreadState_SetAsyncExc");
}

PY_EXPORT PyThreadState* PyThreadState_Swap(PyThreadState* /* s */) {
  UNIMPLEMENTED("PyThreadState_Swap");
}

PY_EXPORT int _PyState_AddModule(PyObject* /* e */,
                                 struct PyModuleDef* /* f */) {
  UNIMPLEMENTED("_PyState_AddModule");
}

PY_EXPORT void _PyThreadState_Init(PyThreadState* /* e */) {
  UNIMPLEMENTED("_PyThreadState_Init");
}

PY_EXPORT PyThreadState* _PyThreadState_Prealloc(PyInterpreterState* /* p */) {
  UNIMPLEMENTED("_PyThreadState_Prealloc");
}

PY_EXPORT PyInterpreterState* PyInterpreterState_Head() {
  UNIMPLEMENTED("PyInterpreterState_Head");
}

PY_EXPORT PyInterpreterState* PyInterpreterState_Next(
    PyInterpreterState* /* p */) {
  UNIMPLEMENTED("PyInterpreterState_Next");
}

PY_EXPORT PyThreadState* PyInterpreterState_ThreadHead(
    PyInterpreterState* /* p */) {
  UNIMPLEMENTED("PyInterpreterState_ThreadHead");
}

PY_EXPORT void _PyState_ClearModules() {
  UNIMPLEMENTED("_PyState_ClearModules");
}

}  // namespace python
