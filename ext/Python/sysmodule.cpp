#include <unistd.h>

#include <climits>
#include <cstdarg>

#include "cpython-data.h"

#include "capi-handles.h"
#include "list-builtins.h"
#include "module-builtins.h"
#include "os.h"
#include "runtime.h"
#include "str-builtins.h"
#include "sys-module.h"

namespace py {

PY_EXPORT size_t _PySys_GetSizeOf(PyObject* o) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(o)->asObject());
  Object result_obj(&scope,
                    thread->invokeFunction1(ID(sys), ID(getsizeof), obj));
  if (result_obj.isError()) {
    // Pass through a pending exception if any exists.
    return static_cast<size_t>(-1);
  }
  DCHECK(thread->runtime()->isInstanceOfInt(*result_obj),
         "sys.getsizeof() should return an int");
  Int result(&scope, intUnderlying(*result_obj));
  return static_cast<size_t>(result.asWord());
}

PY_EXPORT PyObject* PySys_GetObject(const char* name) {
  DCHECK(name != nullptr, "name must not be nullptr");
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Module module(&scope, runtime->findModuleById(ID(sys)));
  Object name_obj(&scope, Runtime::internStrFromCStr(thread, name));
  Object result(&scope, moduleAt(module, name_obj));
  if (result.isErrorNotFound()) return nullptr;
  return ApiHandle::borrowedReference(thread, *result);
}

PY_EXPORT void PySys_AddWarnOption(const wchar_t* /* s */) {
  UNIMPLEMENTED("PySys_AddWarnOption");
}

PY_EXPORT void PySys_AddWarnOptionUnicode(PyObject* /* n */) {
  UNIMPLEMENTED("PySys_AddWarnOptionUnicode");
}

PY_EXPORT void PySys_AddXOption(const wchar_t* /* s */) {
  UNIMPLEMENTED("PySys_AddXOption");
}

PY_EXPORT void PySys_FormatStderr(const char* /* t */, ...) {
  UNIMPLEMENTED("PySys_FormatStderr");
}

PY_EXPORT void PySys_FormatStdout(const char* /* t */, ...) {
  UNIMPLEMENTED("PySys_FormatStdout");
}

PY_EXPORT PyObject* PySys_GetXOptions() { UNIMPLEMENTED("PySys_GetXOptions"); }

PY_EXPORT int PySys_HasWarnOptions() { UNIMPLEMENTED("PySys_HasWarnOptions"); }

PY_EXPORT void PySys_ResetWarnOptions() {
  UNIMPLEMENTED("PySys_ResetWarnOptions");
}

static void sysUpdatePath(Thread* thread, const Str& arg0) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  List path(&scope, runtime->lookupNameInModule(thread, ID(sys), ID(path)));

  char* arg0_cstr = arg0.toCStr();
  char* script_path = arg0_cstr;

  if (std::strcmp(script_path, "-c") == 0) {
    Str empty(&scope, Str::empty());
    listInsert(thread, path, empty, 0);
    std::free(arg0_cstr);
    return;
  }

  if (std::strcmp(script_path, "-m") == 0) {
    char buf[PATH_MAX];
    if (::getcwd(buf, PATH_MAX) == nullptr) {
      return;
    }
    Str cwd(&scope, runtime->newStrFromCStr(buf));
    listInsert(thread, path, cwd, 0);
    std::free(arg0_cstr);
    return;
  }

  char* buf = nullptr;
  // Follow symlink, if it exists
  char* link = OS::readLink(script_path);
  if (link != nullptr) {
    // It's a symlink
    if (link[0] == '/') {
      script_path = link;  // Link to absolute path
    } else if (std::strchr(link, '/') == nullptr) {
      ;  // Link without path
    } else {
      // Link has partial path, must join(dirname(script_path), link)
      char* last_sep = std::strrchr(script_path, '/');
      if (last_sep == nullptr) {
        script_path = link;  // script_path has no path
      } else {
        // Must make a copy
        int dir_len = last_sep - script_path + 1;
        int link_len = std::strlen(link);
        buf = static_cast<char*>(std::malloc(dir_len + link_len + 1));
        std::strncpy(buf, script_path, dir_len);
        last_sep = buf + (last_sep - script_path);
        std::strncpy(last_sep + 1, link, link_len + 1);
        script_path = buf;
      }
    }
  }

  // Resolve the real path. Allow realpath to allocate to avoid PATH_MAX issues
  char* fullpath = ::realpath(script_path, nullptr);
  if (fullpath != nullptr) {
    script_path = fullpath;
  }

  char* last_sep = std::strrchr(script_path, '/');
  word path_len = 0;
  if (last_sep != nullptr) {
    path_len = last_sep - script_path + 1;
    if (path_len > 1) {
      path_len--;  // Drop trailing separator
    }
  }

  auto script_path_data = reinterpret_cast<const byte*>(script_path);
  View<byte> script_path_view(script_path_data, path_len);
  Object path_element(&scope, runtime->newStrWithAll(script_path_view));
  listInsert(thread, path, path_element, 0);

  std::free(fullpath);
  std::free(link);
  std::free(buf);
  std::free(arg0_cstr);
}

PY_EXPORT void PySys_SetArgv(int argc, wchar_t** argv) {
  PySys_SetArgvEx(argc, argv, Py_IsolatedFlag == 0);
}

PY_EXPORT void PySys_SetArgvEx(int argc, wchar_t** argv, int updatepath) {
  CHECK(argc >= 0, "Unexpected argc");

  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  List args(&scope, runtime->newList());

  Str arg(&scope, Str::empty());
  if (argc == 0 || argv == nullptr) {
    // Ensure at least one (empty) argument is given in sys.argv
    // This will also ensure the first element of sys.path is an empty string
    runtime->listAdd(thread, args, arg);
  } else {
    for (int i = 0; i < argc; i++) {
      RawObject result = newStrFromWideChar(thread, argv[i]);
      CHECK(!result.isErrorException(), "Invalid unicode character in argv");
      arg = result;
      runtime->listAdd(thread, args, arg);
    }
  }
  Module sys_module(&scope, runtime->findModuleById(ID(sys)));
  moduleAtPutById(thread, sys_module, ID(argv), args);

  if (updatepath == 0) {
    return;
  }

  arg = args.at(0);
  sysUpdatePath(thread, arg);
}

PY_EXPORT int PySys_SetObject(const char* /* e */, PyObject* /* v */) {
  UNIMPLEMENTED("PySys_SetObject");
}

PY_EXPORT void PySys_SetPath(const wchar_t* /* h */) {
  UNIMPLEMENTED("PySys_SetPath");
}

PY_EXPORT void PySys_WriteStderr(const char* format, ...) {
  va_list va;
  va_start(va, format);
  writeStderrV(Thread::current(), format, va);
  va_end(va);
}

PY_EXPORT void PySys_WriteStdout(const char* format, ...) {
  va_list va;
  va_start(va, format);
  writeStdoutV(Thread::current(), format, va);
  va_end(va);
}

}  // namespace py
