#include "trampolines.h"

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

static inline void initFrame(Thread* thread, Function* function,
                             Frame* new_frame, Frame* caller_frame) {
  new_frame->setGlobals(function->globals());
  if (new_frame->globals() == caller_frame->globals()) {
    new_frame->setBuiltins(caller_frame->builtins());
  } else {
    // TODO: Set builtins appropriately
    // See PyFrame_New in cpython frameobject.c.
    // If caller_frame->globals() contains the __builtins__ key and the
    // associated value is a module, use its dict. Otherwise, create a new
    // dict that contains the single assoc
    // ("None", None::object())
    new_frame->setBuiltins(thread->runtime()->newDict());
  }
  new_frame->setVirtualPC(0);
  new_frame->setFastGlobals(function->fastGlobals());
}

// Final stage of a call when arguments are all in place
static inline Object* callNoChecks(Thread* thread, Function* function,
                                   Frame* caller_frame, Code* code) {
  // Set up the new frame
  auto callee_frame = thread->pushFrame(code);

  // Initialize it
  initFrame(thread, function, callee_frame, caller_frame);

  // Off we go!
  return Interpreter::execute(thread, callee_frame);
}

// Final stage of a call with possible Freevars/Cellvars
static inline Object* callCheckFreeCell(Thread* thread, Function* function,
                                        Frame* caller_frame, Code* code) {
  // Set up the new frame
  auto callee_frame = thread->pushFrame(code);

  // Initialize it
  initFrame(thread, function, callee_frame, caller_frame);

  // initialize cell var
  auto num_locals = code->nlocals();
  auto num_cellvars = code->numCellvars();
  for (word i = 0; i < code->numCellvars(); i++) {
    // TODO: implement cell2arg
    callee_frame->setLocal(num_locals + i, thread->runtime()->newValueCell());
  }

  // initialize free var
  DCHECK(code->numFreevars() == 0 ||
             code->numFreevars() ==
                 ObjectArray::cast(function->closure())->length(),
         "Number of freevars is different than the closure.");
  for (word i = 0; i < code->numFreevars(); i++) {
    callee_frame->setLocal(num_locals + num_cellvars + i,
                           ObjectArray::cast(function->closure())->at(i));
  }

  // Off we go!
  return Interpreter::execute(thread, callee_frame);
}

// The basic interpreterTrampoline biases for the common case for a
// CALL_FUNCTION - correct number of arguments passed by position with no
// cell or free vars. If not, we bail out to a more general routine.
Object* interpreterTrampoline(Thread* thread, Frame* caller_frame, word argc) {
  HandleScope scope(thread);
  Handle<Function> function(&scope, caller_frame->function(argc));
  Handle<Code> code(&scope, function->code());
  // Are we one of the less common cases?
  if ((argc != code->argcount() || !(code->flags() & Code::SIMPLE_CALL))) {
    return interpreterTrampolineSlowPath(thread, *function, *code, caller_frame,
                                         argc);
  }
  DCHECK((code->kwonlyargcount() == 0) && (code->flags() & Code::NOFREE) &&
             !(code->flags() & (Code::VARARGS | Code::VARKEYARGS)),
         "Code::SIMPLE_CALL out of sync with "
         "kwonlyargcount()/NOFREE/VARARGS/VARKEYARGS");

  return callNoChecks(thread, *function, caller_frame, *code);
}

Object* interpreterTrampolineSlowPath(Thread* thread, Function* function,
                                      Code* code, Frame* caller_frame,
                                      word argc) {
  uword flags = code->flags();
  HandleScope scope(thread);
  Handle<Object> tmp_varargs(&scope, None::object());
  if (argc < code->argcount() && function->hasDefaults()) {
    // Add default positional args
    Handle<ObjectArray> default_args(&scope, function->defaults());
    if (default_args->length() < (code->argcount() - argc)) {
      return thread->throwTypeErrorFromCStr(
          "TypeError: not enough positional arguments");
    }
    const word positional_only = code->argcount() - default_args->length();
    for (; argc < code->argcount(); argc++) {
      caller_frame->pushValue(default_args->at(argc - positional_only));
    }
  }

  if ((argc > code->argcount()) || (flags & Code::VARARGS)) {
    // VARARGS - spill extra positional args into the varargs tuple.
    if (flags & Code::VARARGS) {
      word len = Utils::maximum(static_cast<word>(0), argc - code->argcount());
      Handle<ObjectArray> varargs(&scope,
                                  thread->runtime()->newObjectArray(len));
      for (word i = (len - 1); i >= 0; i--) {
        varargs->atPut(i, caller_frame->topValue());
        caller_frame->popValue();
        argc--;
      }
      tmp_varargs = *varargs;
    } else {
      return thread->throwTypeErrorFromCStr("TypeError: too many arguments");
    }
  }

  // If there are any keyword-only args, there must be defaults for them
  // because we arrived here via CALL_FUNCTION (and thus, no keywords were
  // supplied at the call site).
  if (code->kwonlyargcount() != 0 && !function->kwDefaults()->isNone()) {
    Handle<Dict> kw_defaults(&scope, function->kwDefaults());
    if (!kw_defaults->isNone()) {
      Handle<ObjectArray> formal_names(&scope, code->varnames());
      word first_kw = code->argcount();
      for (word i = 0; i < code->kwonlyargcount(); i++) {
        Handle<Object> name(&scope, formal_names->at(first_kw + i));
        Object* val = thread->runtime()->dictAt(kw_defaults, name);
        if (!val->isError()) {
          caller_frame->pushValue(val);
          argc++;
        } else {
          return thread->throwTypeErrorFromCStr(
              "TypeError: missing keyword-only argument");
        }
      }
    } else {
      return thread->throwTypeErrorFromCStr(
          "TypeError: missing keyword-only argument");
    }
  }

  if (flags & Code::VARARGS) {
    caller_frame->pushValue(*tmp_varargs);
    argc++;
  }

  if (flags & Code::VARKEYARGS) {
    // VARKEYARGS - because we arrived via CALL_FUNCTION, no keyword arguments
    // provided.  Just add an empty dict.
    Handle<Object> kwdict(&scope, thread->runtime()->newDict());
    caller_frame->pushValue(*kwdict);
    argc++;
  }

  // At this point, we should have the correct number of arguments.  Throw if
  // not.
  if (argc != code->totalArgs()) {
    return thread->throwTypeErrorFromCStr(
        "TypeError: incorrect argument count");
  }
  return callCheckFreeCell(thread, function, caller_frame, code);
}

word findName(Object* name, ObjectArray* name_list) {
  word len = name_list->length();
  for (word i = 0; i < len; i++) {
    if (name == name_list->at(i)) {
      return i;
    }
  }
  return len;
}

// Verify correct number and order of arguments.  If order is wrong, try to
// fix it.  If argument is missing (denoted by Error::object()), try to supply
// it with a default.  This routine expects the number of args on the stack
// and number of names in the actual_names tuple to match.  Caller must pad
// prior to calling to ensure this.
// Return None::object() if successfull, error object if not.
Object* checkArgs(Function* function, Object** kw_arg_base,
                  ObjectArray* actual_names, ObjectArray* formal_names,
                  word start) {
  word num_actuals = actual_names->length();
  // Helper function to swap actual arguments and names
  auto swap = [kw_arg_base, actual_names](word arg_pos1,
                                          word arg_pos2) -> void {
    Object* tmp = *(kw_arg_base - arg_pos1);
    *(kw_arg_base - arg_pos1) = *(kw_arg_base - arg_pos2);
    *(kw_arg_base - arg_pos2) = tmp;
    tmp = actual_names->at(arg_pos1);
    actual_names->atPut(arg_pos1, actual_names->at(arg_pos2));
    actual_names->atPut(arg_pos2, tmp);
  };
  // Helper function to retrieve argument
  auto arg_at = [kw_arg_base](word idx) -> Object*& {
    return *(kw_arg_base - idx);
  };
  for (word arg_pos = 0; arg_pos < num_actuals; arg_pos++) {
    if (actual_names->at(arg_pos) == formal_names->at(arg_pos + start)) {
      // We're good here: actual & formal arg names match.  Check the next one.
      continue;
    }
    // Mismatch.  Try to fix it.  Note: args grow down.
    bool swapped = false;
    // Look for expected Formal name in Actuals tuple.
    for (word i = arg_pos + 1; i < num_actuals; i++) {
      if (actual_names->at(i) == formal_names->at(arg_pos + start)) {
        // Found it.  Swap both the stack and the actual_names tuple.
        swap(arg_pos, i);
        swapped = true;
        break;
      }
    }
    if (swapped) {
      // We managed to fix it.  Check the next one.
      continue;
    }
    // Can't find an Actual for this Formal.
    // If we have a real actual in current slot, move it somewhere safe.
    if (!arg_at(arg_pos)->isError()) {
      for (word i = arg_pos + 1; i < num_actuals; i++) {
        if (arg_at(i)->isError()) {
          // Found an uninitialized slot.  Use it to save current actual.
          swap(arg_pos, i);
          break;
        }
      }
      // If we were unable to find a slot to swap into, TypeError
      if (!arg_at(arg_pos)->isError()) {
        return Thread::currentThread()->throwTypeErrorFromCStr(
            "TypeError: invalid arguments");
      }
    }
    // Now, can we fill that slot with a default argument?
    HandleScope scope;
    Handle<Code> code(&scope, function->code());
    word absolute_pos = arg_pos + start;
    if (absolute_pos < code->argcount()) {
      word defaults_size =
          function->hasDefaults()
              ? ObjectArray::cast(function->defaults())->length()
              : 0;
      word defaults_start = code->argcount() - defaults_size;
      if (absolute_pos >= (defaults_start)) {
        // Set the default value
        Handle<ObjectArray> default_args(&scope, function->defaults());
        *(kw_arg_base - arg_pos) =
            default_args->at(absolute_pos - defaults_start);
        continue;  // Got it, move on to the next
      }
    } else if (!function->kwDefaults()->isNone()) {
      // How about a kwonly default?
      Handle<Dict> kw_defaults(&scope, function->kwDefaults());
      Thread* thread = Thread::currentThread();
      Handle<Object> name(&scope, formal_names->at(arg_pos + start));
      Object* val = thread->runtime()->dictAt(kw_defaults, name);
      if (!val->isError()) {
        *(kw_arg_base - arg_pos) = val;
        continue;  // Got it, move on to the next
      }
    }
    return Thread::currentThread()->throwTypeErrorFromCStr(
        "TypeError: missing argument");
  }
  return None::object();
}

// Trampoline for calls in which the caller provided keyword arguments.  On
// TOS will be a tuple of the provided keyword names.  Above them the associated
// values in left-to-right order.
Object* interpreterTrampolineKw(Thread* thread, Frame* caller_frame,
                                word argc) {
  HandleScope scope(thread);
  // Destructively pop the tuple of kwarg names
  Handle<ObjectArray> keywords(&scope, caller_frame->topValue());
  caller_frame->popValue();
  DCHECK(keywords->length() > 0, "Invalid keyword name tuple");
  Handle<Function> function(&scope, caller_frame->peek(argc));
  Handle<Code> code(&scope, function->code());
  word expected_args = code->argcount() + code->kwonlyargcount();
  word num_keyword_args = keywords->length();
  word num_positional_args = argc - num_keyword_args;
  Handle<ObjectArray> formal_parm_names(&scope, code->varnames());
  word flags = code->flags();
  Object* res;
  Handle<Object> tmp_varargs(&scope, None::object());
  Handle<Object> tmp_dict(&scope, None::object());

  // We expect use of keyword argument calls to be uncommon, but when used
  // we anticipate mostly use of simple forms.  General scheme here is to
  // normalize the odd forms into standard form and then handle them all
  // in the same place.
  if (flags & (Code::VARARGS | Code::VARKEYARGS)) {
    if (flags & Code::VARARGS) {
      // If we have more positional than expected, add the remainder to a tuple,
      // remove from the stack and close up the hole.
      word excess = Utils::maximum(static_cast<word>(0),
                                   num_positional_args - code->argcount());
      Handle<ObjectArray> varargs(&scope,
                                  thread->runtime()->newObjectArray(excess));
      if (excess > 0) {
        // Point to the leftmost excess argument
        Object** p =
            (caller_frame->valueStackTop() + num_keyword_args + excess) - 1;
        // Copy the excess to the * tuple
        for (word i = 0; i < excess; i++) {
          varargs->atPut(i, *(p - i));
        }
        // Fill in the hole
        for (word i = 0; i < num_keyword_args; i++) {
          *p = *(p - excess);
          p--;
        }
        // Adjust the counts
        caller_frame->dropValues(excess);
        argc -= excess;
        num_positional_args -= excess;
      }
      tmp_varargs = *varargs;
    }
    if (flags & Code::VARKEYARGS) {
      // Too many positional args passed?
      if (num_positional_args > code->argcount()) {
        return thread->throwTypeErrorFromCStr(
            "TypeError: Too many positional arguments");
      }
      // If we have keyword arguments that don't appear in the formal parameter
      // list, add them to a keyword dict.
      Handle<Dict> dict(&scope, thread->runtime()->newDict());
      Handle<List> saved_keyword_list(&scope, thread->runtime()->newList());
      Handle<List> saved_values(&scope, thread->runtime()->newList());
      word formal_parm_size = formal_parm_names->length();
      Runtime* runtime = thread->runtime();
      Object** p = caller_frame->valueStackTop() + (num_keyword_args - 1);
      for (word i = 0; i < num_keyword_args; i++) {
        Handle<Object> key(&scope, keywords->at(i));
        Handle<Object> value(&scope, *(p - i));
        if (findName(*key, *formal_parm_names) < formal_parm_size) {
          // Got a match, stash pair for future restoration on the stack
          runtime->listAdd(saved_keyword_list, key);
          runtime->listAdd(saved_values, value);
        } else {
          // New, add it and associated value to the varkeyargs dict
          runtime->dictAtPut(dict, key, value);
          argc--;
        }
      }
      // Now, restore the stashed values to the stack and build a new
      // keywords name list.
      caller_frame->dropValues(
          num_keyword_args);  // Pop all of the old keyword values
      num_keyword_args = saved_keyword_list->allocated();
      // Replace the old keywords list with a new one.
      keywords = runtime->newObjectArray(num_keyword_args);
      for (word i = 0; i < num_keyword_args; i++) {
        caller_frame->pushValue(saved_values->at(i));
        keywords->atPut(i, saved_keyword_list->at(i));
      }
      tmp_dict = *dict;
    }
  }
  // At this point, all vararg forms have been normalized
  Object** kw_arg_base = (caller_frame->valueStackTop() + num_keyword_args) -
                         1;  // pointer to first non-positional arg
  if (UNLIKELY(argc > expected_args)) {
    return thread->throwTypeErrorFromCStr("TypeError: Too many arguments");
  } else if (UNLIKELY(argc < expected_args)) {
    // Too few args passed.  Can we supply default args to make it work?
    // First, normalize & pad keywords and stack arguments
    word name_tuple_size = expected_args - num_positional_args;
    Handle<ObjectArray> padded_keywords(
        &scope, thread->runtime()->newObjectArray(name_tuple_size));
    for (word i = 0; i < num_keyword_args; i++) {
      padded_keywords->atPut(i, keywords->at(i));
    }
    // Fill in missing spots w/ Error code
    for (word i = num_keyword_args; i < name_tuple_size; i++) {
      caller_frame->pushValue(Error::object());
      padded_keywords->atPut(i, Error::object());
    }
    keywords = *padded_keywords;
  }
  // Now we've got the right number.  Do they match up?
  res = checkArgs(*function, kw_arg_base, *keywords, *formal_parm_names,
                  num_positional_args);
  // If we're a vararg form, need to push the tuple/dict.
  if (res->isNone()) {
    if (flags & Code::VARARGS) {
      caller_frame->pushValue(*tmp_varargs);
    }
    if (flags & Code::VARKEYARGS) {
      caller_frame->pushValue(*tmp_dict);
    }
    return callNoChecks(thread, *function, caller_frame, *code);
  }
  return res;  // TypeError created by checkArgs.
}

Object* interpreterTrampolineEx(Thread* thread, Frame* caller_frame, word arg) {
  HandleScope scope(thread);
  Handle<Object> kw_dict(&scope, None::object());
  if (arg & CallFunctionExFlag::VAR_KEYWORDS) {
    kw_dict = caller_frame->topValue();
    caller_frame->popValue();
  }
  Handle<ObjectArray> positional_args(&scope, caller_frame->topValue());
  caller_frame->popValue();
  for (word i = 0; i < positional_args->length(); i++) {
    caller_frame->pushValue(positional_args->at(i));
  }
  word argc = positional_args->length();
  if (arg & CallFunctionExFlag::VAR_KEYWORDS) {
    Runtime* runtime = thread->runtime();
    Handle<Dict> dict(&scope, *kw_dict);
    Handle<ObjectArray> keys(&scope, runtime->dictKeys(dict));
    for (word i = 0; i < keys->length(); i++) {
      Handle<Object> key(&scope, keys->at(i));
      caller_frame->pushValue(runtime->dictAt(dict, key));
    }
    argc += keys->length();
    caller_frame->pushValue(*keys);
    return interpreterTrampolineKw(thread, caller_frame, argc);
  } else {
    return interpreterTrampoline(thread, caller_frame, argc);
  }
}

typedef PyObject* (*PyCFunction)(PyObject*, PyObject*, PyObject*);

Object* extensionTrampoline(Thread* thread, Frame* caller_frame, word argc) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // Set the address pointer to the function pointer
  Handle<Function> function(&scope, caller_frame->function(argc));
  Handle<Int> address(&scope, function->code());

  Handle<Object> object(&scope, caller_frame->topValue());
  Handle<Object> attr_name(&scope, runtime->symbols()->ExtensionPtr());
  // TODO(eelizondo): Cache the None handle
  PyObject* none = ApiHandle::fromObject(None::object())->asPyObject();

  if (object->isType()) {
    Handle<Type> type_class(&scope, *object);
    Handle<Int> extension_type(&scope, type_class->extensionType());
    PyObject* type = static_cast<PyObject*>(extension_type->asCPointer());

    // void* to CFunction idiom
    PyCFunction new_function;
    *reinterpret_cast<void**>(&new_function) = address->asCPointer();

    PyObject* new_pyobject = (*new_function)(type, none, none);
    return ApiHandle::fromPyObject(new_pyobject)->asObject();
  }

  Handle<HeapObject> instance(&scope, *object);
  Handle<Int> object_ptr(&scope,
                         runtime->instanceAt(thread, instance, attr_name));
  PyObject* self = static_cast<PyObject*>(object_ptr->asCPointer());

  // void* to CFunction idiom
  PyCFunction init_function;
  *reinterpret_cast<void**>(&init_function) = address->asCPointer();
  (*init_function)(self, none, none);

  return *instance;
}

Object* extensionTrampolineKw(Thread*, Frame*, word) {
  UNIMPLEMENTED("ExtensionTrampolineKw");
}

Object* extensionTrampolineEx(Thread*, Frame*, word) {
  UNIMPLEMENTED("ExtensionTrampolineEx");
}

Object* unimplementedTrampoline(Thread*, Frame*, word) {
  UNIMPLEMENTED("Trampoline");
}

}  // namespace python
