#include "builtins.h"

#include <iostream>

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

std::ostream* builtinPrintStream = &std::cout;

class Arguments {
 public:
  Arguments(Frame* caller, word nargs)
      : tos_(caller->valueStackTop()), nargs_(nargs) {}
  Object* get(word n) {
    return tos_[nargs_ - 1 - n];
  }

 private:
  Object** tos_;
  word nargs_;
};

Object* builtinBuildClass(Thread* thread, Frame* caller, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread->handles());

  if (nargs < 2) {
    std::abort(); // TODO: throw a TypeError exception.
  }
  Arguments args(caller, nargs);
  if (!args.get(0)->isFunction()) {
    std::abort(); // TODO: throw a TypeError exception.
  }
  if (!args.get(1)->isString()) {
    std::abort(); // TODO: throw a TypeError exception.
  }

  Handle<Function> body(&scope, args.get(0));
  Handle<Object> name(&scope, args.get(1));

  Handle<Dictionary> dictionary(&scope, runtime->newDictionary());
  Handle<Object> key(
      &scope, thread->runtime()->newStringFromCString("__name__"));
  Handle<ValueCell> value_cell(
      &scope,
      runtime->dictionaryAtIfAbsentPut(
          dictionary, key, runtime->newValueCellCallback()));
  value_cell->setValue(*name);

  // TODO: might need to do some kind of callback here and we want backtraces to
  // work correctly.  The key to doing that would be to put some state on the
  // stack in between the the incoming arguments from the builtin' caller and
  // the on-stack state for the class body function call.
  thread->runClassFunction(*body, *dictionary, caller);

  Handle<Class> result(&scope, runtime->newClass());
  result->setName(*name);
  result->setDictionary(*dictionary);

  // TODO: actually compute the class hierarchy.  Until we do, nargs should be
  // equal to the number of classes we need in the MRO.  This MRO should also
  // be correct if all of the classes inherit directly from object.
  Handle<ObjectArray> mro(&scope, runtime->newObjectArray(nargs));
  mro->atPut(0, *result);
  for (word i = 1, j = 2; j < nargs; i++, j++) {
    mro->atPut(i, args.get(j));
  }
  mro->atPut(mro->length() - 1, runtime->classAt(ClassId::kObject));
  result->setMro(*mro);
  return *result;
}

static void printString(String* str) {
  for (int i = 0; i < str->length(); i++) {
    *builtinPrintStream << str->charAt(i);
  }
}

Object* builtinPrint(Thread*, Frame* caller, word nargs) {
  const char separator = ' ';
  const char terminator = '\n';

  Arguments args(caller, nargs);
  for (word i = 0; i < nargs; i++) {
    Object* arg = args.get(i);
    if (arg->isString()) {
      printString(String::cast(arg));
    } else if (arg->isSmallInteger()) {
      *builtinPrintStream << SmallInteger::cast(arg)->value();
    } else if (arg->isBoolean()) {
      *builtinPrintStream << (Boolean::cast(arg)->value() ? "True" : "False");
    } else {
      std::abort();
    }
    if ((i + 1) != nargs) {
      *builtinPrintStream << separator;
    }
  }
  *builtinPrintStream << terminator;

  return None::object();
}

} // namespace python
