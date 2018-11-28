#include "trampolines.h"

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

static inline void initFrame(Thread* thread, RawFunction function,
                             Frame* new_frame, Frame* caller_frame) {
  new_frame->setGlobals(function->globals());
  if (new_frame->globals() == caller_frame->globals()) {
    new_frame->setBuiltins(caller_frame->builtins());
  } else {
    // TODO(T36407403): Set builtins appropriately
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

// If the given Code object represents any kind of resumable function, create
// and return the corresponding object. Otherwise, return None.
static inline RawObject checkCreateGenerator(RawCode raw_code, Thread* thread) {
  word flags = raw_code->flags();
  if ((flags & (Code::GENERATOR | Code::COROUTINE)) == 0) {
    return NoneType::object();
  }

  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Code code(&scope, raw_code);
  HeapFrame heap_frame(&scope, runtime->newHeapFrame(code));
  GeneratorBase gen_base(&scope, (flags & Code::GENERATOR)
                                     ? runtime->newGenerator()
                                     : runtime->newCoroutine());
  gen_base->setHeapFrame(*heap_frame);
  runtime->genSave(thread, gen_base);
  return *gen_base;
}

// Final stage of a call when arguments are all in place
static inline RawObject callNoChecks(Thread* thread, RawFunction function,
                                     Frame* caller_frame, RawCode code) {
  // Set up the new frame
  auto callee_frame = thread->pushFrame(code);

  // Initialize it
  initFrame(thread, function, callee_frame, caller_frame);

  RawObject generator = checkCreateGenerator(code, thread);
  if (!generator->isNoneType()) {
    return generator;
  }

  // Off we go!
  return Interpreter::execute(thread, callee_frame);
}

// Final stage of a call with possible Freevars/Cellvars
static inline RawObject callCheckFreeCell(Thread* thread, RawFunction function,
                                          Frame* caller_frame, RawCode code) {
  // Set up the new frame
  auto callee_frame = thread->pushFrame(code);

  // Initialize it
  initFrame(thread, function, callee_frame, caller_frame);

  // initialize cell var
  auto num_locals = code->nlocals();
  auto num_cellvars = code->numCellvars();
  for (word i = 0; i < code->numCellvars(); i++) {
    // TODO(T36407558): implement cell2arg
    callee_frame->setLocal(num_locals + i, thread->runtime()->newValueCell());
  }

  // initialize free var
  DCHECK(
      code->numFreevars() == 0 ||
          code->numFreevars() == RawTuple::cast(function->closure())->length(),
      "Number of freevars is different than the closure.");
  for (word i = 0; i < code->numFreevars(); i++) {
    callee_frame->setLocal(num_locals + num_cellvars + i,
                           RawTuple::cast(function->closure())->at(i));
  }

  RawObject generator = checkCreateGenerator(code, thread);
  if (!generator->isNoneType()) {
    return generator;
  }

  // Off we go!
  return Interpreter::execute(thread, callee_frame);
}

// The basic interpreterTrampoline biases for the common case for a
// CALL_FUNCTION - correct number of arguments passed by position with no
// cell or free vars. If not, we bail out to a more general routine.
RawObject interpreterTrampoline(Thread* thread, Frame* caller_frame,
                                word argc) {
  HandleScope scope(thread);
  Function function(&scope, caller_frame->function(argc));
  Code code(&scope, function->code());
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

RawObject interpreterTrampolineSlowPath(Thread* thread, RawFunction function,
                                        RawCode code, Frame* caller_frame,
                                        word argc) {
  uword flags = code->flags();
  HandleScope scope(thread);
  Object tmp_varargs(&scope, NoneType::object());
  if (argc < code->argcount() && function->hasDefaults()) {
    // Add default positional args
    Tuple default_args(&scope, function->defaults());
    if (default_args->length() < (code->argcount() - argc)) {
      return thread->raiseTypeErrorWithCStr(
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
      Tuple varargs(&scope, thread->runtime()->newTuple(len));
      for (word i = (len - 1); i >= 0; i--) {
        varargs->atPut(i, caller_frame->topValue());
        caller_frame->popValue();
        argc--;
      }
      tmp_varargs = *varargs;
    } else {
      return thread->raiseTypeErrorWithCStr("TypeError: too many arguments");
    }
  }

  // If there are any keyword-only args, there must be defaults for them
  // because we arrived here via CALL_FUNCTION (and thus, no keywords were
  // supplied at the call site).
  if (code->kwonlyargcount() != 0 && !function->kwDefaults()->isNoneType()) {
    Dict kw_defaults(&scope, function->kwDefaults());
    if (!kw_defaults->isNoneType()) {
      Tuple formal_names(&scope, code->varnames());
      word first_kw = code->argcount();
      for (word i = 0; i < code->kwonlyargcount(); i++) {
        Object name(&scope, formal_names->at(first_kw + i));
        RawObject val = thread->runtime()->dictAt(kw_defaults, name);
        if (!val->isError()) {
          caller_frame->pushValue(val);
          argc++;
        } else {
          return thread->raiseTypeErrorWithCStr(
              "TypeError: missing keyword-only argument");
        }
      }
    } else {
      return thread->raiseTypeErrorWithCStr(
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
    Object kwdict(&scope, thread->runtime()->newDict());
    caller_frame->pushValue(*kwdict);
    argc++;
  }

  // At this point, we should have the correct number of arguments.  Throw if
  // not.
  if (argc != code->totalArgs()) {
    return thread->raiseTypeErrorWithCStr(
        "TypeError: incorrect argument count");
  }
  return callCheckFreeCell(thread, function, caller_frame, code);
}

word findName(RawObject name, RawTuple name_list) {
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
RawObject checkArgs(RawFunction function, RawObject* kw_arg_base,
                    RawTuple actual_names, RawTuple formal_names, word start) {
  word num_actuals = actual_names->length();
  // Helper function to swap actual arguments and names
  auto swap = [kw_arg_base, actual_names](word arg_pos1,
                                          word arg_pos2) -> void {
    RawObject tmp = *(kw_arg_base - arg_pos1);
    *(kw_arg_base - arg_pos1) = *(kw_arg_base - arg_pos2);
    *(kw_arg_base - arg_pos2) = tmp;
    tmp = actual_names->at(arg_pos1);
    actual_names->atPut(arg_pos1, actual_names->at(arg_pos2));
    actual_names->atPut(arg_pos2, tmp);
  };
  // Helper function to retrieve argument
  auto arg_at = [kw_arg_base](word idx) -> RawObject& {
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
        return Thread::currentThread()->raiseTypeErrorWithCStr(
            "TypeError: invalid arguments");
      }
    }
    // Now, can we fill that slot with a default argument?
    HandleScope scope;
    Code code(&scope, function->code());
    word absolute_pos = arg_pos + start;
    if (absolute_pos < code->argcount()) {
      word defaults_size = function->hasDefaults()
                               ? RawTuple::cast(function->defaults())->length()
                               : 0;
      word defaults_start = code->argcount() - defaults_size;
      if (absolute_pos >= (defaults_start)) {
        // Set the default value
        Tuple default_args(&scope, function->defaults());
        *(kw_arg_base - arg_pos) =
            default_args->at(absolute_pos - defaults_start);
        continue;  // Got it, move on to the next
      }
    } else if (!function->kwDefaults()->isNoneType()) {
      // How about a kwonly default?
      Dict kw_defaults(&scope, function->kwDefaults());
      Thread* thread = Thread::currentThread();
      Object name(&scope, formal_names->at(arg_pos + start));
      RawObject val = thread->runtime()->dictAt(kw_defaults, name);
      if (!val->isError()) {
        *(kw_arg_base - arg_pos) = val;
        continue;  // Got it, move on to the next
      }
    }
    return Thread::currentThread()->raiseTypeErrorWithCStr(
        "TypeError: missing argument");
  }
  return NoneType::object();
}

// Trampoline for calls in which the caller provided keyword arguments.  On
// TOS will be a tuple of the provided keyword names.  Above them the associated
// values in left-to-right order.
RawObject interpreterTrampolineKw(Thread* thread, Frame* caller_frame,
                                  word argc) {
  HandleScope scope(thread);
  // Destructively pop the tuple of kwarg names
  Tuple keywords(&scope, caller_frame->topValue());
  caller_frame->popValue();
  DCHECK(keywords->length() > 0, "Invalid keyword name tuple");
  Function function(&scope, caller_frame->peek(argc));
  Code code(&scope, function->code());
  word expected_args = code->argcount() + code->kwonlyargcount();
  word num_keyword_args = keywords->length();
  word num_positional_args = argc - num_keyword_args;
  Tuple formal_parm_names(&scope, code->varnames());
  word flags = code->flags();
  Object tmp_varargs(&scope, NoneType::object());
  Object tmp_dict(&scope, NoneType::object());

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
      Tuple varargs(&scope, thread->runtime()->newTuple(excess));
      if (excess > 0) {
        // Point to the leftmost excess argument
        RawObject* p =
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
        return thread->raiseTypeErrorWithCStr(
            "TypeError: Too many positional arguments");
      }
      // If we have keyword arguments that don't appear in the formal parameter
      // list, add them to a keyword dict.
      Dict dict(&scope, thread->runtime()->newDict());
      List saved_keyword_list(&scope, thread->runtime()->newList());
      List saved_values(&scope, thread->runtime()->newList());
      word formal_parm_size = formal_parm_names->length();
      Runtime* runtime = thread->runtime();
      RawObject* p = caller_frame->valueStackTop() + (num_keyword_args - 1);
      for (word i = 0; i < num_keyword_args; i++) {
        Object key(&scope, keywords->at(i));
        Object value(&scope, *(p - i));
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
      num_keyword_args = saved_keyword_list->numItems();
      // Replace the old keywords list with a new one.
      keywords = runtime->newTuple(num_keyword_args);
      for (word i = 0; i < num_keyword_args; i++) {
        caller_frame->pushValue(saved_values->at(i));
        keywords->atPut(i, saved_keyword_list->at(i));
      }
      tmp_dict = *dict;
    }
  }
  // At this point, all vararg forms have been normalized
  RawObject* kw_arg_base = (caller_frame->valueStackTop() + num_keyword_args) -
                           1;  // pointer to first non-positional arg
  if (UNLIKELY(argc > expected_args)) {
    return thread->raiseTypeErrorWithCStr("TypeError: Too many arguments");
  }
  if (UNLIKELY(argc < expected_args)) {
    // Too few args passed.  Can we supply default args to make it work?
    // First, normalize & pad keywords and stack arguments
    word name_tuple_size = expected_args - num_positional_args;
    Tuple padded_keywords(&scope, thread->runtime()->newTuple(name_tuple_size));
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
  RawObject res = checkArgs(*function, kw_arg_base, *keywords,
                            *formal_parm_names, num_positional_args);
  // If we're a vararg form, need to push the tuple/dict.
  if (res->isNoneType()) {
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

RawObject interpreterTrampolineEx(Thread* thread, Frame* caller_frame,
                                  word arg) {
  HandleScope scope(thread);
  Object kw_dict(&scope, NoneType::object());
  if (arg & CallFunctionExFlag::VAR_KEYWORDS) {
    kw_dict = caller_frame->topValue();
    caller_frame->popValue();
  }
  Tuple positional_args(&scope, caller_frame->topValue());
  caller_frame->popValue();
  for (word i = 0; i < positional_args->length(); i++) {
    caller_frame->pushValue(positional_args->at(i));
  }
  word argc = positional_args->length();
  if (arg & CallFunctionExFlag::VAR_KEYWORDS) {
    Runtime* runtime = thread->runtime();
    Dict dict(&scope, *kw_dict);
    Tuple keys(&scope, runtime->dictKeys(dict));
    for (word i = 0; i < keys->length(); i++) {
      Object key(&scope, keys->at(i));
      caller_frame->pushValue(runtime->dictAt(dict, key));
    }
    argc += keys->length();
    caller_frame->pushValue(*keys);
    return interpreterTrampolineKw(thread, caller_frame, argc);
  }
  return interpreterTrampoline(thread, caller_frame, argc);
}

typedef PyObject* (*PyCFunction)(PyObject*, PyObject*, PyObject*);

RawObject extensionTrampoline(Thread* thread, Frame* caller_frame, word argc) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // Set the address pointer to the function pointer
  Function function(&scope, caller_frame->function(argc));
  Int address(&scope, function->code());

  Object object(&scope, caller_frame->topValue());
  Object attr_name(&scope, runtime->symbols()->ExtensionPtr());
  // TODO(eelizondo): Cache the None handle
  PyObject* none = ApiHandle::newReference(thread, NoneType::object());

  if (object->isType()) {
    Type type_class(&scope, *object);
    Int extension_type(&scope, type_class->extensionType());
    PyObject* type = static_cast<PyObject*>(extension_type->asCPtr());

    PyCFunction new_function = bit_cast<PyCFunction>(address->asCPtr());

    PyObject* new_pyobject = (*new_function)(type, none, none);
    return ApiHandle::fromPyObject(new_pyobject)->asObject();
  }

  HeapObject instance(&scope, *object);
  Int object_ptr(&scope, runtime->instanceAt(thread, instance, attr_name));
  PyObject* self = static_cast<PyObject*>(object_ptr->asCPtr());

  PyCFunction init_function = bit_cast<PyCFunction>(address->asCPtr());
  (*init_function)(self, none, none);

  return *instance;
}

RawObject extensionTrampolineKw(Thread*, Frame*, word) {
  UNIMPLEMENTED("ExtensionTrampolineKw");
}

RawObject extensionTrampolineEx(Thread*, Frame*, word) {
  UNIMPLEMENTED("ExtensionTrampolineEx");
}

RawObject unimplementedTrampoline(Thread*, Frame*, word) {
  UNIMPLEMENTED("Trampoline");
}

}  // namespace python
