#include "trampolines.h"

#include "capi-handles.h"
#include "dict-builtins.h"
#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"
#include "tuple-builtins.h"

namespace py {

// Populate the free variable and cell variable arguments.
void processFreevarsAndCellvars(Thread* thread, Frame* frame) {
  // initialize cell variables
  HandleScope scope(thread);
  Function function(&scope, frame->function());
  DCHECK(function.hasFreevarsOrCellvars(),
         "no free variables or cell variables");
  Code code(&scope, function.code());
  Runtime* runtime = thread->runtime();
  word num_locals = code.nlocals();
  word num_cellvars = code.numCellvars();
  for (word i = 0; i < code.numCellvars(); i++) {
    Cell cell(&scope, runtime->newCell());

    // Allocate a cell for a local variable if cell2arg is not preset
    if (code.cell2arg().isNoneType()) {
      frame->setLocal(num_locals + i, *cell);
      continue;
    }

    // Allocate a cell for a local variable if cell2arg is present but
    // the cell does not match any argument
    Object arg_index(&scope, Tuple::cast(code.cell2arg()).at(i));
    if (arg_index.isNoneType()) {
      frame->setLocal(num_locals + i, *cell);
      continue;
    }

    // Allocate a cell for an argument
    word local_idx = Int::cast(*arg_index).asWord();
    cell.setValue(frame->local(local_idx));
    frame->setLocal(local_idx, NoneType::object());
    frame->setLocal(num_locals + i, *cell);
  }

  // initialize free variables
  DCHECK(code.numFreevars() == 0 ||
             code.numFreevars() == Tuple::cast(function.closure()).length(),
         "Number of freevars is different than the closure.");
  for (word i = 0; i < code.numFreevars(); i++) {
    frame->setLocal(num_locals + num_cellvars + i,
                    Tuple::cast(function.closure()).at(i));
  }
}

RawObject raiseMissingArgumentsError(Thread* thread, word nargs,
                                     RawFunction function) {
  HandleScope scope(thread);
  Function function_obj(&scope, function);
  Object defaults(&scope, function_obj.defaults());
  word n_defaults = defaults.isNoneType() ? 0 : Tuple::cast(*defaults).length();
  return thread->raiseWithFmt(
      LayoutId::kTypeError,
      "'%F' takes min %w positional arguments but %w given", &function_obj,
      function_obj.argcount() - n_defaults, nargs);
}

RawObject processDefaultArguments(Thread* thread, word nargs,
                                  RawFunction function_raw) {
  word argcount = function_raw.argcount();
  word n_missing_args = argcount - nargs;
  if (n_missing_args > 0) {
    RawObject result =
        addDefaultArguments(thread, nargs, function_raw, n_missing_args);
    if (result.isErrorException()) return result;
    function_raw = Function::cast(result);
    nargs += n_missing_args;
    if (function_raw.hasSimpleCall()) {
      DCHECK(function_raw.totalArgs() == nargs, "argument count mismatch");
      return function_raw;
    }
  }

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function function(&scope, function_raw);
  Object varargs_param(&scope, runtime->emptyTuple());
  if (n_missing_args < 0) {
    // We have too many arguments.
    if (!function.hasVarargs()) {
      thread->stackDrop(nargs + 1);
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "'%F' takes max %w positional arguments but %w given", &function,
          argcount, nargs);
    }
    // Put extra positional args into the varargs tuple.
    word len = -n_missing_args;
    Tuple tuple(&scope, runtime->newTuple(len));
    for (word i = (len - 1); i >= 0; i--) {
      tuple.atPut(i, thread->stackPop());
    }
    nargs -= len;
    varargs_param = *tuple;
  }

  // If there are any keyword-only args, there must be defaults for them
  // because we arrived here via CALL_FUNCTION (and thus, no keywords were
  // supplied at the call site).
  Code code(&scope, function.code());
  word kwonlyargcount = code.kwonlyargcount();
  if (kwonlyargcount > 0) {
    if (function.kwDefaults().isNoneType()) {
      thread->stackDrop(nargs + 1);
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "missing keyword-only argument");
    }
    Dict kw_defaults(&scope, function.kwDefaults());
    if (!kw_defaults.isNoneType()) {
      Tuple formal_names(&scope, code.varnames());
      word first_kw = argcount;
      Object name(&scope, NoneType::object());
      for (word i = 0; i < kwonlyargcount; i++) {
        name = formal_names.at(first_kw + i);
        RawObject value = dictAtByStr(thread, kw_defaults, name);
        if (value.isErrorNotFound()) {
          thread->stackDrop(nargs + i + 1);
          return thread->raiseWithFmt(LayoutId::kTypeError,
                                      "missing keyword-only argument");
        }
        thread->stackPush(value);
      }
      nargs += kwonlyargcount;
    }
  }

  if (function.hasVarargs()) {
    thread->stackPush(*varargs_param);
    nargs++;
  }
  if (function.hasVarkeyargs()) {
    // VARKEYARGS - because we arrived via CALL_FUNCTION, no keyword arguments
    // provided.  Just add an empty dict.
    thread->stackPush(runtime->newDict());
    nargs++;
  }
  DCHECK(function.totalArgs() == nargs, "argument count mismatch");
  return *function;
}

// Verify correct number and order of arguments.  If order is wrong, try to
// fix it.  If argument is missing (denoted by Error::object()), try to supply
// it with a default.  This routine expects the number of args on the stack
// and number of names in the actual_names tuple to match.  Caller must pad
// prior to calling to ensure this.
// Return None::object() if successful, error object if not.
static RawObject checkArgs(Thread* thread, const Function& function,
                           RawObject* kw_arg_base, const Tuple& actual_names,
                           const Tuple& formal_names, word start) {
  word posonlyargcount = RawCode::cast(function.code()).posonlyargcount();
  word num_actuals = actual_names.length();
  // Helper function to swap actual arguments and names
  auto swap = [&kw_arg_base](RawMutableTuple ordered_names, word arg_pos1,
                             word arg_pos2) -> void {
    RawObject tmp = *(kw_arg_base - arg_pos1);
    *(kw_arg_base - arg_pos1) = *(kw_arg_base - arg_pos2);
    *(kw_arg_base - arg_pos2) = tmp;
    tmp = ordered_names.at(arg_pos1);
    ordered_names.atPut(arg_pos1, ordered_names.at(arg_pos2));
    ordered_names.atPut(arg_pos2, tmp);
  };
  // Helper function to retrieve argument
  auto arg_at = [&kw_arg_base](word idx) -> RawObject& {
    return *(kw_arg_base - idx);
  };
  HandleScope scope(thread);
  // In case the order of the parameters in the call does not match the
  // declaration order, create a copy of `actual_names` to adjust it to match
  // `formal_names`.
  Tuple ordered_names(&scope, *actual_names);
  Object formal_name(&scope, NoneType::object());
  for (word arg_pos = 0; arg_pos < num_actuals; arg_pos++) {
    word formal_pos = arg_pos + start;
    formal_name = formal_names.at(formal_pos);
    RawObject result =
        Runtime::objectEquals(thread, ordered_names.at(arg_pos), *formal_name);
    if (result.isErrorException()) return result;
    if (result == Bool::trueObj()) {
      if (formal_pos >= posonlyargcount) {
        // We're good here: actual & formal arg names match.  Check the next
        // one.
        continue;
      }
      // A matching keyword arg but for a positional-only parameter.
      return Thread::current()->raiseWithFmt(
          LayoutId::kTypeError,
          "keyword argument specified for positional-only argument '%S'",
          &formal_name);
    }
    // Mismatch.  Try to fix it.  Note: args grow down.
    // TODO(T66307914): Avoid heap allocation here.
    // In case `actual_names` needs to be adjusted, create a copy to avoid
    // modifying `actual_names`.
    if (ordered_names == actual_names) {
      word actual_names_length = actual_names.length();
      ordered_names = thread->runtime()->newMutableTuple(actual_names_length);
      for (word i = 0; i < actual_names_length; ++i) {
        ordered_names.atPut(i, actual_names.at(i));
      }
    }
    DCHECK(ordered_names.isMutableTuple(), "MutableTuple is expected");
    bool swapped = false;
    // Look for expected Formal name in Actuals tuple.
    for (word i = arg_pos + 1; i < num_actuals; i++) {
      result = Runtime::objectEquals(thread, ordered_names.at(i), *formal_name);
      if (result.isErrorException()) return result;
      if (result == Bool::trueObj()) {
        // Found it.  Swap both the stack and the ordered_names tuple.
        swap(MutableTuple::cast(*ordered_names), arg_pos, i);
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
    if (!arg_at(arg_pos).isError()) {
      for (word i = arg_pos + 1; i < num_actuals; i++) {
        if (arg_at(i).isError()) {
          // Found an uninitialized slot.  Use it to save current actual.
          swap(MutableTuple::cast(*ordered_names), arg_pos, i);
          break;
        }
      }
      // If we were unable to find a slot to swap into, TypeError
      if (!arg_at(arg_pos).isError()) {
        Object param_name(&scope, swapped ? formal_names.at(arg_pos)
                                          : ordered_names.at(arg_pos));
        return thread->raiseWithFmt(
            LayoutId::kTypeError,
            "%F() got an unexpected keyword argument '%S'", &function,
            &param_name);
      }
    }
    // Now, can we fill that slot with a default argument?
    word absolute_pos = arg_pos + start;
    word argcount = function.argcount();
    if (absolute_pos < argcount) {
      word defaults_size = function.hasDefaults()
                               ? Tuple::cast(function.defaults()).length()
                               : 0;
      word defaults_start = argcount - defaults_size;
      if (absolute_pos >= (defaults_start)) {
        // Set the default value
        Tuple default_args(&scope, function.defaults());
        *(kw_arg_base - arg_pos) =
            default_args.at(absolute_pos - defaults_start);
        continue;  // Got it, move on to the next
      }
    } else if (!function.kwDefaults().isNoneType()) {
      // How about a kwonly default?
      Dict kw_defaults(&scope, function.kwDefaults());
      Str name(&scope, formal_names.at(arg_pos + start));
      RawObject val = dictAtByStr(thread, kw_defaults, name);
      if (!val.isErrorNotFound()) {
        *(kw_arg_base - arg_pos) = val;
        continue;  // Got it, move on to the next
      }
    }
    return thread->raiseWithFmt(LayoutId::kTypeError, "missing argument");
  }
  return NoneType::object();
}

static word findName(Thread* thread, word posonlyargcount, const Object& name,
                     const Tuple& names) {
  word len = names.length();
  for (word i = posonlyargcount; i < len; i++) {
    RawObject result = Runtime::objectEquals(thread, *name, names.at(i));
    if (result.isErrorException()) return -1;
    if (result == Bool::trueObj()) {
      return i;
    }
  }
  return len;
}

// Converts the outgoing arguments of a keyword call into positional arguments
// and processes default arguments, rearranging everything into a form expected
// by the callee.
RawObject prepareKeywordCall(Thread* thread, word nargs,
                             RawFunction function_raw) {
  HandleScope scope(thread);
  Function function(&scope, function_raw);
  // Pop the tuple of kwarg names
  Tuple keywords(&scope, thread->stackPop());
  Code code(&scope, function.code());
  word expected_args = function.argcount() + code.kwonlyargcount();
  word num_keyword_args = keywords.length();
  word num_positional_args = nargs - num_keyword_args;
  Tuple varnames(&scope, code.varnames());
  Object tmp_varargs(&scope, NoneType::object());
  Object tmp_dict(&scope, NoneType::object());

  // We expect use of keyword argument calls to be uncommon, but when used
  // we anticipate mostly use of simple forms.  General scheme here is to
  // normalize the odd forms into standard form and then handle them all
  // in the same place.
  if (function.hasVarargsOrVarkeyargs()) {
    Runtime* runtime = thread->runtime();
    if (function.hasVarargs()) {
      // If we have more positional than expected, add the remainder to a tuple,
      // remove from the stack and close up the hole.
      word excess =
          Utils::maximum<word>(0, num_positional_args - function.argcount());
      Tuple varargs(&scope, runtime->newTuple(excess));
      if (excess > 0) {
        // Point to the leftmost excess argument
        RawObject* p = (thread->stackPointer() + num_keyword_args + excess) - 1;
        // Copy the excess to the * tuple
        for (word i = 0; i < excess; i++) {
          varargs.atPut(i, *(p - i));
        }
        // Fill in the hole
        for (word i = 0; i < num_keyword_args; i++) {
          *p = *(p - excess);
          p--;
        }
        // Adjust the counts
        thread->stackDrop(excess);
        nargs -= excess;
        num_positional_args -= excess;
      }
      tmp_varargs = *varargs;
    }
    if (function.hasVarkeyargs()) {
      // Too many positional args passed?
      if (num_positional_args > function.argcount()) {
        thread->stackDrop(nargs + 1);
        return thread->raiseWithFmt(LayoutId::kTypeError,
                                    "Too many positional arguments");
      }
      // If we have keyword arguments that don't appear in the formal parameter
      // list, add them to a keyword dict.
      Dict dict(&scope, runtime->newDict());
      List saved_keyword_list(&scope, runtime->newList());
      List saved_values(&scope, runtime->newList());
      DCHECK(varnames.length() >= expected_args,
             "varnames must be greater than or equal to positional args");
      RawObject* p = thread->stackPointer() + (num_keyword_args - 1);
      word posonlyargcount = code.posonlyargcount();
      for (word i = 0; i < num_keyword_args; i++) {
        Object key(&scope, keywords.at(i));
        Object value(&scope, *(p - i));
        word result = findName(thread, posonlyargcount, key, varnames);
        if (result < 0) {
          thread->stackDrop(nargs + 1);
          return Error::exception();
        }
        if (result < expected_args) {
          // Got a match, stash pair for future restoration on the stack
          runtime->listAdd(thread, saved_keyword_list, key);
          runtime->listAdd(thread, saved_values, value);
        } else {
          // New, add it and associated value to the varkeyargs dict
          Object hash_obj(&scope, Interpreter::hash(thread, key));
          if (hash_obj.isErrorException()) {
            thread->stackDrop(nargs + 1);
            return *hash_obj;
          }
          word hash = SmallInt::cast(*hash_obj).value();
          Object dict_result(&scope, dictAtPut(thread, dict, key, hash, value));
          if (dict_result.isErrorException()) {
            thread->stackDrop(nargs + 1);
            return *dict_result;
          }
          nargs--;
        }
      }
      // Now, restore the stashed values to the stack and build a new
      // keywords name list.
      thread->stackDrop(num_keyword_args);  // Pop all of the old keyword values
      num_keyword_args = saved_keyword_list.numItems();
      // Replace the old keywords list with a new one.
      keywords = runtime->newTuple(num_keyword_args);
      for (word i = 0; i < num_keyword_args; i++) {
        thread->stackPush(saved_values.at(i));
        keywords.atPut(i, saved_keyword_list.at(i));
      }
      tmp_dict = *dict;
    }
  }
  // At this point, all vararg forms have been normalized
  RawObject* kw_arg_base = (thread->stackPointer() + num_keyword_args) -
                           1;  // pointer to first non-positional arg
  if (UNLIKELY(nargs > expected_args)) {
    thread->stackDrop(nargs + 1);
    return thread->raiseWithFmt(LayoutId::kTypeError, "Too many arguments");
  }
  if (UNLIKELY(nargs < expected_args)) {
    // Too few args passed.  Can we supply default args to make it work?
    // First, normalize & pad keywords and stack arguments
    word name_tuple_size = expected_args - num_positional_args;
    Tuple padded_keywords(&scope, thread->runtime()->newTuple(name_tuple_size));
    for (word i = 0; i < num_keyword_args; i++) {
      padded_keywords.atPut(i, keywords.at(i));
    }
    // Fill in missing spots w/ Error code
    for (word i = num_keyword_args; i < name_tuple_size; i++) {
      thread->stackPush(Error::error());
      nargs++;
      padded_keywords.atPut(i, Error::error());
    }
    keywords = *padded_keywords;
  }
  // Now we've got the right number.  Do they match up?
  RawObject res = checkArgs(thread, function, kw_arg_base, keywords, varnames,
                            num_positional_args);
  if (res.isErrorException()) {
    thread->stackDrop(nargs + 1);
    return res;  // TypeError created by checkArgs.
  }
  CHECK(res.isNoneType(), "checkArgs should return an Error or None");
  // If we're a vararg form, need to push the tuple/dict.
  if (function.hasVarargs()) {
    thread->stackPush(*tmp_varargs);
    nargs++;
  }
  if (function.hasVarkeyargs()) {
    thread->stackPush(*tmp_dict);
    nargs++;
  }
  DCHECK(function.totalArgs() == nargs, "argument count mismatch");
  return *function;
}

// Converts explode arguments into positional arguments.
//
// Returns the new number of positional arguments as a SmallInt, or Error if an
// exception was raised (most likely due to a non-string keyword name).
static RawObject processExplodeArguments(Thread* thread, word flags) {
  HandleScope scope(thread);
  Object kw_mapping(&scope, NoneType::object());
  if (flags & CallFunctionExFlag::VAR_KEYWORDS) {
    kw_mapping = thread->stackPop();
  }
  Tuple positional_args(&scope, thread->stackPop());
  word length = positional_args.length();
  for (word i = 0; i < length; i++) {
    thread->stackPush(positional_args.at(i));
  }
  word nargs = length;
  Runtime* runtime = thread->runtime();
  if (flags & CallFunctionExFlag::VAR_KEYWORDS) {
    if (!kw_mapping.isDict()) {
      DCHECK(runtime->isMapping(thread, kw_mapping),
             "kw_mapping must have __getitem__");
      Dict dict(&scope, runtime->newDict());
      Object result(&scope, dictMergeIgnore(thread, dict, kw_mapping));
      if (result.isErrorException()) {
        thread->stackDrop(nargs + 1);
        if (thread->pendingExceptionType() ==
            runtime->typeAt(LayoutId::kAttributeError)) {
          thread->clearPendingException();
          return thread->raiseWithFmt(LayoutId::kTypeError,
                                      "argument must be a mapping, not %T\n",
                                      &kw_mapping);
        }
        return *result;
      }
      kw_mapping = *dict;
    }
    Dict dict(&scope, *kw_mapping);
    word len = dict.numItems();
    if (len == 0) {
      thread->stackPush(runtime->emptyTuple());
      return SmallInt::fromWord(nargs);
    }
    MutableTuple keys(&scope, runtime->newMutableTuple(len));
    Object key(&scope, NoneType::object());
    Object value(&scope, NoneType::object());
    for (word i = 0, j = 0; dictNextItem(dict, &i, &key, &value); j++) {
      if (!runtime->isInstanceOfStr(*key)) {
        thread->stackDrop(nargs + 1);
        return thread->raiseWithFmt(LayoutId::kTypeError,
                                    "keywords must be strings");
      }
      keys.atPut(j, *key);
      thread->stackPush(*value);
      nargs++;
    }
    thread->stackPush(keys.becomeImmutable());
  }
  return SmallInt::fromWord(nargs);
}

// Takes the outgoing arguments of an explode argument call and rearranges them
// into the form expected by the callee.
RawObject prepareExplodeCall(Thread* thread, word flags,
                             RawFunction function_raw) {
  HandleScope scope(thread);
  Function function(&scope, function_raw);

  RawObject arg_obj = processExplodeArguments(thread, flags);
  if (arg_obj.isErrorException()) return arg_obj;
  word new_argc = SmallInt::cast(arg_obj).value();

  if (flags & CallFunctionExFlag::VAR_KEYWORDS) {
    RawObject result = prepareKeywordCall(thread, new_argc, *function);
    if (result.isErrorException()) {
      return result;
    }
  } else {
    // Are we one of the less common cases?
    if (new_argc != function.argcount() || !(function.hasSimpleCall())) {
      RawObject result = processDefaultArguments(thread, new_argc, *function);
      if (result.isErrorException()) {
        return result;
      }
    }
  }
  return *function;
}

static RawObject createGeneratorObject(Thread* thread,
                                       const Function& function) {
  Runtime* runtime = thread->runtime();
  if (function.isGenerator()) return runtime->newGenerator();
  if (function.isCoroutine()) return runtime->newCoroutine();
  DCHECK(function.isAsyncGenerator(), "unexpected type");
  HandleScope scope(thread);
  Layout async_gen_layout(&scope, runtime->layoutAt(LayoutId::kAsyncGenerator));
  AsyncGenerator async_gen(&scope, runtime->newInstance(async_gen_layout));
  async_gen.setFinalizer(NoneType::object());
  async_gen.setHooksInited(false);
  return *async_gen;
}

static RawObject createGenerator(Thread* thread, const Function& function) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  GeneratorFrame generator_frame(&scope, runtime->newGeneratorFrame(function));
  thread->popFrameToGeneratorFrame(generator_frame);
  GeneratorBase gen_base(&scope, createGeneratorObject(thread, function));
  gen_base.setGeneratorFrame(*generator_frame);
  gen_base.setExceptionState(runtime->newExceptionState());
  gen_base.setQualname(function.qualname());
  gen_base.setName(function.name());
  return *gen_base;
}

RawObject generatorTrampoline(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs));
  RawObject error = preparePositionalCall(thread, nargs, *function);
  if (error.isErrorException()) {
    return error;
  }
  if (UNLIKELY(thread->pushCallFrame(*function) == nullptr)) {
    thread->stackDrop(nargs + 1);
    return Error::exception();
  }
  return createGenerator(thread, function);
}

RawObject generatorTrampolineKw(Thread* thread, word nargs) {
  HandleScope scope(thread);
  // The argument does not include the hidden keyword dictionary argument.  Add
  // one to skip over the keyword dictionary to read the function object.
  Function function(&scope, thread->stackPeek(nargs + 1));
  RawObject error = prepareKeywordCall(thread, nargs, *function);
  if (error.isErrorException()) {
    return error;
  }
  if (UNLIKELY(thread->pushCallFrame(*function) == nullptr)) {
    thread->stackDrop(nargs + 1);
    return Error::exception();
  }
  return createGenerator(thread, function);
}

RawObject generatorTrampolineEx(Thread* thread, word flags) {
  HandleScope scope(thread);
  // The argument is either zero when there is one argument and one when there
  // are two arguments.  Skip over these arguments to read the function object.
  word function_offset = (flags & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1;
  Function function(&scope, thread->stackPeek(function_offset));
  RawObject error = prepareExplodeCall(thread, flags, *function);
  if (error.isErrorException()) {
    return error;
  }
  if (UNLIKELY(thread->pushCallFrame(*function) == nullptr)) {
    thread->stackDrop(function_offset + 1);
    return Error::exception();
  }
  return createGenerator(thread, function);
}

RawObject generatorClosureTrampoline(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs));
  RawObject error = preparePositionalCall(thread, nargs, *function);
  if (error.isErrorException()) {
    return error;
  }
  Frame* callee_frame = thread->pushCallFrame(*function);
  if (UNLIKELY(callee_frame == nullptr)) {
    thread->stackDrop(nargs + 1);
    return Error::exception();
  }
  processFreevarsAndCellvars(thread, callee_frame);
  return createGenerator(thread, function);
}

RawObject generatorClosureTrampolineKw(Thread* thread, word nargs) {
  HandleScope scope(thread);
  // The argument does not include the hidden keyword dictionary argument.  Add
  // one to skip the keyword dictionary to get to the function object.
  Function function(&scope, thread->stackPeek(nargs + 1));
  RawObject error = prepareKeywordCall(thread, nargs, *function);
  if (error.isErrorException()) {
    return error;
  }
  Frame* callee_frame = thread->pushCallFrame(*function);
  if (UNLIKELY(callee_frame == nullptr)) {
    thread->stackDrop(nargs + 2);
    return Error::exception();
  }
  processFreevarsAndCellvars(thread, callee_frame);
  return createGenerator(thread, function);
}

RawObject generatorClosureTrampolineEx(Thread* thread, word flags) {
  HandleScope scope(thread);
  // The argument is either zero when there is one argument and one when there
  // are two arguments.  Skip over these arguments to read the function object.
  word function_offset = (flags & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1;
  Function function(&scope, thread->stackPeek(function_offset));
  RawObject error = prepareExplodeCall(thread, flags, *function);
  if (error.isErrorException()) {
    return error;
  }
  Frame* callee_frame = thread->pushCallFrame(*function);
  if (UNLIKELY(callee_frame == nullptr)) {
    thread->stackDrop(function_offset + 1);
    return Error::exception();
  }
  processFreevarsAndCellvars(thread, callee_frame);
  return createGenerator(thread, function);
}

RawObject interpreterTrampoline(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs));
  RawObject error = preparePositionalCall(thread, nargs, *function);
  if (error.isErrorException()) {
    return error;
  }
  if (UNLIKELY(thread->pushCallFrame(*function) == nullptr)) {
    thread->stackDrop(nargs + 1);
    return Error::exception();
  }
  return Interpreter::execute(thread);
}

RawObject interpreterTrampolineKw(Thread* thread, word nargs) {
  HandleScope scope(thread);
  // The argument does not include the hidden keyword dictionary argument.  Add
  // one to skip the keyword dictionary to get to the function object.
  Function function(&scope, thread->stackPeek(nargs + 1));
  RawObject error = prepareKeywordCall(thread, nargs, *function);
  if (error.isErrorException()) {
    return error;
  }
  if (UNLIKELY(thread->pushCallFrame(*function) == nullptr)) {
    thread->stackDrop(nargs + 2);
    return Error::exception();
  }
  return Interpreter::execute(thread);
}

RawObject interpreterTrampolineEx(Thread* thread, word flags) {
  HandleScope scope(thread);
  // The argument is either zero when there is one argument and one when there
  // are two arguments.  Skip over these arguments to read the function object.
  word function_offset = (flags & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1;
  Function function(&scope, thread->stackPeek(function_offset));
  RawObject error = prepareExplodeCall(thread, flags, *function);
  if (error.isErrorException()) {
    return error;
  }
  if (UNLIKELY(thread->pushCallFrame(*function) == nullptr)) {
    thread->stackDrop(function_offset + 1);
    return Error::exception();
  }
  return Interpreter::execute(thread);
}

RawObject interpreterClosureTrampoline(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs));
  RawObject error = preparePositionalCall(thread, nargs, *function);
  if (error.isErrorException()) {
    return error;
  }
  Frame* callee_frame = thread->pushCallFrame(*function);
  if (UNLIKELY(callee_frame == nullptr)) {
    thread->stackDrop(nargs + 1);
    return Error::exception();
  }
  processFreevarsAndCellvars(thread, callee_frame);
  return Interpreter::execute(thread);
}

RawObject interpreterClosureTrampolineKw(Thread* thread, word nargs) {
  HandleScope scope(thread);
  // The argument does not include the hidden keyword dictionary argument.  Add
  // one to skip the keyword dictionary to get to the function object.
  Function function(&scope, thread->stackPeek(nargs + 1));
  RawObject error = prepareKeywordCall(thread, nargs, *function);
  if (error.isErrorException()) {
    return error;
  }
  Frame* callee_frame = thread->pushCallFrame(*function);
  if (UNLIKELY(callee_frame == nullptr)) {
    thread->stackDrop(nargs + 2);
    return Error::exception();
  }
  processFreevarsAndCellvars(thread, callee_frame);
  return Interpreter::execute(thread);
}

RawObject interpreterClosureTrampolineEx(Thread* thread, word flags) {
  HandleScope scope(thread);
  // The argument is either zero when there is one argument and one when there
  // are two arguments.  Skip over these arguments to read the function object.
  word function_offset = (flags & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1;
  Function function(&scope, thread->stackPeek(function_offset));
  RawObject error = prepareExplodeCall(thread, flags, *function);
  if (error.isErrorException()) {
    return error;
  }
  Frame* callee_frame = thread->pushCallFrame(*function);
  if (UNLIKELY(callee_frame == nullptr)) {
    thread->stackDrop(function_offset + 1);
    return Error::exception();
  }
  processFreevarsAndCellvars(thread, callee_frame);
  return Interpreter::execute(thread);
}

// method no args

static RawObject callMethNoArgs(Thread* thread, const Function& function,
                                const Object& self) {
  HandleScope scope(thread);
  Int address(&scope, function.code());
  binaryfunc method = bit_cast<binaryfunc>(address.asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  PyObject* pyresult = (*method)(self_obj, nullptr);
  Object result(&scope, ApiHandle::checkFunctionResult(thread, pyresult));
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  return *result;
}

static RawObject raiseTypeErrorMustBeBound(Thread* thread,
                                           const Function& function) {
  HandleScope scope(thread);
  Str function_name(&scope, function.name());
  return thread->raiseWithFmt(
      LayoutId::kTypeError, "'%S' must be bound to an object", &function_name);
}

static RawObject raiseTypeErrorNoArguments(Thread* thread,
                                           const Function& function,
                                           word nargs) {
  HandleScope scope(thread);
  Str function_name(&scope, function.name());
  if (nargs == 0) {
    return raiseTypeErrorMustBeBound(thread, function);
  }
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "'%S' takes no arguments (%w given)",
                              &function_name, nargs - 1);
}

static RawObject raiseTypeErrorNoKeywordArguments(Thread* thread,
                                                  const Function& function) {
  HandleScope scope(thread);
  Str function_name(&scope, function.name());
  return thread->raiseWithFmt(
      LayoutId::kTypeError, "'%S' takes no keyword arguments", &function_name);
}

RawObject methodTrampolineNoArgs(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs));
  if (nargs != 1) {
    RawObject result = raiseTypeErrorNoArguments(thread, function, nargs);
    thread->stackDrop(nargs + 1);
    return result;
  }
  Object self(&scope, thread->stackPeek(0));
  RawObject result = callMethNoArgs(thread, function, self);
  thread->stackDrop(nargs + 1);
  return result;
}

RawObject methodTrampolineNoArgsKw(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs + 1));
  Tuple kw_names(&scope, thread->stackPeek(0));
  if (kw_names.length() != 0) {
    RawObject result = raiseTypeErrorNoKeywordArguments(thread, function);
    thread->stackDrop(nargs + 2);
    return result;
  }
  if (nargs != 1) {
    RawObject result = raiseTypeErrorNoArguments(thread, function, nargs);
    thread->stackDrop(nargs + 2);
    return result;
  }
  Object self(&scope, thread->stackPeek(1));
  RawObject result = callMethNoArgs(thread, function, self);
  thread->stackDrop(nargs + 2);
  return result;
}

RawObject methodTrampolineNoArgsEx(Thread* thread, word flags) {
  HandleScope scope(thread);
  bool has_varkeywords = flags & CallFunctionExFlag::VAR_KEYWORDS;
  Function function(&scope, thread->stackPeek(has_varkeywords + 1));
  Tuple args(&scope, thread->stackPeek(has_varkeywords));
  if (has_varkeywords) {
    Object kw_args(&scope, thread->stackTop());
    if (!kw_args.isDict()) UNIMPLEMENTED("mapping kwargs");
    if (Dict::cast(*kw_args).numItems() != 0) {
      RawObject result = raiseTypeErrorNoKeywordArguments(thread, function);
      thread->stackDrop(has_varkeywords + 2);
      return result;
    }
  }
  word args_length = args.length();
  if (args_length != 1) {
    RawObject result = raiseTypeErrorNoArguments(thread, function, args_length);
    thread->stackDrop(has_varkeywords + 2);
    return result;
  }
  Object self(&scope, args.at(0));
  RawObject result = callMethNoArgs(thread, function, self);
  thread->stackDrop(has_varkeywords + 2);
  return result;
}

// method one arg

static RawObject raiseTypeErrorOneArgument(Thread* thread,
                                           const Function& function,
                                           word nargs) {
  HandleScope scope(thread);
  Str function_name(&scope, function.name());
  if (nargs == 0) {
    return raiseTypeErrorMustBeBound(thread, function);
  }
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "'%S' takes exactly one argument (%w given)",
                              &function_name, nargs - 1);
}

static RawObject callMethOneArg(Thread* thread, const Function& function,
                                const Object& self, const Object& arg) {
  HandleScope scope(thread);
  Int address(&scope, function.code());
  binaryfunc method = bit_cast<binaryfunc>(address.asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  PyObject* arg_obj = ApiHandle::newReference(thread, *arg);
  PyObject* pyresult = (*method)(self_obj, arg_obj);
  Object result(&scope, ApiHandle::checkFunctionResult(thread, pyresult));
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  ApiHandle::fromPyObject(arg_obj)->decref();
  return *result;
}

RawObject methodTrampolineOneArg(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs));
  if (nargs != 2) {
    RawObject result = raiseTypeErrorOneArgument(thread, function, nargs);
    thread->stackDrop(nargs + 1);
    return result;
  }
  Object self(&scope, thread->stackPeek(1));
  Object arg(&scope, thread->stackPeek(0));
  RawObject result = callMethOneArg(thread, function, self, arg);
  thread->stackDrop(nargs + 1);
  return result;
}

RawObject methodTrampolineOneArgKw(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs + 1));
  Tuple kw_names(&scope, thread->stackPeek(0));
  if (kw_names.length() != 0) {
    RawObject result = raiseTypeErrorNoKeywordArguments(thread, function);
    thread->stackDrop(nargs + 2);
    return result;
  }
  if (nargs != 2) {
    RawObject result = raiseTypeErrorOneArgument(thread, function, nargs);
    thread->stackDrop(nargs + 2);
    return result;
  }
  Object self(&scope, thread->stackPeek(2));
  Object arg(&scope, thread->stackPeek(1));
  RawObject result = callMethOneArg(thread, function, self, arg);
  thread->stackDrop(nargs + 2);
  return result;
}

RawObject methodTrampolineOneArgEx(Thread* thread, word flags) {
  HandleScope scope(thread);
  bool has_varkeywords = flags & CallFunctionExFlag::VAR_KEYWORDS;
  Function function(&scope, thread->stackPeek(has_varkeywords + 1));
  if (has_varkeywords) {
    Object kw_args(&scope, thread->stackTop());
    if (!kw_args.isDict()) UNIMPLEMENTED("mapping kwargs");
    if (Dict::cast(*kw_args).numItems() != 0) {
      RawObject result = raiseTypeErrorNoKeywordArguments(thread, function);
      thread->stackDrop(has_varkeywords + 2);
      return result;
    }
  }
  Tuple varargs(&scope, thread->stackPeek(has_varkeywords));
  if (varargs.length() != 2) {
    RawObject result =
        raiseTypeErrorOneArgument(thread, function, varargs.length());
    thread->stackDrop(has_varkeywords + 2);
    return result;
  }
  Object self(&scope, varargs.at(0));
  Object arg(&scope, varargs.at(1));
  RawObject result = callMethOneArg(thread, function, self, arg);
  thread->stackDrop(has_varkeywords + 2);
  return result;
}

// callMethVarArgs

static RawObject callMethVarArgs(Thread* thread, const Function& function,
                                 const Object& self, const Object& varargs) {
  HandleScope scope(thread);
  Int address(&scope, function.code());
  binaryfunc method = bit_cast<binaryfunc>(address.asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  PyObject* varargs_obj = ApiHandle::newReference(thread, *varargs);
  PyObject* pyresult = (*method)(self_obj, varargs_obj);
  Object result(&scope, ApiHandle::checkFunctionResult(thread, pyresult));
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  ApiHandle::fromPyObject(varargs_obj)->decref();
  return *result;
}

RawObject methodTrampolineVarArgs(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs));
  if (nargs == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(nargs + 1);
    return result;
  }
  Object self(&scope, thread->stackPeek(nargs - 1));
  Tuple varargs(&scope, thread->runtime()->newTuple(nargs - 1));
  for (word i = 0; i < nargs - 1; i++) {
    varargs.atPut(nargs - i - 2, thread->stackPeek(i));
  }
  RawObject result = callMethVarArgs(thread, function, self, varargs);
  thread->stackDrop(nargs + 1);
  return result;
}

RawObject methodTrampolineVarArgsKw(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs + 1));
  Tuple kw_names(&scope, thread->stackPeek(0));
  if (kw_names.length() != 0) {
    RawObject result = raiseTypeErrorNoKeywordArguments(thread, function);
    thread->stackDrop(nargs + 2);
    return result;
  }
  if (nargs == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(nargs + 2);
    return result;
  }
  Object self(&scope, thread->stackPeek(nargs));
  Tuple varargs(&scope, thread->runtime()->newTuple(nargs - 1));
  for (word i = 0; i < nargs - 1; i++) {
    varargs.atPut(i, thread->stackPeek(nargs - i - 1));
  }
  RawObject result = callMethVarArgs(thread, function, self, varargs);
  thread->stackDrop(nargs + 2);
  return result;
}

RawObject methodTrampolineVarArgsEx(Thread* thread, word flags) {
  HandleScope scope(thread);
  bool has_varkeywords = flags & CallFunctionExFlag::VAR_KEYWORDS;
  Function function(&scope, thread->stackPeek(has_varkeywords + 1));
  if (has_varkeywords) {
    Object kw_args(&scope, thread->stackTop());
    if (!kw_args.isDict()) UNIMPLEMENTED("mapping kwargs");
    if (Dict::cast(*kw_args).numItems() != 0) {
      RawObject result = raiseTypeErrorNoKeywordArguments(thread, function);
      thread->stackDrop(has_varkeywords + 2);
      return result;
    }
  }
  Tuple args(&scope, thread->stackPeek(has_varkeywords));
  if (args.length() == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(has_varkeywords + 2);
    return result;
  }
  Object self(&scope, args.at(0));
  Object varargs(&scope, thread->runtime()->tupleSubseq(thread, args, 1,
                                                        args.length() - 1));
  RawObject result = callMethVarArgs(thread, function, self, varargs);
  thread->stackDrop(has_varkeywords + 2);
  return result;
}

// callMethKeywordArgs

static RawObject callMethKeywords(Thread* thread, const Function& function,
                                  const Object& self, const Object& args,
                                  const Object& kwargs) {
  HandleScope scope(thread);
  Int address(&scope, function.code());
  ternaryfunc method = bit_cast<ternaryfunc>(address.asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  PyObject* args_obj = ApiHandle::newReference(thread, *args);
  PyObject* kwargs_obj = nullptr;
  if (*kwargs != NoneType::object()) {
    kwargs_obj = ApiHandle::newReference(thread, *kwargs);
  }
  PyObject* pyresult = (*method)(self_obj, args_obj, kwargs_obj);
  Object result(&scope, ApiHandle::checkFunctionResult(thread, pyresult));
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  ApiHandle::fromPyObject(args_obj)->decref();
  if (kwargs_obj != nullptr) {
    ApiHandle::fromPyObject(kwargs_obj)->decref();
  }
  return *result;
}

RawObject methodTrampolineKeywords(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function function(&scope, thread->stackPeek(nargs));
  if (nargs == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(nargs + 1);
    return result;
  }
  Object self(&scope, thread->stackPeek(nargs - 1));
  Tuple varargs(&scope, runtime->newTuple(nargs - 1));
  for (word i = 0; i < nargs - 1; i++) {
    varargs.atPut(nargs - i - 2, thread->stackPeek(i));
  }
  Object keywords(&scope, NoneType::object());
  RawObject result =
      callMethKeywords(thread, function, self, varargs, keywords);
  thread->stackDrop(nargs + 1);
  return result;
}

RawObject methodTrampolineKeywordsKw(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple kw_names(&scope, thread->stackPeek(0));
  Object kwargs(&scope, NoneType::object());
  word num_keywords = kw_names.length();
  if (num_keywords != 0) {
    Dict dict(&scope, runtime->newDict());
    for (word i = 0; i < num_keywords; i++) {
      Str name(&scope, kw_names.at(i));
      Object value(&scope, thread->stackPeek(num_keywords - i));
      dictAtPutByStr(thread, dict, name, value);
    }
    kwargs = *dict;
  }
  Function function(&scope, thread->stackPeek(nargs + 1));
  if (nargs - num_keywords == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(nargs + 2);
    return result;
  }
  word num_positional = nargs - num_keywords - 1;
  Tuple args(&scope, runtime->newTuple(num_positional));
  for (word i = 0; i < num_positional; i++) {
    args.atPut(i, thread->stackPeek(nargs - i - 1));
  }
  Object self(&scope, thread->stackPeek(nargs));
  RawObject result = callMethKeywords(thread, function, self, args, kwargs);
  thread->stackDrop(nargs + 2);
  return result;
}

RawObject methodTrampolineKeywordsEx(Thread* thread, word flags) {
  HandleScope scope(thread);
  bool has_varkeywords = flags & CallFunctionExFlag::VAR_KEYWORDS;
  Tuple varargs(&scope, thread->stackPeek(has_varkeywords));
  Object kwargs(&scope, NoneType::object());
  if (has_varkeywords) {
    kwargs = thread->stackTop();
    if (!kwargs.isDict()) UNIMPLEMENTED("mapping kwargs");
  }
  Function function(&scope, thread->stackPeek(has_varkeywords + 1));
  if (varargs.length() == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(has_varkeywords + 2);
    return result;
  }
  Object self(&scope, varargs.at(0));
  Object args(&scope, thread->runtime()->tupleSubseq(thread, varargs, 1,
                                                     varargs.length() - 1));
  RawObject result = callMethKeywords(thread, function, self, args, kwargs);
  thread->stackDrop(has_varkeywords + 2);
  return result;
}

static RawObject callMethFast(Thread* thread, const Function& function,
                              const Object& self, PyObject* const* args,
                              word num_args) {
  _PyCFunctionFast method =
      bit_cast<_PyCFunctionFast>(Int::cast(function.code()).asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  PyObject* pyresult = (*method)(self_obj, args, num_args);
  RawObject result = ApiHandle::checkFunctionResult(thread, pyresult);
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  return result;
}

RawObject methodTrampolineFast(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs));
  if (nargs == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(nargs + 1);
    return result;
  }
  Object self(&scope, thread->stackPeek(nargs - 1));

  word num_positional = nargs - 1;
  std::unique_ptr<PyObject*[]> fastcall_args(new PyObject*[num_positional]);
  for (word i = 0; i < num_positional; i++) {
    fastcall_args[nargs - i - 2] =
        ApiHandle::newReference(thread, thread->stackPeek(i));
  }
  Object result(&scope, callMethFast(thread, function, self,
                                     fastcall_args.get(), num_positional));
  for (word i = 0; i < num_positional; i++) {
    ApiHandle::fromPyObject(fastcall_args[nargs - i - 2])->decref();
  }
  thread->stackDrop(nargs + 1);
  return *result;
}

RawObject methodTrampolineFastKw(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs + 1));
  if (nargs == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(nargs + 2);
    return result;
  }
  Tuple kw_names(&scope, thread->stackPeek(0));
  if (kw_names.length() != 0) {
    RawObject result = raiseTypeErrorNoKeywordArguments(thread, function);
    thread->stackDrop(nargs + 2);
    return result;
  }

  Object self(&scope, thread->stackPeek(nargs));
  word num_positional = nargs - 1;
  std::unique_ptr<PyObject*[]> fastcall_args(new PyObject*[num_positional]);
  for (word i = 0; i < num_positional; i++) {
    fastcall_args[i] =
        ApiHandle::newReference(thread, thread->stackPeek(nargs - i - 1));
  }
  Object result(&scope, callMethFast(thread, function, self,
                                     fastcall_args.get(), nargs - 1));
  for (word i = 0; i < num_positional; i++) {
    ApiHandle::fromPyObject(fastcall_args[i])->decref();
  }
  thread->stackDrop(nargs + 2);
  return *result;
}

RawObject methodTrampolineFastEx(Thread* thread, word flags) {
  HandleScope scope(thread);
  bool has_varkeywords = flags & CallFunctionExFlag::VAR_KEYWORDS;
  Function function(&scope, thread->stackPeek(has_varkeywords + 1));

  // Get the keyword arguments
  if (has_varkeywords) {
    Object kw_args_obj(&scope, thread->stackTop());
    if (!kw_args_obj.isDict()) UNIMPLEMENTED("mapping kwargs");
    Dict kw_args(&scope, *kw_args_obj);
    if (kw_args.numItems() != 0) {
      RawObject result = raiseTypeErrorNoKeywordArguments(thread, function);
      thread->stackDrop(has_varkeywords + 2);
      return result;
    }
  }

  Tuple args(&scope, thread->stackPeek(has_varkeywords));
  word args_length = args.length();
  if (args_length == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(has_varkeywords + 2);
    return result;
  }
  Object self(&scope, args.at(0));
  word num_positional = args_length - 1;
  std::unique_ptr<PyObject*[]> fastcall_args(new PyObject*[num_positional]);

  // Set the positional arguments
  for (word i = 0; i < num_positional; i++) {
    fastcall_args[i] = ApiHandle::newReference(thread, args.at(i + 1));
  }

  Object result(&scope, callMethFast(thread, function, self,
                                     fastcall_args.get(), num_positional));
  for (word i = 0; i < num_positional; i++) {
    ApiHandle::fromPyObject(fastcall_args[i])->decref();
  }
  thread->stackDrop(has_varkeywords + 2);
  return *result;
}

static RawObject callMethFastWithKeywordsWithKwargs(
    Thread* thread, const Function& function, const Object& self,
    PyObject* const* args, word num_args, const Object& kw_names) {
  _PyCFunctionFastWithKeywords method = bit_cast<_PyCFunctionFastWithKeywords>(
      Int::cast(function.code()).asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  ApiHandle* kw_names_obj = ApiHandle::newReference(thread, *kw_names);
  PyObject* pyresult = (*method)(self_obj, args, num_args, kw_names_obj);
  RawObject result = ApiHandle::checkFunctionResult(thread, pyresult);
  kw_names_obj->decref();
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  return result;
}

static RawObject callMethFastWithKeywords(Thread* thread,
                                          const Function& function,
                                          const Object& self,
                                          PyObject* const* args,
                                          word num_args) {
  _PyCFunctionFastWithKeywords method = bit_cast<_PyCFunctionFastWithKeywords>(
      Int::cast(function.code()).asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  PyObject* pyresult = (*method)(self_obj, args, num_args, nullptr);
  RawObject result = ApiHandle::checkFunctionResult(thread, pyresult);
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  return result;
}

RawObject methodTrampolineFastWithKeywords(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs));
  if (nargs == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(nargs + 1);
    return result;
  }
  Object self(&scope, thread->stackPeek(nargs - 1));

  std::unique_ptr<PyObject*[]> fastcall_args(new PyObject*[nargs - 1]);
  for (word i = 0; i < nargs - 1; i++) {
    fastcall_args[nargs - i - 2] =
        ApiHandle::newReference(thread, thread->stackPeek(i));
  }
  word num_positional = nargs - 1;
  Object result(&scope,
                callMethFastWithKeywords(thread, function, self,
                                         fastcall_args.get(), num_positional));
  for (word i = 0; i < nargs - 1; i++) {
    ApiHandle::fromPyObject(fastcall_args[nargs - i - 2])->decref();
  }
  thread->stackDrop(nargs + 1);
  return *result;
}

RawObject methodTrampolineFastWithKeywordsKw(Thread* thread, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, thread->stackPeek(nargs + 1));
  if (nargs == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(nargs + 2);
    return result;
  }
  Object self(&scope, thread->stackPeek(nargs));

  std::unique_ptr<PyObject*[]> fastcall_args(new PyObject*[nargs - 1]);
  for (word i = 0; i < nargs - 1; i++) {
    fastcall_args[i] =
        ApiHandle::newReference(thread, thread->stackPeek(nargs - i - 1));
  }
  Tuple kw_names(&scope, thread->stackPeek(0));
  word num_positional = nargs - kw_names.length() - 1;
  Object result(&scope, callMethFastWithKeywordsWithKwargs(
                            thread, function, self, fastcall_args.get(),
                            num_positional, kw_names));
  for (word i = 0; i < nargs - 1; i++) {
    ApiHandle::fromPyObject(fastcall_args[i])->decref();
  }
  thread->stackDrop(nargs + 2);
  return *result;
}

RawObject methodTrampolineFastWithKeywordsEx(Thread* thread, word flags) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  bool has_varkeywords = flags & CallFunctionExFlag::VAR_KEYWORDS;
  word num_keywords = 0;

  // Get the keyword arguments
  Tuple kw_names(&scope, runtime->emptyTuple());
  if (has_varkeywords) {
    Object kw_args_obj(&scope, thread->stackTop());
    if (!kw_args_obj.isDict()) UNIMPLEMENTED("mapping kwargs");
    Dict kw_args(&scope, *kw_args_obj);
    num_keywords = kw_args.numItems();
  }

  Function function(&scope, thread->stackPeek(has_varkeywords + 1));
  Tuple args(&scope, thread->stackPeek(has_varkeywords));
  word args_length = args.length();
  if (args_length == 0) {
    RawObject result = raiseTypeErrorMustBeBound(thread, function);
    thread->stackDrop(has_varkeywords + 2);
    return result;
  }
  Object self(&scope, args.at(0));
  word num_positional = args_length - 1;
  std::unique_ptr<PyObject*[]> fastcall_args(
      new PyObject*[num_positional + num_keywords]);

  // Set the positional arguments
  for (word i = 0; i < num_positional; i++) {
    fastcall_args[i] = ApiHandle::newReference(thread, args.at(i + 1));
  }

  // Set the keyword arguments
  if (has_varkeywords) {
    Dict kw_args(&scope, thread->stackTop());

    Object key(&scope, NoneType::object());
    Object value(&scope, NoneType::object());
    kw_names = runtime->newTuple(num_keywords);
    for (word dict_i = 0, arg_i = 0;
         dictNextItem(kw_args, &dict_i, &key, &value); arg_i++) {
      kw_names.atPut(arg_i, *key);
      fastcall_args[num_positional + arg_i] =
          ApiHandle::newReference(thread, *value);
    }
  }

  Object result(&scope, NoneType::object());
  if (!has_varkeywords) {
    result = callMethFastWithKeywords(thread, function, self,
                                      fastcall_args.get(), num_positional);
  } else {
    result = callMethFastWithKeywordsWithKwargs(
        thread, function, self, fastcall_args.get(), num_positional, kw_names);
  }
  for (word i = 0; i < num_positional + num_keywords; i++) {
    ApiHandle::fromPyObject(fastcall_args[i])->decref();
  }
  thread->stackDrop(has_varkeywords + 2);
  return *result;
}

RawObject unimplementedTrampoline(Thread*, word) {
  UNIMPLEMENTED("Trampoline");
}

static inline RawObject builtinTrampolineImpl(Thread* thread, word arg,
                                              word function_idx,
                                              PrepareCallFunc prepare_call) {
  // Warning: This code is using `RawXXX` variables for performance! This is
  // despite the fact that we call functions that do potentially perform memory
  // allocations. This is legal here because we always rely on the functions
  // returning an up-to-date address and we make sure to never access any value
  // produce before a call after that call. Be careful not to break this
  // invariant if you change the code!

  RawObject prepare_result = prepare_call(
      thread, arg, Function::cast(thread->stackPeek(function_idx)));
  if (prepare_result.isErrorException()) {
    return prepare_result;
  }
  RawFunction function_obj = Function::cast(prepare_result);

  RawObject result = NoneType::object();
  {
    DCHECK(!function_obj.code().isNoneType(),
           "builtin functions should have annotated code objects");
    RawCode code = Code::cast(function_obj.code());
    DCHECK(code.code().isSmallInt(),
           "builtin functions should contain entrypoint in code.code");
    BuiltinFunction function =
        bit_cast<BuiltinFunction>(SmallInt::cast(code.code()).asCPtr());

    word nargs = function_obj.totalArgs();
    Frame* callee_frame = thread->pushNativeFrame(nargs);
    if (UNLIKELY(callee_frame == nullptr)) {
      thread->stackDrop(nargs + 1);
      return Error::exception();
    }
    result = (*function)(thread, Arguments(callee_frame));
    // End scope so people do not accidentally use raw variables after the call
    // which could have triggered a GC.
  }
  DCHECK(thread->isErrorValueOk(result), "error/exception mismatch");
  thread->popFrame();
  return result;
}

RawObject builtinTrampoline(Thread* thread, word nargs) {
  return builtinTrampolineImpl(thread, nargs, /*function_idx=*/nargs,
                               preparePositionalCall);
}

RawObject builtinTrampolineKw(Thread* thread, word nargs) {
  return builtinTrampolineImpl(thread, nargs, /*function_idx=*/nargs + 1,
                               prepareKeywordCall);
}

RawObject builtinTrampolineEx(Thread* thread, word flags) {
  return builtinTrampolineImpl(
      thread, flags,
      /*function_idx=*/(flags & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1,
      prepareExplodeCall);
}

}  // namespace py
