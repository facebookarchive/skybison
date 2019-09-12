// dictobject.c implementation

#include "capi-handles.h"
#include "cpython-func.h"
#include "dict-builtins.h"
#include "handles.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"

namespace python {

PY_EXPORT int PyDict_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isDict();
}

PY_EXPORT int PyDict_Check_Func(PyObject* obj) {
  return Thread::current()->runtime()->isInstanceOfDict(
      ApiHandle::fromPyObject(obj)->asObject());
}

PY_EXPORT int _PyDict_SetItem_KnownHash(PyObject* pydict, PyObject* key,
                                        PyObject* value, Py_hash_t hash) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  Dict dict(&scope, *dict_obj);
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object value_obj(&scope, ApiHandle::fromPyObject(value)->asObject());
  SmallInt hash_obj(&scope, SmallInt::fromWordTruncated(hash));
  runtime->dictAtPutWithHash(thread, dict, key_obj, value_obj, hash_obj);
  return 0;
}

PY_EXPORT int PyDict_SetItem(PyObject* pydict, PyObject* key, PyObject* value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dictobj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  if (!thread->runtime()->isInstanceOfDict(*dictobj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  Object keyobj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object valueobj(&scope, ApiHandle::fromPyObject(value)->asObject());
  Object result(&scope, thread->invokeFunction3(SymbolId::kBuiltins,
                                                SymbolId::kUnderDictSetItem,
                                                dictobj, keyobj, valueobj));
  return result.isError() ? -1 : 0;
}

PY_EXPORT int PyDict_SetItemString(PyObject* pydict, const char* key,
                                   PyObject* value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dictobj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Object keyobj(&scope, thread->runtime()->newStrFromCStr(key));
  Object valueobj(&scope, ApiHandle::fromPyObject(value)->asObject());
  Object result(&scope, thread->invokeFunction3(SymbolId::kBuiltins,
                                                SymbolId::kUnderDictSetItem,
                                                dictobj, keyobj, valueobj));
  return result.isError() ? -1 : 0;
}

PY_EXPORT PyObject* PyDict_New() {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object value(&scope, runtime->newDict());
  return ApiHandle::newReference(thread, *value);
}

static PyObject* getItem(Thread* thread, const Object& dict,
                         const Object& key) {
  HandleScope scope(thread);
  Object result(
      &scope, thread->invokeFunction2(SymbolId::kBuiltins,
                                      SymbolId::kUnderDictGetItem, dict, key));
  // For historical reasons, PyDict_GetItem supresses all errors that may occur
  if (result.isError()) {
    thread->clearPendingException();
    return nullptr;
  }
  if (result.isUnbound()) {
    return nullptr;
  }
  return ApiHandle::borrowedReference(thread, *result);
}

PY_EXPORT PyObject* _PyDict_GetItem_KnownHash(PyObject* pydict, PyObject* key,
                                              Py_hash_t hash) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dictobj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dictobj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Dict dict(&scope, *dictobj);
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  SmallInt hash_obj(&scope, SmallInt::fromWordTruncated(hash));
  Object value(&scope,
               runtime->dictAtWithHash(thread, dict, key_obj, hash_obj));
  if (value.isError()) return nullptr;
  return ApiHandle::borrowedReference(thread, *value);
}

PY_EXPORT PyObject* PyDict_GetItem(PyObject* pydict, PyObject* key) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  return getItem(thread, dict, key_obj);
}

PY_EXPORT PyObject* PyDict_GetItemString(PyObject* pydict, const char* key) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Object key_obj(&scope, thread->runtime()->newStrFromCStr(key));
  return getItem(thread, dict, key_obj);
}

PY_EXPORT void PyDict_Clear(PyObject* pydict) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    return;
  }
  Dict dict(&scope, *dict_obj);
  dict.setNumItems(0);
  dict.setData(runtime->emptyTuple());
}

PY_EXPORT int PyDict_ClearFreeList() { return 0; }

PY_EXPORT int PyDict_Contains(PyObject* pydict, PyObject* key) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Dict dict(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  // TODO(T36757907): Return -1 when dictIncludes fails to hash the key
  return runtime->dictIncludes(thread, dict, key_obj);
}

PY_EXPORT PyObject* PyDict_Copy(PyObject* pydict) {
  Thread* thread = Thread::current();
  if (pydict == nullptr) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  if (!thread->runtime()->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Dict dict(&scope, *dict_obj);
  return ApiHandle::newReference(thread, dictCopy(thread, dict));
}

PY_EXPORT int PyDict_DelItem(PyObject* pydict, PyObject* key) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  Dict dict(&scope, *dict_obj);
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object key_hash(&scope, Interpreter::hash(thread, key_obj));
  if (key_hash.isErrorException()) return -1;
  if (runtime->dictRemoveWithHash(thread, dict, key_obj, key_hash).isError()) {
    thread->raise(LayoutId::kKeyError, *key_obj);
    return -1;
  }
  return 0;
}

PY_EXPORT int PyDict_DelItemString(PyObject* pydict, const char* key) {
  PyObject* str = PyUnicode_FromString(key);
  if (str == nullptr) return -1;
  int result = PyDict_DelItem(pydict, str);
  Py_DECREF(str);
  return result;
}

PY_EXPORT PyObject* PyDict_GetItemWithError(PyObject* pydict, PyObject* key) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object key_hash(&scope, Interpreter::hash(thread, key_obj));
  if (key_hash.isErrorException()) return nullptr;
  Dict dict(&scope, *dict_obj);
  Object value(&scope,
               runtime->dictAtWithHash(thread, dict, key_obj, key_hash));
  if (value.isError()) {
    return nullptr;
  }
  return ApiHandle::borrowedReference(thread, *value);
}

PY_EXPORT PyObject* PyDict_Items(PyObject* pydict) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Dict dict(&scope, *dict_obj);
  return ApiHandle::newReference(thread, runtime->dictItems(thread, dict));
}

PY_EXPORT PyObject* PyDict_Keys(PyObject* pydict) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Dict dict(&scope, *dict_obj);
  return ApiHandle::newReference(thread, runtime->dictKeys(thread, dict));
}

PY_EXPORT int PyDict_Merge(PyObject* left, PyObject* right,
                           int override_matching) {
  CHECK_BOUND(override_matching, 2);
  Thread* thread = Thread::current();
  if (left == nullptr || right == nullptr) {
    thread->raiseBadInternalCall();
    return -1;
  }
  HandleScope scope(thread);
  Object left_obj(&scope, ApiHandle::fromPyObject(left)->asObject());
  if (!thread->runtime()->isInstanceOfDict(*left_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }
  Dict left_dict(&scope, *left_obj);
  Object right_obj(&scope, ApiHandle::fromPyObject(right)->asObject());
  auto merge_func = override_matching ? dictMergeOverride : dictMergeIgnore;
  if ((*merge_func)(thread, left_dict, right_obj).isError()) {
    return -1;
  }
  return 0;
}

PY_EXPORT int PyDict_MergeFromSeq2(PyObject* /* d */, PyObject* /* 2 */,
                                   int /* e */) {
  UNIMPLEMENTED("PyDict_MergeFromSeq2");
}

PY_EXPORT int PyDict_Next(PyObject* pydict, Py_ssize_t* ppos, PyObject** pkey,
                          PyObject** pvalue) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  if (!thread->runtime()->isInstanceOfDict(*dict_obj)) {
    return 0;
  }
  Dict dict(&scope, *dict_obj);
  Tuple dict_data(&scope, dict.data());
  // Below are all the possible statuses of ppos and what to do in each case.
  // * If an index is out of bounds, we should not advance.
  // * If an index does not point to a valid bucket, we should try and find the
  //   next bucket, or fail.
  // * Read the contents of that bucket.
  // * Advance the index.
  if (!Dict::Bucket::currentOrNextItem(*dict_data, ppos)) {
    return 0;
  }
  // At this point, we will always have a valid bucket index.
  if (pkey != nullptr) {
    *pkey = ApiHandle::borrowedReference(thread,
                                         Dict::Bucket::key(*dict_data, *ppos));
  }
  if (pvalue != nullptr) {
    *pvalue = ApiHandle::borrowedReference(
        thread, Dict::Bucket::value(*dict_data, *ppos));
  }
  *ppos += Dict::Bucket::kNumPointers;
  return true;
}

PY_EXPORT Py_ssize_t PyDict_Size(PyObject* p) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object dict_obj(&scope, ApiHandle::fromPyObject(p)->asObject());
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return -1;
  }

  Dict dict(&scope, *dict_obj);
  return dict.numItems();
}

PY_EXPORT int PyDict_Update(PyObject* left, PyObject* right) {
  return PyDict_Merge(left, right, 1);
}

PY_EXPORT PyObject* PyDict_Values(PyObject* pydict) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object dict_obj(&scope, ApiHandle::fromPyObject(pydict)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*dict_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Dict dict(&scope, *dict_obj);
  return ApiHandle::newReference(thread, runtime->dictValues(thread, dict));
}

PY_EXPORT PyObject* PyObject_GenericGetDict(PyObject* /* j */, void* /* t */) {
  UNIMPLEMENTED("PyObject_GenericGetDict");
}

}  // namespace python
