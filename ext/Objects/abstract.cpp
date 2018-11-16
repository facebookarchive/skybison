#include "runtime.h"

namespace python {

extern "C" int PyBuffer_FillInfo(Py_buffer* /* w */, PyObject* /* j */,
                                 void* /* f */, Py_ssize_t /* n */, int /* y */,
                                 int /* s */) {
  UNIMPLEMENTED("PyBuffer_FillInfo");
}

extern "C" int PyBuffer_IsContiguous(const Py_buffer* /* w */, char /* r */) {
  UNIMPLEMENTED("PyBuffer_IsContiguous");
}

extern "C" void PyBuffer_Release(Py_buffer* /* w */) {
  UNIMPLEMENTED("PyBuffer_Release");
}

extern "C" int PyMapping_Check(PyObject* /* o */) {
  UNIMPLEMENTED("PyMapping_Check");
}

extern "C" Py_ssize_t PyMapping_Size(PyObject* /* o */) {
  UNIMPLEMENTED("PyMapping_Size");
}

extern "C" PyObject* PyNumber_Absolute(PyObject* /* o */) {
  UNIMPLEMENTED("PyNumber_Absolute");
}

extern "C" PyObject* PyNumber_Add(PyObject* /* v */, PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_Add");
}

extern "C" int PyNumber_Check(PyObject* /* o */) {
  UNIMPLEMENTED("PyNumber_Check");
}

extern "C" PyObject* PyNumber_Float(PyObject* /* o */) {
  UNIMPLEMENTED("PyNumber_Float");
}

extern "C" PyObject* PyNumber_InPlaceAdd(PyObject* /* v */, PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_InPlaceAdd");
}

extern "C" PyObject* PyNumber_InPlaceMultiply(PyObject* /* v */,
                                              PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_InPlaceMultiply");
}

extern "C" PyObject* PyNumber_Index(PyObject* /* m */) {
  UNIMPLEMENTED("PyNumber_Index");
}

extern "C" PyObject* PyNumber_Invert(PyObject* /* o */) {
  UNIMPLEMENTED("PyNumber_Invert");
}

extern "C" PyObject* PyNumber_Long(PyObject* /* o */) {
  UNIMPLEMENTED("PyNumber_Long");
}

extern "C" PyObject* PyNumber_Multiply(PyObject* /* v */, PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_Multiply");
}

extern "C" PyObject* PyNumber_Negative(PyObject* /* o */) {
  UNIMPLEMENTED("PyNumber_Negative");
}

extern "C" PyObject* PyNumber_Positive(PyObject* /* o */) {
  UNIMPLEMENTED("PyNumber_Positive");
}

extern "C" int PyObject_AsWriteBuffer(PyObject* /* j */, void** /* buffer */,
                                      Py_ssize_t* /* n */) {
  UNIMPLEMENTED("PyObject_AsWriteBuffer");
}

extern "C" int PyObject_DelItem(PyObject* /* o */, PyObject* /* y */) {
  UNIMPLEMENTED("PyObject_DelItem");
}

extern "C" int PyObject_GetBuffer(PyObject* /* j */, Py_buffer* /* w */,
                                  int /* s */) {
  UNIMPLEMENTED("PyObject_GetBuffer");
}

extern "C" PyObject* PyObject_GetItem(PyObject* /* o */, PyObject* /* y */) {
  UNIMPLEMENTED("PyObject_GetItem");
}

extern "C" int PyObject_SetItem(PyObject* /* o */, PyObject* /* y */,
                                PyObject* /* e */) {
  UNIMPLEMENTED("PyObject_SetItem");
}

extern "C" Py_ssize_t PyObject_Size(PyObject* /* o */) {
  UNIMPLEMENTED("PyObject_Size");
}

extern "C" int PySequence_Check(PyObject* /* s */) {
  UNIMPLEMENTED("PySequence_Check");
}

extern "C" PyObject* PySequence_Concat(PyObject* /* s */, PyObject* /* o */) {
  UNIMPLEMENTED("PySequence_Concat");
}

extern "C" int PySequence_Contains(PyObject* /* q */, PyObject* /* b */) {
  UNIMPLEMENTED("PySequence_Contains");
}

extern "C" PyObject* PySequence_GetItem(PyObject* /* s */, Py_ssize_t /* i */) {
  UNIMPLEMENTED("PySequence_GetItem");
}

extern "C" PyObject* PySequence_GetSlice(PyObject* /* s */, Py_ssize_t /* 1 */,
                                         Py_ssize_t /* 2 */) {
  UNIMPLEMENTED("PySequence_GetSlice");
}

extern "C" PyObject* PySequence_InPlaceConcat(PyObject* /* s */,
                                              PyObject* /* o */) {
  UNIMPLEMENTED("PySequence_InPlaceConcat");
}

extern "C" PyObject* PySequence_InPlaceRepeat(PyObject* /* o */,
                                              Py_ssize_t /* t */) {
  UNIMPLEMENTED("PySequence_InPlaceRepeat");
}

extern "C" PyObject* PySequence_Repeat(PyObject* /* o */, Py_ssize_t /* t */) {
  UNIMPLEMENTED("PySequence_Repeat");
}

extern "C" int PySequence_SetItem(PyObject* /* s */, Py_ssize_t /* i */,
                                  PyObject* /* o */) {
  UNIMPLEMENTED("PySequence_SetItem");
}

extern "C" Py_ssize_t PySequence_Size(PyObject* /* s */) {
  UNIMPLEMENTED("PySequence_Size");
}

extern "C" PyObject* PyIter_Next(PyObject* /* r */) {
  UNIMPLEMENTED("PyIter_Next");
}

extern "C" PyObject* PyMapping_GetItemString(PyObject* /* o */,
                                             const char* /* y */) {
  UNIMPLEMENTED("PyMapping_GetItemString");
}

extern "C" int PyMapping_HasKey(PyObject* /* o */, PyObject* /* y */) {
  UNIMPLEMENTED("PyMapping_HasKey");
}

extern "C" int PyMapping_HasKeyString(PyObject* /* o */, const char* /* y */) {
  UNIMPLEMENTED("PyMapping_HasKeyString");
}

extern "C" PyObject* PyMapping_Items(PyObject* /* o */) {
  UNIMPLEMENTED("PyMapping_Items");
}

extern "C" PyObject* PyMapping_Keys(PyObject* /* o */) {
  UNIMPLEMENTED("PyMapping_Keys");
}

extern "C" Py_ssize_t PyMapping_Length(PyObject* /* o */) {
  UNIMPLEMENTED("PyMapping_Length");
}

extern "C" int PyMapping_SetItemString(PyObject* /* o */, const char* /* y */,
                                       PyObject* /* e */) {
  UNIMPLEMENTED("PyMapping_SetItemString");
}

extern "C" PyObject* PyMapping_Values(PyObject* /* o */) {
  UNIMPLEMENTED("PyMapping_Values");
}

extern "C" Py_ssize_t PyNumber_AsSsize_t(PyObject* /* m */, PyObject* /* r */) {
  UNIMPLEMENTED("PyNumber_AsSsize_t");
}

extern "C" PyObject* PyNumber_FloorDivide(PyObject* /* v */,
                                          PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_FloorDivide");
}

extern "C" PyObject* PyNumber_InPlaceFloorDivide(PyObject* /* v */,
                                                 PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_InPlaceFloorDivide");
}

extern "C" PyObject* PyNumber_InPlaceMatrixMultiply(PyObject* /* v */,
                                                    PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_InPlaceMatrixMultiply");
}

extern "C" PyObject* PyNumber_InPlacePower(PyObject* /* v */, PyObject* /* w */,
                                           PyObject* /* z */) {
  UNIMPLEMENTED("PyNumber_InPlacePower");
}

extern "C" PyObject* PyNumber_InPlaceRemainder(PyObject* /* v */,
                                               PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_InPlaceRemainder");
}

extern "C" PyObject* PyNumber_InPlaceTrueDivide(PyObject* /* v */,
                                                PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_InPlaceTrueDivide");
}

extern "C" PyObject* PyNumber_MatrixMultiply(PyObject* /* v */,
                                             PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_MatrixMultiply");
}

extern "C" PyObject* PyNumber_Power(PyObject* /* v */, PyObject* /* w */,
                                    PyObject* /* z */) {
  UNIMPLEMENTED("PyNumber_Power");
}

extern "C" PyObject* PyNumber_Remainder(PyObject* /* v */, PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_Remainder");
}

extern "C" PyObject* PyNumber_ToBase(PyObject* /* n */, int /* e */) {
  UNIMPLEMENTED("PyNumber_ToBase");
}

extern "C" PyObject* PyNumber_TrueDivide(PyObject* /* v */, PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_TrueDivide");
}

extern "C" int PyObject_AsCharBuffer(PyObject* /* j */,
                                     const char** /* buffer */,
                                     Py_ssize_t* /* n */) {
  UNIMPLEMENTED("PyObject_AsCharBuffer");
}

extern "C" int PyObject_AsReadBuffer(PyObject* /* j */,
                                     const void** /* buffer */,
                                     Py_ssize_t* /* n */) {
  UNIMPLEMENTED("PyObject_AsReadBuffer");
}

extern "C" int PyObject_CheckReadBuffer(PyObject* /* j */) {
  UNIMPLEMENTED("PyObject_CheckReadBuffer");
}

extern "C" int PyObject_DelItemString(PyObject* /* o */, const char* /* y */) {
  UNIMPLEMENTED("PyObject_DelItemString");
}

extern "C" PyObject* PyObject_Format(PyObject* /* j */, PyObject* /* c */) {
  UNIMPLEMENTED("PyObject_Format");
}

extern "C" PyObject* PyObject_GetIter(PyObject* /* o */) {
  UNIMPLEMENTED("PyObject_GetIter");
}

extern "C" int PyObject_IsInstance(PyObject* /* t */, PyObject* /* s */) {
  UNIMPLEMENTED("PyObject_IsInstance");
}

extern "C" int PyObject_IsSubclass(PyObject* /* d */, PyObject* /* s */) {
  UNIMPLEMENTED("PyObject_IsSubclass");
}

extern "C" Py_ssize_t PyObject_Length(PyObject* /* o */) {
  UNIMPLEMENTED("PyObject_Length");
}

extern "C" PyObject* PyObject_Type(PyObject* /* o */) {
  UNIMPLEMENTED("PyObject_Type");
}

extern "C" Py_ssize_t PySequence_Count(PyObject* /* s */, PyObject* /* o */) {
  UNIMPLEMENTED("PySequence_Count");
}

extern "C" int PySequence_DelItem(PyObject* /* s */, Py_ssize_t /* i */) {
  UNIMPLEMENTED("PySequence_DelItem");
}

extern "C" int PySequence_DelSlice(PyObject* /* s */, Py_ssize_t /* 1 */,
                                   Py_ssize_t /* 2 */) {
  UNIMPLEMENTED("PySequence_DelSlice");
}

extern "C" PyObject* PySequence_Fast(PyObject* /* v */, const char* /* m */) {
  UNIMPLEMENTED("PySequence_Fast");
}

extern "C" int PySequence_In(PyObject* /* w */, PyObject* /* v */) {
  UNIMPLEMENTED("PySequence_In");
}

extern "C" Py_ssize_t PySequence_Index(PyObject* /* s */, PyObject* /* o */) {
  UNIMPLEMENTED("PySequence_Index");
}

extern "C" Py_ssize_t PySequence_Length(PyObject* /* s */) {
  UNIMPLEMENTED("PySequence_Length");
}

extern "C" PyObject* PySequence_List(PyObject* /* v */) {
  UNIMPLEMENTED("PySequence_List");
}

extern "C" int PySequence_SetSlice(PyObject* /* s */, Py_ssize_t /* 1 */,
                                   Py_ssize_t /* 2 */, PyObject* /* o */) {
  UNIMPLEMENTED("PySequence_SetSlice");
}

extern "C" PyObject* PySequence_Tuple(PyObject* /* v */) {
  UNIMPLEMENTED("PySequence_Tuple");
}

extern "C" PyObject* PyObject_CallObject(PyObject* /* e */, PyObject* /* s */) {
  UNIMPLEMENTED("PyObject_CallObject");
}

extern "C" PyObject* PyObject_CallFunction(PyObject* /* e */,
                                           const char* /* t */, ...) {
  UNIMPLEMENTED("PyObject_CallFunction");
}

extern "C" PyObject* PyObject_CallFunctionObjArgs(PyObject* /* e */, ...) {
  UNIMPLEMENTED("PyObject_CallFunctionObjArgs");
}

extern "C" PyObject* PyObject_CallMethod(PyObject* /* j */, const char* /* e */,
                                         const char* /* t */, ...) {
  UNIMPLEMENTED("PyObject_CallMethod");
}

extern "C" PyObject* PyObject_CallMethodObjArgs(PyObject* /* e */,
                                                PyObject* /* e */, ...) {
  UNIMPLEMENTED("PyObject_CallMethodObjArgs");
}

extern "C" PyObject* PyObject_Call(PyObject* /* e */, PyObject* /* s */,
                                   PyObject* /* s */) {
  UNIMPLEMENTED("PyObject_Call");
}

extern "C" PyObject* _PyObject_CallFunction_SizeT(PyObject* /* e */,
                                                  const char* /* t */, ...) {
  UNIMPLEMENTED("_PyObject_CallFunction_SizeT");
}

extern "C" PyObject* _PyObject_CallMethod_SizeT(PyObject* /* j */,
                                                const char* /* e */,
                                                const char* /* t */, ...) {
  UNIMPLEMENTED("_PyObject_CallMethod_SizeT");
}

}  // namespace python
