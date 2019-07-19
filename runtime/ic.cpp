#include "ic.h"

#include "bytearray-builtins.h"
#include "bytecode.h"
#include "runtime.h"
#include "utils.h"

namespace python {

struct RewrittenOp {
  Bytecode bc;
  int32_t arg;
  bool needs_inline_cache;
};

static void rewriteZeroArgMethodCallsUsingLoadMethodCached(
    const MutableBytes& bytecode) {
  word bytecode_length = bytecode.length();
  Bytecode prev = LOAD_METHOD;
  word prev_index = 0;
  for (word i = 0; i < bytecode_length;) {
    BytecodeOp op = nextBytecodeOp(bytecode, &i);
    if (prev == LOAD_ATTR_CACHED && op.bc == CALL_FUNCTION && op.arg == 0) {
      bytecode.byteAtPut(prev_index, LOAD_METHOD_CACHED);
      bytecode.byteAtPut(i - 2, CALL_METHOD);
    }
    prev = op.bc;
    prev_index = i - 2;
  }
}

static RewrittenOp rewriteOperation(const Function& function, BytecodeOp op) {
  auto cached_binop = [](Interpreter::BinaryOp bin_op) {
    return RewrittenOp{BINARY_OP_CACHED, static_cast<int32_t>(bin_op), true};
  };
  auto cached_inplace = [](Interpreter::BinaryOp bin_op) {
    return RewrittenOp{INPLACE_OP_CACHED, static_cast<int32_t>(bin_op), true};
  };
  switch (op.bc) {
    case BINARY_ADD:
      return cached_binop(Interpreter::BinaryOp::ADD);
    case BINARY_AND:
      return cached_binop(Interpreter::BinaryOp::AND);
    case BINARY_FLOOR_DIVIDE:
      return cached_binop(Interpreter::BinaryOp::FLOORDIV);
    case BINARY_LSHIFT:
      return cached_binop(Interpreter::BinaryOp::LSHIFT);
    case BINARY_MATRIX_MULTIPLY:
      return cached_binop(Interpreter::BinaryOp::MATMUL);
    case BINARY_MODULO:
      return cached_binop(Interpreter::BinaryOp::MOD);
    case BINARY_MULTIPLY:
      return cached_binop(Interpreter::BinaryOp::MUL);
    case BINARY_OR:
      return cached_binop(Interpreter::BinaryOp::OR);
    case BINARY_POWER:
      return cached_binop(Interpreter::BinaryOp::POW);
    case BINARY_RSHIFT:
      return cached_binop(Interpreter::BinaryOp::RSHIFT);
    case BINARY_SUBSCR:
      return RewrittenOp{BINARY_SUBSCR_CACHED, op.arg, true};
    case BINARY_SUBTRACT:
      return cached_binop(Interpreter::BinaryOp::SUB);
    case BINARY_TRUE_DIVIDE:
      return cached_binop(Interpreter::BinaryOp::TRUEDIV);
    case BINARY_XOR:
      return cached_binop(Interpreter::BinaryOp::XOR);
    case COMPARE_OP:
      switch (op.arg) {
        case CompareOp::LT:
        case CompareOp::LE:
        case CompareOp::EQ:
        case CompareOp::NE:
        case CompareOp::GT:
        case CompareOp::GE:
          return RewrittenOp{COMPARE_OP_CACHED, op.arg, true};
        case CompareOp::IS:
          return RewrittenOp{COMPARE_IS, 0, false};
        case CompareOp::IS_NOT:
          return RewrittenOp{COMPARE_IS_NOT, 0, false};
      }
      break;
    case FOR_ITER:
      return RewrittenOp{FOR_ITER_CACHED, op.arg, true};
    case INPLACE_ADD:
      return cached_inplace(Interpreter::BinaryOp::ADD);
    case INPLACE_AND:
      return cached_inplace(Interpreter::BinaryOp::AND);
    case INPLACE_FLOOR_DIVIDE:
      return cached_inplace(Interpreter::BinaryOp::FLOORDIV);
    case INPLACE_LSHIFT:
      return cached_inplace(Interpreter::BinaryOp::LSHIFT);
    case INPLACE_MATRIX_MULTIPLY:
      return cached_inplace(Interpreter::BinaryOp::MATMUL);
    case INPLACE_MODULO:
      return cached_inplace(Interpreter::BinaryOp::MOD);
    case INPLACE_MULTIPLY:
      return cached_inplace(Interpreter::BinaryOp::MUL);
    case INPLACE_OR:
      return cached_inplace(Interpreter::BinaryOp::OR);
    case INPLACE_POWER:
      return cached_inplace(Interpreter::BinaryOp::POW);
    case INPLACE_RSHIFT:
      return cached_inplace(Interpreter::BinaryOp::RSHIFT);
    case INPLACE_SUBTRACT:
      return cached_inplace(Interpreter::BinaryOp::SUB);
    case INPLACE_TRUE_DIVIDE:
      return cached_inplace(Interpreter::BinaryOp::TRUEDIV);
    case INPLACE_XOR:
      return cached_inplace(Interpreter::BinaryOp::XOR);
    case LOAD_ATTR:
      return RewrittenOp{LOAD_ATTR_CACHED, op.arg, true};
    case LOAD_FAST: {
      CHECK(op.arg < Code::cast(function.code()).nlocals(),
            "unexpected local number");
      word total_locals = function.totalLocals();
      int32_t reverse_arg = total_locals - op.arg - 1;
      return RewrittenOp{LOAD_FAST_REVERSE, reverse_arg, false};
    }
    case LOAD_METHOD:
      return RewrittenOp{LOAD_METHOD_CACHED, op.arg, true};
    case STORE_ATTR:
      return RewrittenOp{STORE_ATTR_CACHED, op.arg, true};
    case STORE_FAST: {
      CHECK(op.arg < Code::cast(function.code()).nlocals(),
            "unexpected local number");
      word total_locals = function.totalLocals();
      int32_t reverse_arg = total_locals - op.arg - 1;
      return RewrittenOp{STORE_FAST_REVERSE, reverse_arg, false};
    }
    case LOAD_CONST: {
      RawObject arg_obj =
          Tuple::cast(Code::cast(function.code()).consts()).at(op.arg);
      if (!arg_obj.isHeapObject() &&
          // This condition is true only the object fits in a byte. Some
          // immediate values of SmallInt and SmallStr do not satify this
          // condition.
          arg_obj == objectFromOparg(opargFromObject(arg_obj))) {
        return RewrittenOp{LOAD_IMMEDIATE, opargFromObject(arg_obj), false};
      }
    } break;
    case BINARY_OP_CACHED:
    case COMPARE_OP_CACHED:
    case FOR_ITER_CACHED:
    case INPLACE_OP_CACHED:
    case LOAD_ATTR_CACHED:
    case LOAD_FAST_REVERSE:
    case LOAD_METHOD_CACHED:
    case STORE_ATTR_CACHED:
    case STORE_FAST_REVERSE:
      UNREACHABLE("should not have cached opcode in input");
    default:
      break;
  }
  return RewrittenOp{op.bc, op.arg, false};
}

void icRewriteBytecode(Thread* thread, const Function& function) {
  HandleScope scope(thread);
  word num_caches = 0;

  // Scan bytecode to figure out how many caches we need.
  MutableBytes bytecode(&scope, function.rewrittenBytecode());
  word bytecode_length = bytecode.length();
  for (word i = 0; i < bytecode_length;) {
    BytecodeOp op = nextBytecodeOp(bytecode, &i);
    RewrittenOp rewritten = rewriteOperation(function, op);
    if (rewritten.needs_inline_cache) {
      num_caches++;
    }
  }

  // Add cache entries for global variables.  This is going to over allocate
  // somewhat in order to simplify the indexing arithmetic.  Not all names are
  // used for globals, some are used for attributes.  This is good enough for
  // now.
  word names_length = Tuple::cast(Code::cast(function.code()).names()).length();
  word num_global_caches = Utils::roundUpDiv(names_length, kIcPointersPerCache);
  num_caches += num_global_caches;

  Runtime* runtime = thread->runtime();
  Tuple original_arguments(&scope, runtime->newTuple(num_caches));
  for (word i = 0, cache = num_global_caches; i < bytecode_length;) {
    word begin = i;
    BytecodeOp op = nextBytecodeOp(bytecode, &i);
    RewrittenOp rewritten = rewriteOperation(function, op);
    if (rewritten.needs_inline_cache) {
      // Replace opcode arg with a cache index and zero EXTENDED_ARG args.
      CHECK(cache < 256, "more than 256 entries may require bytecode resizing");
      for (word j = begin; j < i - 2; j += 2) {
        bytecode.byteAtPut(j, static_cast<byte>(Bytecode::EXTENDED_ARG));
        bytecode.byteAtPut(j + 1, 0);
      }
      bytecode.byteAtPut(i - 2, static_cast<byte>(rewritten.bc));
      bytecode.byteAtPut(i - 1, static_cast<byte>(cache));

      // Remember original arg.
      original_arguments.atPut(cache, SmallInt::fromWord(rewritten.arg));
      cache++;
    } else if (rewritten.arg != op.arg || rewritten.bc != op.bc) {
      CHECK(rewritten.arg < 256,
            "more than 256 entries may require bytecode resizing");
      for (word j = begin; j < i - 2; j += 2) {
        bytecode.byteAtPut(j, static_cast<byte>(Bytecode::EXTENDED_ARG));
        bytecode.byteAtPut(j + 1, 0);
      }
      bytecode.byteAtPut(i - 2, static_cast<byte>(rewritten.bc));
      bytecode.byteAtPut(i - 1, static_cast<byte>(rewritten.arg));
    }
  }

  // TODO(T45428069): Remove this once the compiler starts emitting the opcodes.
  rewriteZeroArgMethodCallsUsingLoadMethodCached(bytecode);
  function.setCaches(runtime->newTuple(num_caches * kIcPointersPerCache));
  function.setOriginalArguments(*original_arguments);
}

void icUpdate(RawTuple caches, word index, LayoutId layout_id,
              RawObject value) {
  RawSmallInt key = SmallInt::fromWord(static_cast<word>(layout_id));
  RawObject entry_key = NoneType::object();
  for (word i = index * kIcPointersPerCache, end = i + kIcPointersPerCache;
       i < end; i += kIcPointersPerEntry) {
    entry_key = caches.at(i + kIcEntryKeyOffset);
    if (entry_key.isNoneType() || entry_key == key) {
      caches.atPut(i + kIcEntryKeyOffset, key);
      caches.atPut(i + kIcEntryValueOffset, value);
      return;
    }
  }
}

static bool dependentIncluded(RawObject dependent, RawObject link) {
  for (; !link.isNoneType(); link = WeakLink::cast(link).next()) {
    if (WeakLink::cast(link).referent() == dependent) {
      return true;
    }
  }
  return false;
}

bool icInsertDependentToValueCellDependencyLink(Thread* thread,
                                                const Object& dependent,
                                                const ValueCell& value_cell) {
  HandleScope scope(thread);
  Object link(&scope, value_cell.dependencyLink());
  if (dependentIncluded(*dependent, *link)) {
    // Already included.
    return false;
  }
  Object none(&scope, NoneType::object());
  WeakLink new_link(
      &scope, thread->runtime()->newWeakLink(thread, dependent, none, link));
  if (link.isWeakLink()) {
    WeakLink::cast(*link).setPrev(*new_link);
  }
  value_cell.setDependencyLink(*new_link);
  return true;
}

void icInsertDependencyForTypeLookupInMro(Thread* thread, const Type& type,
                                          const Object& name_str,
                                          const Object& dependent) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple mro(&scope, type.mro());
  NoneType none(&scope, NoneType::object());
  for (word i = 0; i < mro.length(); i++) {
    Type mro_type(&scope, mro.at(i));
    Dict dict(&scope, mro_type.dict());
    // TODO(T46428372): Consider using a specialized dict lookup to avoid 2
    // probings.
    Object result(&scope, runtime->dictAt(thread, dict, name_str));
    DCHECK(result.isErrorNotFound() || result.isValueCell(),
           "value must be ValueCell if found");
    if (result.isErrorNotFound()) {
      result = runtime->dictAtPutInValueCell(thread, dict, name_str, none);
      ValueCell::cast(*result).makePlaceholder();
    }
    ValueCell value_cell(&scope, *result);
    icInsertDependentToValueCellDependencyLink(thread, dependent, value_cell);
    if (!value_cell.isPlaceholder()) {
      // Attribute lookup terminates here. Therefore, no dependency tracking is
      // needed afterwards.
      return;
    }
  }
}

void icDeleteDependentInValueCell(Thread* thread, const ValueCell& value_cell,
                                  const Object& dependent) {
  HandleScope scope(thread);
  Object link(&scope, value_cell.dependencyLink());
  Object prev(&scope, NoneType::object());
  while (!link.isNoneType()) {
    WeakLink weak_link(&scope, *link);
    if (weak_link.referent() == *dependent) {
      if (weak_link.next().isWeakLink()) {
        WeakLink::cast(weak_link.next()).setPrev(*prev);
      }
      if (prev.isWeakLink()) {
        WeakLink::cast(*prev).setNext(weak_link.next());
      } else {
        value_cell.setDependencyLink(weak_link.next());
      }
      break;
    }
    prev = *link;
    link = weak_link.next();
  }
}

void icDeleteDependentInMro(Thread* thread, const Object& attribute_name,
                            const Tuple& mro, const Object& dependent) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  for (word i = 0; i < mro.length(); ++i) {
    Type mro_type(&scope, mro.at(i));
    Dict dict(&scope, mro_type.dict());
    Object result(&scope, runtime->dictAt(thread, dict, attribute_name));
    if (!result.isValueCell()) {
      // No ValueCell found, implying no dependencies in this type dict and
      // above.
      break;
    }
    ValueCell value_cell(&scope, *result);
    icDeleteDependentInValueCell(thread, value_cell, dependent);
  }
}

static bool isTypeInMro(RawType type, RawTuple mro) {
  for (word i = 0; i < mro.length(); ++i) {
    if (type == mro.at(i)) {
      return true;
    }
  }
  return false;
}

void icDeleteCacheForTypeAttrInDependent(Thread* thread, const Type& type,
                                         const Str& attribute_name,
                                         bool data_descriptor,
                                         const Function& dependent) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple names(&scope, Code::cast(dependent.code()).names());
  Tuple caches(&scope, dependent.caches());
  // Scan through all attribute caches and delete caches whose mro matches the
  // given type & attribute_name.
  MutableBytes bytecode(&scope, dependent.rewrittenBytecode());
  word bytecode_length = bytecode.length();
  for (word i = 0; i < bytecode_length;) {
    BytecodeOp op = nextBytecodeOp(bytecode, &i);
    if (op.bc != LOAD_ATTR_CACHED && op.bc != LOAD_METHOD_CACHED &&
        op.bc != STORE_ATTR_CACHED) {
      continue;
    }
    for (word j = op.arg * kIcPointersPerCache, end = j + kIcPointersPerCache;
         j < end; j += kIcPointersPerEntry) {
      Object cache_key(&scope, caches.at(j + kIcEntryKeyOffset));
      // Unfilled cache.
      if (cache_key.isNoneType()) {
        continue;
      }
      // Attribute name doesn't match.
      if (!attribute_name.equals(names.at(icOriginalArg(*dependent, op.arg)))) {
        continue;
      }
      // A cache offset has a higher precedence than a non-data descriptor.
      if (caches.at(j + kIcEntryValueOffset).isSmallInt() && !data_descriptor) {
        continue;
      }
      Type cache_type(&scope, runtime->typeAt(static_cast<LayoutId>(
                                  SmallInt::cast(*cache_key).value())));
      Tuple cache_mro(&scope, cache_type.mro());
      // The type of the attribute being modified may not not appear in the mro
      // of the type of the cache. See the following example.
      //
      // class A:
      //   def foo(self): return 100
      // class B:
      //   def foo(self): return 200
      //
      // a = A()
      // b = B()
      // tmp0 = a.foo (*)
      // tmp1 = b.foo (+)
      // A.foo = lambda self: -100 (@)
      //
      // In the example above, (*) caches A.foo, (=) caches B.foo.
      // (@) should only invalidate (*) not, (+) although they share the same
      // attribute name since their types are unrelated.
      // TODO(djang): Delete caches for type attribute values (function) only if
      // naming shadowing actually happens. For instance offset caches, this
      // check is enough since creating a data descriptor type attribute shadows
      // them.
      if (!isTypeInMro(*type, *cache_mro)) {
        continue;
      }

      caches.atPut(j + kIcEntryKeyOffset, NoneType::object());
      caches.atPut(j + kIcEntryValueOffset, NoneType::object());

      // Delete all direct/indirect dependencies from the deleted cache to
      // dependent since such dependencies are gone now.
      icDeleteDependentInMro(thread, attribute_name, cache_mro, dependent);
    }
  }
}

void icInvalidateCachesForTypeAttr(Thread* thread, const Type& type,
                                   const Str& attribute_name,
                                   bool data_descriptor) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Dict dict(&scope, type.dict());
  Object value(&scope, runtime->dictAt(thread, dict, attribute_name));
  if (value.isErrorNotFound()) {
    return;
  }
  DCHECK(value.isValueCell(), "value must be ValueCell");
  ValueCell type_attr_cell(&scope, *value);
  // Delete caches for attribute_name to be shadowed by the type[attribute_name]
  // change in all dependents that depend on the attribute being updated.
  Object link(&scope, type_attr_cell.dependencyLink());
  while (!link.isNoneType()) {
    Function dependent(&scope, WeakLink::cast(*link).referent());
    // Capturing the next node in case the current node is deleted by
    // icDeleteCacheForTypeAttrInDependent
    link = WeakLink::cast(*link).next();
    icDeleteCacheForTypeAttrInDependent(thread, type, attribute_name,
                                        data_descriptor, dependent);
  }

  // In case data_descriptor is true, we shouldn't see any dependents after
  // caching invalidation.
  DCHECK(!data_descriptor || type_attr_cell.dependencyLink().isNoneType(),
         "dependencyLink must be None if data_descriptor is true");
}

void icUpdateBinop(RawTuple caches, word index, LayoutId left_layout_id,
                   LayoutId right_layout_id, RawObject value,
                   IcBinopFlags flags) {
  word key_high_bits = static_cast<word>(left_layout_id)
                           << Header::kLayoutIdBits |
                       static_cast<word>(right_layout_id);
  RawObject entry_key = NoneType::object();
  for (word i = index * kIcPointersPerCache, end = i + kIcPointersPerCache;
       i < end; i += kIcPointersPerEntry) {
    entry_key = caches.at(i + kIcEntryKeyOffset);
    if (entry_key.isNoneType() ||
        SmallInt::cast(entry_key).value() >> kBitsPerByte == key_high_bits) {
      caches.atPut(i + kIcEntryKeyOffset,
                   SmallInt::fromWord(key_high_bits << kBitsPerByte |
                                      static_cast<word>(flags)));
      caches.atPut(i + kIcEntryValueOffset, value);
    }
  }
}

void icUpdateGlobalVar(Thread* thread, const Function& function, word index,
                       const ValueCell& value_cell) {
  HandleScope scope(thread);
  Tuple caches(&scope, function.caches());
  DCHECK(caches.at(index).isNoneType(),
         "cache entry must be empty one before update");
  icInsertDependentToValueCellDependencyLink(thread, function, value_cell);
  caches.atPut(index, *value_cell);

  // Update all global variable access to the cached value in the function.
  MutableBytes bytecode(&scope, function.rewrittenBytecode());
  word bytecode_length = bytecode.length();
  byte target_arg = static_cast<byte>(index);
  for (word i = 0; i < bytecode_length;) {
    BytecodeOp op = nextBytecodeOp(bytecode, &i);
    if (op.arg != target_arg) {
      continue;
    }
    if (op.bc == LOAD_GLOBAL) {
      bytecode.byteAtPut(i - 2, LOAD_GLOBAL_CACHED);
    } else if (op.bc == STORE_GLOBAL) {
      bytecode.byteAtPut(i - 2, STORE_GLOBAL_CACHED);
    }
  }
}

void icInvalidateGlobalVar(Thread* thread, const ValueCell& value_cell) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Tuple caches(&scope, runtime->emptyTuple());
  Object referent(&scope, NoneType::object());
  Object function(&scope, NoneType::object());
  Object link(&scope, value_cell.dependencyLink());
  MutableBytes bytecode(&scope, runtime->newMutableBytesUninitialized(0));
  while (!link.isNoneType()) {
    DCHECK(link.isWeakLink(), "ValuCell.dependenyLink must be a WeakLink");
    referent = WeakLink::cast(*link).referent();
    if (referent.isNoneType()) {
      // The function got deallocated.
      link = WeakLink::cast(*link).next();
      continue;
    }
    DCHECK(referent.isFunction(),
           "dependencyLink's payload must be a function");
    function = *referent;
    word names_length =
        Tuple::cast(Code::cast(Function::cast(*function).code()).names())
            .length();
    DCHECK(names_length < 256,
           "more than 256 global names may require bytecode stretching");

    // Empty the cache.
    caches = Function::cast(*function).caches();
    word name_index_found = -1;
    for (word i = 0; i < names_length; i++) {
      if (caches.at(i) == *value_cell) {
        caches.atPut(i, NoneType::object());
        name_index_found = i;
        break;
      }
    }
    DCHECK(name_index_found >= 0,
           "a dependent function must have cached the value");

    bytecode = Function::cast(*function).rewrittenBytecode();
    word bytecode_length = bytecode.length();
    for (word i = 0; i < bytecode_length;) {
      BytecodeOp op = nextBytecodeOp(bytecode, &i);
      Bytecode original_bc = op.bc;
      switch (op.bc) {
        case LOAD_GLOBAL_CACHED:
          original_bc = LOAD_GLOBAL;
          break;
        case STORE_GLOBAL_CACHED:
          original_bc = STORE_GLOBAL;
          break;
        default:
          break;
      }
      if (op.bc != original_bc && op.arg == name_index_found) {
        bytecode.byteAtPut(i - 2, original_bc);
      }
    }
    link = WeakLink::cast(*link).next();
  }
  value_cell.setDependencyLink(NoneType::object());
}

}  // namespace python
