#include "list-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinListNew(Thread* thread, Frame* caller, word nargs) {
  if (nargs < 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(caller, nargs);
  if (!args.get(0)->isClass()) {
    return thread->throwTypeErrorFromCString("not a type object");
  }
  HandleScope scope(thread->handles());
  Handle<Class> type(&scope, args.get(0));
  Handle<Layout> layout(&scope, type->instanceLayout());
  if (layout->id() == LayoutId::kList) {
    return thread->runtime()->newList();
  }
  CHECK(layout->hasDelegateSlot(), "must have a delegate slot");
  Handle<Object> result(&scope, thread->runtime()->newInstance(layout));
  Handle<Object> delegate(&scope, thread->runtime()->newList());
  thread->runtime()->setInstanceDelegate(result, delegate);
  return *result;
}

static Object* listOrDelegate(Thread* thread, const Handle<Object>& instance) {
  if (instance->isList()) {
    return *instance;
  } else {
    HandleScope scope(thread);
    Handle<Class> klass(&scope, thread->runtime()->classOf(*instance));
    if (klass->hasFlag(Class::Flag::kListSubclass)) {
      return thread->runtime()->instanceDelegate(instance);
    }
  }
  return Error::object();
}

Object* builtinListAppend(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString(
        "append() takes exactly one argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
  if (list_or_error->isError()) {
    return thread->throwTypeErrorFromCString(
        "append() only support list or its subclasses");
  }
  Handle<List> list(&scope, *list_or_error);
  Handle<Object> value(&scope, args.get(1));
  thread->runtime()->listAdd(list, value);
  return None::object();
}

Object* builtinListExtend(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString(
        "extend() takes exactly one argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
  if (list_or_error->isError()) {
    return thread->throwTypeErrorFromCString(
        "extend() only support list or its subclasses");
  }
  Handle<List> list(&scope, *list_or_error);
  Handle<Object> value(&scope, args.get(1));
  // TODO(jeethu): Throw TypeError if value is not iterable.
  thread->runtime()->listExtend(list, value);
  return None::object();
}

Object* builtinListLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("__len__() takes no arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
  if (list_or_error->isError()) {
    return thread->throwTypeErrorFromCString(
        "__len__() only support list or its subclasses");
  }
  Handle<List> list(&scope, *list_or_error);
  return SmallInteger::fromWord(list->allocated());
}

Object* builtinListInsert(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->throwTypeErrorFromCString(
        "insert() takes exactly two arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(1)->isInteger()) {
    return thread->throwTypeErrorFromCString(
        "index object cannot be interpreted as an integer");
  }

  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
  if (list_or_error->isError()) {
    return thread->throwTypeErrorFromCString(
        "descriptor 'insert' requires a 'list' object");
  }
  Handle<List> list(&scope, *list_or_error);
  word index = SmallInteger::cast(args.get(1))->value();
  Handle<Object> value(&scope, args.get(2));
  thread->runtime()->listInsert(list, value, index);
  return None::object();
}

Object* builtinListPop(Thread* thread, Frame* frame, word nargs) {
  if (nargs > 2) {
    return thread->throwTypeErrorFromCString("pop() takes at most 1 argument");
  }
  Arguments args(frame, nargs);
  if (nargs == 2 && !args.get(1)->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "index object cannot be interpreted as an integer");
  }

  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
  if (list_or_error->isError()) {
    return thread->throwTypeErrorFromCString(
        "descriptor 'pop' requires a 'list' object");
  }
  Handle<List> list(&scope, *list_or_error);
  word index = list->allocated() - 1;
  if (nargs == 2) {
    word last_index = index;
    index = SmallInteger::cast(args.get(1))->value();
    index = index < 0 ? last_index + index : index;
    // Pop out of bounds
    if (index > last_index) {
      // TODO(T27365047): Throw an IndexError exception
      UNIMPLEMENTED("Throw an IndexError for an out of range list index.");
    }
  }
  // Pop empty, or negative out of bounds
  if (index < 0) {
    // TODO(T27365047): Throw an IndexError exception
    UNIMPLEMENTED("Throw an IndexError for an out of range list index.");
  }

  return thread->runtime()->listPop(list, index);
}

Object* builtinListRemove(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString(
        "remove() takes exactly one argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> value(&scope, args.get(1));
  Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
  if (list_or_error->isError()) {
    return thread->throwTypeErrorFromCString(
        "descriptor 'remove' requires a 'list' object");
  }
  Handle<List> list(&scope, *list_or_error);
  for (word i = 0; i < list->allocated(); i++) {
    Handle<Object> item(&scope, list->at(i));
    if (Boolean::cast(Interpreter::compareOperation(
                          thread,
                          frame,
                          frame->valueStackTop(),
                          CompareOp::EQ,
                          item,
                          value))
            ->value()) {
      thread->runtime()->listPop(list, i);
      return None::object();
    }
  }
  return thread->throwValueErrorFromCString("list.remove(x) x not in list");
}

} // namespace python
