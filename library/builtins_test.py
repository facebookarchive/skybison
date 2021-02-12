#!/usr/bin/env python3
import ast
import builtins
import contextlib
import copy
import errno
import math
import sys
import types
import typing
import unittest
import warnings
from collections import namedtuple
from types import SimpleNamespace
from unittest.mock import Mock

from test_support import pyro_only, supports_38_feature


Py_TPFLAGS_HEAPTYPE = 1 << 9
Py_TPFLAGS_BASETYPE = 1 << 10
Py_TPFLAGS_READY = 1 << 12
Py_TPFLAGS_READYING = 1 << 13
Py_TPFLAGS_IS_ABSTRACT = 1 << 20
Py_TPFLAGS_LONG_SUBCLASS = 1 << 24
Py_TPFLAGS_LIST_SUBCLASS = 1 << 25
Py_TPFLAGS_TUPLE_SUBCLASS = 1 << 26
Py_TPFLAGS_BYTES_SUBCLASS = 1 << 27
Py_TPFLAGS_UNICODE_SUBCLASS = 1 << 28
Py_TPFLAGS_DICT_SUBCLASS = 1 << 29
Py_TPFLAGS_BASE_EXC_SUBCLASS = 1 << 30
Py_TPFLAGS_TYPE_SUBCLASS = 1 << 31


try:
    from builtins import _number_check, instance_proxy, _non_heaptype

    from _builtins import _async_generator_op_iter_get_state, _gc
except ImportError:
    pass


class Example:
    pass


class Forward:
    pass


class Super:
    pass


class Child(Super):
    pass


class AbsTests(unittest.TestCase):
    def test_abs_calls_dunder_abs(self):
        class C:
            __abs__ = Mock(name="__abs__", return_value=3)

        abs(C())
        C.__abs__.assert_called_once()

    def test_abs_with_non_callable_dunder_abs_raises_type_error(self):
        class C:
            __abs__ = None

        instance = C()
        with self.assertRaisesRegex(TypeError, "'NoneType' object is not callable"):
            abs(instance)

    def test_abs_with_nan_returns_nan(self):
        nan = float("nan")
        result = abs(nan)
        self.assertIsNot(result, nan)
        self.assertNotEqual(result, nan)
        self.assertTrue(math.isnan(result))


class AsyncGeneratorTests(unittest.TestCase):
    def test_dunder_aiter_with_non_async_generator_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func()).__aiter__(None)

    def test_dunder_anext_with_non_async_generator_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func()).__anext__(None)

    def test_dunder_del_with_non_async_generator_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func()).__del__(None)

    def test_dunder_repr_with_non_async_generator_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func()).__repr__(None)

    def test_aclose_with_non_async_generator_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func()).aclose(None)

    def test_asend_with_non_async_generator_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func()).asend(None, None)

    def test_athrow_with_non_async_generator_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func()).athrow(None, None, None, None)

    def test_dunder_aiter_returns_self(self):
        async def async_gen_func():
            yield

        async_gen = async_gen_func()

        self.assertIs(async_gen.__aiter__(), async_gen)

    def test_dunder_anext_returns_iterable_over_states_of_trivial_async_gen_func(self):
        async def async_gen_func():
            yield 1

        with self.assertRaises(StopIteration) as exc:
            next(iter(async_gen_func().__anext__()))
        self.assertEqual(exc.exception.value, 1)

    def test_dunder_anext_runs_firstiter_hook_exactly_once(self):
        firstiter_ran = None

        def firstiter_hook(async_gen_inst):
            nonlocal firstiter_ran
            firstiter_ran = async_gen_inst

        sys.set_asyncgen_hooks(firstiter=firstiter_hook)

        async def async_gen_func():
            yield

        async_gen = async_gen_func()
        async_gen.__anext__()
        self.assertIs(firstiter_ran, async_gen)

        firstiter_ran = None
        async_gen.__anext__()
        self.assertIsNone(firstiter_ran)

    def test_dunder_anext_sets_finalizer(self):
        finalizer_ran = None

        def finalizer_hook(async_gen_inst):
            nonlocal finalizer_ran
            finalizer_ran = async_gen_inst

        sys.set_asyncgen_hooks(finalizer=finalizer_hook)

        async def async_gen_func():
            yield

        async_gen = async_gen_func()
        async_gen.__anext__()
        async_gen.__del__()
        self.assertIs(finalizer_ran, async_gen)

    def test_dunder_repr(self):
        async def async_gen_func():
            yield

        async_gen_obj = async_gen_func()

        self.assertTrue(
            async_gen_obj.__repr__().startswith(
                "<async_generator object AsyncGeneratorTests.test_dunder_repr."
                "<locals>.async_gen_func at "
            )
        )

    def test_aclose_runs_firstiter_hook(self):
        firstiter_ran = None

        def firstiter_hook(async_gen_inst):
            nonlocal firstiter_ran
            firstiter_ran = async_gen_inst

        sys.set_asyncgen_hooks(firstiter=firstiter_hook)

        async def async_gen_func():
            yield

        async_gen = async_gen_func()
        async_gen.aclose()
        self.assertIs(firstiter_ran, async_gen)

    def test_aclose_sets_finalizer(self):
        finalizer_ran = None

        def finalizer_hook(async_gen_inst):
            nonlocal finalizer_ran
            finalizer_ran = async_gen_inst

        sys.set_asyncgen_hooks(finalizer=finalizer_hook)

        async def async_gen_func():
            yield

        async_gen = async_gen_func()
        async_gen.aclose()
        async_gen.__del__()
        self.assertIs(finalizer_ran, async_gen)

    def test_aclose_raises_generator_exit_inside_generator(self):
        saw_generator_exit = False

        async def f():
            nonlocal saw_generator_exit
            try:
                yield 1
            except GeneratorExit:
                saw_generator_exit = True

        # Create async_generator for f()
        async_gen = f()

        # Advance generator to 'yield 1' to put us inside a try-except
        # block from which we can catch aclose's GeneratorExit.
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        try:
            async_gen.aclose().send(None)
        except StopIteration:
            pass

        self.assertTrue(saw_generator_exit)

    def test_aclose_prevents_further_iteration(self):
        async def f():
            yield 1

        # Create async_generator for f()
        async_gen = f()

        # Perform the aclose operation
        with self.assertRaises(StopIteration) as exc:
            async_gen.aclose().send(None)
        # Note '1' is not yielded because we closed before execution begun
        self.assertIsNone(exc.exception.value)

        # Further attempts to send into the generator result in 'StopAsyncIteration'
        # which means the iterator has no more results. Again '1' is never yielded.
        with self.assertRaises(StopAsyncIteration):
            async_gen.asend(None).send(None)

    def test_asend_runs_firstiter_hook_exactly_once(self):
        firstiter_ran = None

        def firstiter_hook(async_gen_inst):
            nonlocal firstiter_ran
            firstiter_ran = async_gen_inst

        sys.set_asyncgen_hooks(firstiter=firstiter_hook)

        async def async_gen_func():
            yield
            yield

        async_gen = async_gen_func()
        async_gen.asend(None)
        self.assertIs(firstiter_ran, async_gen)

        firstiter_ran = None
        async_gen.asend(None)
        self.assertIs(firstiter_ran, None)

    def test_asend_sets_finalizer(self):
        finalizer_ran = None

        def finalizer_hook(async_gen_inst):
            nonlocal finalizer_ran
            finalizer_ran = async_gen_inst

        sys.set_asyncgen_hooks(finalizer=finalizer_hook)

        async def async_gen_func():
            yield

        async_gen = async_gen_func()
        async_gen.asend(None)
        async_gen.__del__()
        self.assertIs(finalizer_ran, async_gen)

    def test_asend_returns_iterable_over_states_of_no_op_async_gen_func(self):
        async def async_gen_func():
            if False:
                yield

        async_gen_obj = async_gen_func()

        # Call next() on no-op function advances us to the final state.
        # StopAsyncIteration indicates there are no further results.
        with self.assertRaises(StopAsyncIteration):
            next(iter(async_gen_obj.asend(None)))

    def test_asend_returns_iterables_over_states_of_yielding_async_gen_func(self):
        async def async_gen_func():
            yield 1

        async_gen_obj = async_gen_func()

        # Calling next advances us to the first state: yielding 1.
        with self.assertRaises(StopIteration) as exc:
            next(iter(async_gen_obj.asend(None)))
        self.assertEqual(exc.exception.value, 1)

        # Calling next now advances us to the final state.
        # StopAsyncIteration indicates there are no further results.
        with self.assertRaises(StopAsyncIteration):
            next(iter(async_gen_obj.asend(None)))

    def test_using_asend_to_inject_a_value_into_generator(self):
        async def async_gen_func():
            yield (yield)

        async_gen_obj = async_gen_func()

        # First next() call runs generator until the first yield.
        with self.assertRaises(StopIteration) as exc:
            next(async_gen_obj.asend(None))
        self.assertIsNone(exc.exception.value)

        # Send value into generator with a value to advance to the next
        # stopping state which will yield the value we sent in.
        with self.assertRaises(StopIteration) as exc:
            next(async_gen_obj.asend(1))
        self.assertEqual(exc.exception.value, 1)

    def test_athrow_runs_firstiter_hook(self):
        firstiter_ran = None

        def firstiter_hook(async_gen_inst):
            nonlocal firstiter_ran
            firstiter_ran = async_gen_inst

        sys.set_asyncgen_hooks(firstiter=firstiter_hook)

        async def async_gen_func():
            yield

        async_gen = async_gen_func()
        async_gen.athrow(None)
        self.assertIs(firstiter_ran, async_gen)

    def test_athrow_sets_finalizer(self):
        finalizer_ran = None

        def finalizer_hook(async_gen_inst):
            nonlocal finalizer_ran
            finalizer_ran = async_gen_inst

        sys.set_asyncgen_hooks(finalizer=finalizer_hook)

        async def async_gen_func():
            yield

        async_gen = async_gen_func()
        async_gen.athrow(None)
        async_gen.__del__()
        self.assertIs(finalizer_ran, async_gen)

    def test_athrow_raises_inside_generator(self):
        async def f():
            try:
                yield 1
            except ValueError:
                yield 2

        async_gen = f()

        # Advance generator to 'yield 1' so we can catch the ValueError.
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        with self.assertRaises(StopIteration) as exc:
            async_gen.athrow(ValueError).send(None)
        self.assertEqual(exc.exception.value, 2)

    def test_athrow_uncaught_in_generator_propagates_out(self):
        async def f():
            yield 1

        with self.assertRaises(ValueError) as exc:
            f().athrow(ValueError, 1).send(None)
        self.assertEqual(exc.exception.args, (1,))

    def test_async_generator_is_an_asynchronous_iterable(self):
        # Inline coments from PEP-492 definition of "asynchronous
        # iterable":
        # https://www.python.org/dev/peps/pep-0492/#asynchronous-iterators-and-async-for

        async def async_gen_func():
            yield

        # 1. [An asynchronous iterable] must implement an __aiter__ method ...
        async_gen_obj = async_gen_func()
        self.assertTrue(hasattr(async_gen_obj, "__aiter__"))

        # ... returning an asynchronous iterator object.
        # 2. An asynchronous iterator object must implement an __anext__
        # method ...
        async_it_obj = async_gen_obj.__aiter__()
        self.assertTrue(hasattr(async_it_obj, "__anext__"))

        # ... returning an awaitable.
        awaitable_obj = async_it_obj.__anext__()
        self.assertTrue(hasattr(awaitable_obj, "__await__"))

        # 3. To stop iteration __anext__ must raise a StopAsyncIteration
        # exception.
        iterable_obj = awaitable_obj.__await__()
        with self.assertRaises(StopIteration) as exc:
            next(iter(iterable_obj))
        self.assertIsNone(exc.exception.value)
        with self.assertRaises(StopAsyncIteration):
            next(iter(async_gen_obj.__aiter__().__anext__().__await__()))

    def test_async_generator_explicitly_throws_stop_async_iteration_raise_runtime_error(
        self,
    ):
        async def f():
            raise StopIteration
            yield

        with self.assertRaises(RuntimeError):
            f().asend(None).send(None)


class AsyncGeneratorAcloseTests(unittest.TestCase):
    _STATE_MAP = {"_STATE_INIT": 0, "_STATE_ITER": 1, "_STATE_CLOSED": 2}

    def _assertOpIterState(self, op_iter, state_str):
        if sys.implementation.name == "pyro":
            state = _async_generator_op_iter_get_state(op_iter)
            self.assertEqual(state, AsyncGeneratorAcloseTests._STATE_MAP[state_str])

    def test_dunder_await_with_non_async_generator_aclose_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().aclose()).__await__(None)

    def test_dunder_iter_with_non_async_generator_aclose_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().aclose()).__iter__(None)

    def test_dunder_next_with_non_async_generator_aclose_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().aclose()).__next__(None)

    def test_close_with_non_async_generator_aclose_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().aclose()).close(None)

    def test_send_with_non_async_generator_aclose_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().aclose()).send(None, None)

    def test_throw_with_non_async_generator_aclose_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().aclose()).throw(None, None, None, None)

    def test_dunder_await_returns_self(self):
        async def f():
            yield 1

        op_iter = f().aclose()

        self.assertIs(op_iter.__await__(), op_iter)

    def test_dunder_iter_returns_self(self):
        async def f():
            yield

        op_iter = f().aclose()

        self.assertIs(op_iter.__iter__(), op_iter)

    def test_is_awaitable(self):
        async def f():
            yield 1

        op_iter = f().aclose()

        async def awaiter(awaitable):
            return await awaitable

        try:
            # This would raise TypeError if op_iter were not awaitable.
            awaiter(op_iter).send(None)
        except StopIteration:
            pass

    def test_is_iterable(self):
        async def f():
            yield 1

        op_iter = f().aclose()

        with self.assertRaises(StopIteration) as exc:
            next(iter(op_iter))
        self.assertIsNone(exc.exception.value)

    @pyro_only
    def test_new_instance_starts_in_init_state(self):
        async def f():
            yield 1

        op_iter = f().aclose()

        self._assertOpIterState(op_iter, "_STATE_INIT")

    @pyro_only
    def test_close_moves_into_closed_state(self):
        async def f():
            yield 1

        # Make aclose op_iter
        op_iter = f().aclose()

        op_iter.close()
        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_send_state_closed_raises_stop_iteration_state_closed(self):
        async def f():
            yield 1

        # Make aclose op_iter in closed state
        op_iter = f().aclose()
        op_iter.close()

        with self.assertRaises(StopIteration) as exc:
            op_iter.send(None)
        self.assertIsNone(exc.exception.value)

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_send_state_init_arg_is_not_none_raises_runtime_error_state_init(self):
        async def f():
            yield 1

        # Make aclose op_iter
        op_iter = f().aclose()

        with self.assertRaises(RuntimeError):
            op_iter.send(1)

        self._assertOpIterState(op_iter, "_STATE_INIT")

    def test_send_state_init_arg_is_none_generator_raises_stop_async_iteration_raises_stop_iteration_state_closed(  # noqa: B950
        self,
    ):
        async def f():
            try:
                yield 1
            except GeneratorExit:
                # Returning causes StopAsyncIteration to be raised
                return

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 1'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make aclose op_iter which is primted to run inside try-except block
        op_iter = async_gen.aclose()

        with self.assertRaises(StopIteration) as exc:
            op_iter.send(None)
        self.assertIsNone(exc.exception.value)

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_send_state_init_arg_is_none_generator_raises_generator_exit_raises_stop_iteration_state_closed(  # noqa: B950
        self,
    ):
        async def f():
            yield 1

        # Make aclose op_iter
        op_iter = f().aclose()

        with self.assertRaises(StopIteration) as exc:
            # The aclose operation implicitly raises GeneratorExit in the generator
            op_iter.send(None)
        self.assertIsNone(exc.exception.value)

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_send_state_init_arg_is_none_generator_yields_raises_runtime_error_state_iter(  # noqa: B950
        self,
    ):
        async def f():
            try:
                yield 1
            except GeneratorExit:
                yield 2

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 1'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make aclose op_iter which is primted to run inside try-except block
        op_iter = async_gen.aclose()

        with self.assertRaises(RuntimeError):
            op_iter.send(None)

        self._assertOpIterState(op_iter, "_STATE_ITER")

    def test_send_state_init_arg_is_none_generator_awaits_returns_awaitable_yielded_value_state_iter(  # noqa: B950
        self,
    ):
        async def f():
            class Awaitable:
                def __await__(self):
                    yield 1

            try:
                yield 2
            except GeneratorExit:
                await Awaitable()

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 2'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make aclose op_iter which is primted to run inside try-except block
        op_iter = async_gen.aclose()

        self.assertEqual(op_iter.send(None), 1)

        self._assertOpIterState(op_iter, "_STATE_ITER")

    def test_send_state_iter_arg_is_not_none_arg_sent_into_generator(self):
        sent_value = None

        async def f():
            class Awaitable:
                def __await__(self):
                    nonlocal sent_value
                    sent_value = yield 1

            try:
                yield 2
            except GeneratorExit:
                await Awaitable()

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 2'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make aclose op_iter which is primted to run inside try-except block
        op_iter = async_gen.aclose()

        # Executing the aclose operation which should take us to the ITER state
        # as we run 'await Awaitable()'
        op_iter.send(None)

        # Send the value 3 in which will appear at the yield in the awaitable
        # and then finish the generator with a StopIteration.
        try:
            op_iter.send(3)
        except StopIteration:
            pass
        self.assertEqual(sent_value, 3)

    def test_send_state_iter_arg_is_none_generator_raises_generator_exit_raises_stop_iteration_state_closed(  # noqa: B950
        self,
    ):
        async def f():
            class Awaitable:
                def __await__(self):
                    yield 1
                    raise GeneratorExit

            try:
                yield 2
            except GeneratorExit:
                await Awaitable()

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 2'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make aclose op_iter which is primted to run inside try-except block
        op_iter = async_gen.aclose()

        # Executing the aclose operation which should take us to the ITER state
        # as we run 'await Awaitable()'
        op_iter.send(None)

        with self.assertRaises(StopIteration) as exc:
            op_iter.send(None)
        self.assertIsNone(exc.exception.value)

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_send_state_iter_arg_is_none_generator_raises_stop_async_iteration_raises_stop_iteration_state_closed(  # noqa: B950
        self,
    ):
        async def f():
            class Awaitable:
                def __await__(self):
                    yield 1

            try:
                yield 2
            except GeneratorExit:
                await Awaitable()
            # Terminating here raises StopAsyncIteration

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 2'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make aclose op_iter which is primted to run inside try-except block
        op_iter = async_gen.aclose()

        # Executing the aclose operation which should take us to the ITER state
        # as we run 'await Awaitable()'
        op_iter.send(None)

        with self.assertRaises(StopIteration) as exc:
            op_iter.send(None)
        self.assertIsNone(exc.exception.value)

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_send_state_iter_arg_is_none_generator_yields_raises_runtime_error_state_iter(  # noqa: B950
        self,
    ):
        async def f():
            class Awaitable:
                def __await__(self):
                    yield 1

            try:
                yield 2
            except GeneratorExit:
                await Awaitable()
                yield 3

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 2'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make aclose op_iter which is primted to run inside try-except block
        op_iter = async_gen.aclose()

        # Executing the aclose operation which should take us to the ITER state
        # as we run 'await Awaitable()'
        op_iter.send(None)

        with self.assertRaises(RuntimeError):
            op_iter.send(None)

        self._assertOpIterState(op_iter, "_STATE_ITER")

    def test_send_state_iter_arg_is_none_generator_awaits_returns_awaitable_yielded_value_state_iter(  # noqa: B950
        self,
    ):
        async def f():
            class Awaitable:
                def __init__(self, yield_value):
                    self._yield_value = yield_value

                def __await__(self):
                    yield self._yield_value

            try:
                yield 1
            except GeneratorExit:
                await Awaitable(2)
                await Awaitable(3)

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 1'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make aclose op_iter which is primted to run inside try-except block
        op_iter = async_gen.aclose()

        # Executing the aclose operation which should take us to the ITER state
        # as we run 'await Awaitable()'
        op_iter.send(None)

        self.assertEqual(op_iter.send(None), 3)

        self._assertOpIterState(op_iter, "_STATE_ITER")

    def test_throw_state_closed_raises_stop_iteration_state_closed(self):
        async def f():
            yield 1

        # Make aclose op_iter in closed state
        op_iter = f().aclose()
        op_iter.close()

        with self.assertRaises(StopIteration) as exc:
            op_iter.throw(None, None, None)
        self.assertIsNone(exc.exception.value)

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_throw_state_init_raises_runtime_error_state_init(self):
        async def f():
            yield 1

        # Make aclose op_iter
        op_iter = f().aclose()

        with self.assertRaises(RuntimeError):
            op_iter.throw(None, None, None)

        self._assertOpIterState(op_iter, "_STATE_INIT")

    def test_throw_state_iter_generator_raise_propagates_state_iter(self):
        async def f():
            class Awaitable:
                def __await__(self):
                    yield 1

            try:
                yield 2
            except GeneratorExit:
                await Awaitable()

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 2'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make aclose op_iter which is primted to run inside try-except block
        op_iter = async_gen.aclose()

        # Executing the aclose operation which should take us to the ITER state
        # as we run 'await Awaitable()'
        op_iter.send(None)

        with self.assertRaises(ValueError) as exc:
            op_iter.throw(ValueError, 3, None)
        self.assertEqual(exc.exception.args, (3,))

        self._assertOpIterState(op_iter, "_STATE_ITER")

    def test_throw_state_iter_generator_yields_raises_runtime_error_state_iter(self):
        async def f():
            class Awaitable:
                def __await__(self):
                    yield 1

            try:
                yield 2
            except GeneratorExit:
                try:
                    await Awaitable()
                except ValueError:
                    yield 3

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 2'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make aclose op_iter which is primted to run inside try-except block
        op_iter = async_gen.aclose()

        # Executing the aclose operation which should take us to the ITER state
        # as we run 'await Awaitable()'
        op_iter.send(None)

        with self.assertRaises(RuntimeError):
            op_iter.throw(ValueError, 4, None)

        self._assertOpIterState(op_iter, "_STATE_ITER")

    def test_throw_state_iter_generator_awaits_returns_awaitable_yielded_value_state_iter(  # noqa: B950
        self,
    ):
        async def f():
            class Awaitable:
                def __init__(self, yield_value):
                    self._yield_value = yield_value

                def __await__(self):
                    yield self._yield_value

            try:
                yield 1
            except GeneratorExit:
                try:
                    await Awaitable(2)
                except ValueError:
                    await Awaitable(3)

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 2'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make aclose op_iter which is primted to run inside try-except block
        op_iter = async_gen.aclose()

        # Executing the aclose operation which should take us to the ITER state
        # as we run 'await Awaitable()'
        op_iter.send(None)

        self.assertEqual(op_iter.throw(ValueError, 4, None), 3)

        self._assertOpIterState(op_iter, "_STATE_ITER")


class AsyncGeneratorAsendTests(unittest.TestCase):
    _STATE_MAP = {"_STATE_INIT": 0, "_STATE_ITER": 1, "_STATE_CLOSED": 2}

    def _assertOpIterState(self, op_iter, state_str):
        if sys.implementation.name == "pyro":
            state = _async_generator_op_iter_get_state(op_iter)
            self.assertEqual(state, AsyncGeneratorAsendTests._STATE_MAP[state_str])

    def test_dunder_await_with_non_async_generator_asend_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().asend(None)).__await__(None)

    def test_dunder_iter_with_non_async_generator_asend_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().asend(None)).__iter__(None)

    def test_dunder_next_with_non_async_generator_asend_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().asend(None)).__next__(None)

    def test_close_with_non_async_generator_asend_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().asend(None)).close(None)

    def test_send_with_non_async_generator_asend_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().asend(None)).send(None, None)

    def test_throw_with_non_async_generator_asend_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().asend(None)).throw(None, None, None, None)

    def test_dunder_await_returns_self(self):
        async def f():
            yield 1

        op_iter = f().asend(None)

        self.assertIs(op_iter.__await__(), op_iter)

    def test_dunder_iter_returns_self(self):
        async def f():
            yield 1

        op_iter = f().asend(None)

        self.assertIs(op_iter.__iter__(), op_iter)

    def test_is_awaitable(self):
        async def f():
            yield 1

        op_iter = f().asend(None)

        async def awaiter(awaitable):
            return await awaitable

        try:
            # This would raise TypeError if op_iter were not awaitable.
            awaiter(op_iter).send(None)
        except StopIteration:
            pass

    def test_is_iterable(self):
        async def f():
            yield 1

        op_iter = f().asend(None)

        with self.assertRaises(StopIteration) as exc:
            next(iter(op_iter))
        self.assertEqual(exc.exception.value, 1)

    @pyro_only
    def test_new_instance_starts_in_init_state(self):
        async def f():
            yield 1

        op_iter = f().asend(None)

        self._assertOpIterState(op_iter, "_STATE_INIT")

    @pyro_only
    def test_close_moves_into_closed_state(self):
        async def f():
            yield 1

        # Make asend op_iter
        op_iter = f().asend(None)

        op_iter.close()
        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_send_state_closed_raises_stop_iteration_state_closed(self):
        async def f():
            yield 1

        # Make asend op_iter in closed state
        op_iter = f().asend(None)
        op_iter.close()

        with self.assertRaises(StopIteration) as exc:
            op_iter.send(None)
        self.assertIsNone(exc.exception.value)

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_send_state_init_arg_is_none_sends_primed_value_into_generator(self):
        sent_value = 1

        async def f():
            nonlocal sent_value
            sent_value = yield 2

        async_gen = f()

        # Advance generator through 'yield 2' to waiting to receive a value.
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        try:
            # Setup an asend op iter with a "primed" value of 3
            async_gen.asend(3).send(None)
        except StopAsyncIteration:
            pass
        self.assertEqual(sent_value, 3)

    def test_send_state_init_arg_is_set_sends_arg_into_generator(self):
        sent_value = 1

        async def f():
            nonlocal sent_value
            sent_value = yield 2

        async_gen = f()

        # Advance generator through 'yield 2' to waiting to receive a value.
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        try:
            # Setup an asend op iter with a "primed" value of 3 but override
            # it with an initial send value of 4.
            async_gen.asend(3).send(4)
        except StopAsyncIteration:
            pass
        self.assertEqual(sent_value, 4)

    def test_send_state_init_generator_raises_propagates_state_closed(self):
        async def f():
            raise ValueError
            yield

        op_iter = f().asend(None)

        with self.assertRaises(ValueError):
            op_iter.send(None)

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_send_state_init_generator_yields_raises_stop_iteration_with_value_state_closed(  # noqa: B950
        self,
    ):
        async def f():
            yield 1

        op_iter = f().asend(None)

        with self.assertRaises(StopIteration) as exc:
            op_iter.send(None)
        self.assertEqual(exc.exception.value, 1)

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_send_state_init_generator_awaits_returns_value_yielded_from_awaitable_state_iter(  # noqa: B950
        self,
    ):
        async def f():
            class Awaitable:
                def __await__(self):
                    yield 1

            await Awaitable()
            yield 2

        op_iter = f().asend(None)

        self.assertEqual(op_iter.send(None), 1)

        self._assertOpIterState(op_iter, "_STATE_ITER")

    def test_send_state_iter_arg_is_none_sends_none_into_generator(self):
        sent_value = 1

        async def f():
            class Awaitable:
                def __await__(self):
                    nonlocal sent_value
                    sent_value = yield 2

            await Awaitable()
            yield 3

        op_iter = f().asend(None)

        # Advance generator to the 'yield 2' inside the Awaitable.
        # The op_iter is now in the ITER state.
        op_iter.send(None)

        # Send None into generator to be received by 'sent_value'
        try:
            op_iter.send(None)
        except StopIteration:
            pass

        self.assertEqual(sent_value, None)

    def test_send_state_iter_arg_is_set_sends_arg_into_generator(self):
        sent_value = 1

        async def f():
            class Awaitable:
                def __await__(self):
                    nonlocal sent_value
                    sent_value = yield 2

            await Awaitable()
            yield 3

        op_iter = f().asend(None)

        # Advance generator to the 'yield 2' inside the Awaitable.
        # The op_iter is now in the ITER state.
        op_iter.send(None)

        # Send 4 into generator to be received by 'sent_value'
        try:
            op_iter.send(4)
        except StopIteration:
            pass

        self.assertEqual(sent_value, 4)

    def test_send_state_init_generator_raises_propagates_state_closed(self):
        async def f():
            class Awaitable:
                def __await__(self):
                    yield 1
                    raise ValueError

            await Awaitable()
            yield 2

        op_iter = f().asend(None)

        # Advance generator to the 'yield 1' inside the Awaitable.
        # The op_iter is now in the ITER state.
        op_iter.send(None)

        with self.assertRaises(ValueError):
            op_iter.send(None)

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_send_state_iter_generator_yields_raises_stop_iteration_with_value_state_closed(  # noqa: B950
        self,
    ):
        async def f():
            class Awaitable:
                def __await__(self):
                    yield 1

            await Awaitable()
            yield 2

        # Make asend op_iter
        op_iter = f().asend(None)

        # Advance generator to the 'yield 1' inside the Awaitable.
        # The op_iter is now in the ITER state.
        op_iter.send(None)

        with self.assertRaises(StopIteration) as exc:
            op_iter.send(None)
        self.assertEqual(exc.exception.value, 2)

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_send_state_init_generator_awaits_returns_value_yielded_from_awaitable_state_iter(  # noqa: B950
        self,
    ):
        async def f():
            class Awaitable:
                def __init__(self, yield_value):
                    self._yield_value = yield_value

                def __await__(self):
                    yield self._yield_value

            await Awaitable(1)
            await Awaitable(2)
            yield 3

        # Make asend op_iter
        op_iter = f().asend(None)

        # Advance generator to the point it runs 'yield 1' inside the Awaitable.
        # The op_iter is now in the ITER state.
        op_iter.send(None)

        self.assertEqual(op_iter.send(None), 2)

        self._assertOpIterState(op_iter, "_STATE_ITER")

    def test_throw_state_closed_raises_stop_iteration_state_closed(self):
        async def f():
            yield 1

        # Make asend op_iter in closed state
        op_iter = f().asend(None)
        op_iter.close()

        with self.assertRaises(StopIteration) as exc:
            op_iter.throw(None, None, None)
        self.assertIsNone(exc.exception.value)

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_throw_state_init_propagates_exception_through_generator_state_closed(self):
        async def f():
            yield 1

        # Make asend op_iter
        op_iter = f().asend(None)

        with self.assertRaises(ValueError) as exc:
            op_iter.throw(ValueError, 1)
        self.assertEqual(exc.exception.args, (1,))

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_throw_state_init_generator_catches_and_yields_raises_stop_iteration_with_value_state_closed(  # noqa: B950
        self,
    ):
        async def f():
            try:
                yield 1
            except ValueError:
                yield 2

        # Make async generator for f
        async_gen = f()

        # Advanced generator inside try-except block so throw can be caught
        # and further action taken.
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        op_iter = async_gen.asend(None)

        with self.assertRaises(StopIteration) as exc:
            op_iter.throw(ValueError, 3)
        self.assertEqual(exc.exception.value, 2)

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_throw_state_init_generator_catches_and_awaits_returns_value_yielded_from_awaitable_state_iter(  # noqa: B950
        self,
    ):
        async def f():
            class Awaitable:
                def __await__(self):
                    yield 1

            try:
                yield 2
            except ValueError:
                await Awaitable()

        # Make async generator for f
        async_gen = f()

        # Advanced generator inside try-except block so throw can be caught
        # and further action taken.
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        op_iter = async_gen.asend(None)

        self.assertEqual(op_iter.throw(ValueError, 3), 1)

        self._assertOpIterState(op_iter, "_STATE_ITER")


class AsyncGeneratorAthrowTests(unittest.TestCase):
    _STATE_MAP = {"_STATE_INIT": 0, "_STATE_ITER": 1, "_STATE_CLOSED": 2}

    def _assertOpIterState(self, op_iter, state_str):
        if sys.implementation.name == "pyro":
            state = _async_generator_op_iter_get_state(op_iter)
            self.assertEqual(state, AsyncGeneratorAthrowTests._STATE_MAP[state_str])

    def test_dunder_await_with_non_async_generator_athrow_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().athrow(None)).__await__(None)

    def test_dunder_iter_with_non_async_generator_athrow_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().athrow(None)).__iter__(None)

    def test_dunder_next_with_non_async_generator_athrow_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().athrow(None)).__next__(None)

    def test_close_with_non_async_generator_athrow_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().athrow(None)).close(None)

    def test_send_with_non_async_generator_athrow_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().athrow(None)).send(None, None)

    def test_throw_with_non_async_generator_athrow_raises_type_error(self):
        async def async_gen_func():
            yield

        with self.assertRaises(TypeError):
            type(async_gen_func().athrow(None)).throw(None, None, None, None)

    def test_dunder_await_returns_self(self):
        async def f():
            yield 1

        # Make athrow op_iter
        op_iter = f().athrow(ValueError, 1)

        self.assertIs(op_iter.__await__(), op_iter)

    def test_dunder_iter_returns_self(self):
        async def f():
            yield 1

        # Make athrow op_iter
        op_iter = f().athrow(ValueError, 1)

        self.assertIs(op_iter.__iter__(), op_iter)

    def test_is_awaitable(self):
        async def f():
            yield 1

        # Make athrow op_iter
        op_iter = f().athrow(ValueError, 1)

        async def awaiter(awaitable):
            return await awaitable

        try:
            # This would raise TypeError if op_iter were not awaitable.
            awaiter(op_iter).send(None)
        except ValueError:
            pass

    def test_is_iterable(self):
        async def f():
            yield 1

        # Make athrow op_iter
        op_iter = f().athrow(ValueError, 1)

        with self.assertRaises(ValueError) as exc:
            next(iter(op_iter))
        self.assertEqual(exc.exception.args, (1,))

    @pyro_only
    def test_new_instance_starts_in_init_state(self):
        async def f():
            yield 1

        # Make athrow op_iter
        op_iter = f().athrow(ValueError, 1)

        self._assertOpIterState(op_iter, "_STATE_INIT")

    @pyro_only
    def test_close_moves_into_closed_state(self):
        async def f():
            yield 1

        # Make athrow op_iter
        op_iter = f().athrow(ValueError, 1)

        op_iter.close()
        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_send_state_closed_raises_stop_iteration_state_closed(self):
        async def f():
            yield 1

        # Make athrow op_iter in closed state
        op_iter = f().athrow(ValueError, 1)
        op_iter.close()

        with self.assertRaises(StopIteration) as exc:
            op_iter.send(None)
        self.assertIsNone(exc.exception.value)

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_send_state_init_arg_is_not_none_raises_runtime_error_state_init(self):
        async def f():
            yield 1

        # Make athrow op_iter
        op_iter = f().athrow(ValueError, 1)

        with self.assertRaises(RuntimeError):
            op_iter.send(1)

        self._assertOpIterState(op_iter, "_STATE_INIT")

    def test_send_state_init_arg_is_none_generator_raises_stop_async_iteration_propagates_state_closed(  # noqa: B950
        self,
    ):
        async def f():
            try:
                yield 1
            except ValueError:
                # Returning causes StopAsyncIteration to be raised
                return

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 1'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make athrow op_iter which is primted to run inside try-except block
        op_iter = async_gen.athrow(ValueError, 2)

        with self.assertRaises(StopAsyncIteration):
            op_iter.send(None)

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_send_state_init_arg_is_none_generator_raises_generator_exit_raises_stop_iteration_state_closed(  # noqa: B950
        self,
    ):
        async def f():
            yield 1

        # Make athrow op_iter
        op_iter = f().athrow(GeneratorExit)

        with self.assertRaises(StopIteration) as exc:
            op_iter.send(None)
        self.assertIsNone(exc.exception.value)

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_send_state_init_arg_is_none_generator_yields_raises_stop_iteration_with_value_state_iter(  # noqa: B950
        self,
    ):
        async def f():
            try:
                yield 1
            except ValueError:
                yield 2

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 1'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make athrow op_iter which is primted to run inside try-except block
        op_iter = async_gen.athrow(ValueError)

        with self.assertRaises(StopIteration) as exc:
            op_iter.send(None)
        self.assertEqual(exc.exception.value, 2)

        self._assertOpIterState(op_iter, "_STATE_ITER")

    def test_send_state_init_arg_is_none_generator_awaits_returns_awaitable_yielded_value_state_iter(  # noqa: B950
        self,
    ):
        async def f():
            class Awaitable:
                def __await__(self):
                    yield 1

            try:
                yield 2
            except ValueError:
                await Awaitable()

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 2'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make athrow op_iter which is primted to run inside try-except block
        op_iter = async_gen.athrow(ValueError)

        self.assertEqual(op_iter.send(None), 1)

        self._assertOpIterState(op_iter, "_STATE_ITER")

    def test_send_state_iter_arg_is_not_none_arg_sent_into_generator(self):
        sent_value = None

        async def f():
            class Awaitable:
                def __await__(self):
                    nonlocal sent_value
                    sent_value = yield 1

            try:
                yield 2
            except ValueError:
                await Awaitable()

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 2'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make athrow op_iter which is primted to run inside try-except block
        op_iter = async_gen.athrow(ValueError, 3)

        # Executing the athrow operation which should take us to the ITER state
        # as we run 'await Awaitable()'
        op_iter.send(None)

        # Send the value 4 in which will appear at the yield in the awaitable
        # and then finish the generator with a StopAsyncIteration.
        try:
            op_iter.send(4)
        except StopAsyncIteration:
            pass
        self.assertEqual(sent_value, 4)

    def test_send_state_iter_arg_is_none_generator_raise_propagates_state_iter(  # noqa: B950
        self,
    ):
        async def f():
            class Awaitable:
                def __await__(self):
                    yield 1

            try:
                yield 2
            except ValueError:
                await Awaitable()
                raise ValueError(3)

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 2'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make athrow op_iter which is primted to run inside try-except block
        op_iter = async_gen.athrow(ValueError, 4)

        # Execute the athrow operation which should take us to the ITER state
        # as we run 'await Awaitable()'
        op_iter.send(None)

        with self.assertRaises(ValueError) as exc:
            op_iter.send(None)
        self.assertEqual(exc.exception.args, (3,))

        self._assertOpIterState(op_iter, "_STATE_ITER")

    def test_send_state_iter_arg_is_none_generator_yields_raises_stop_iteration_with_value_state_iter(  # noqa: B950
        self,
    ):
        async def f():
            class Awaitable:
                def __await__(self):
                    yield 1

            try:
                yield 2
            except ValueError:
                await Awaitable()
                yield 3

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 2'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make athrow op_iter which is primted to run inside try-except block
        op_iter = async_gen.athrow(ValueError, 4)

        # Execute the athrow operation which should take us to the ITER state
        # as we run 'await Awaitable()'
        op_iter.send(None)

        with self.assertRaises(StopIteration) as exc:
            op_iter.send(None)
        self.assertEqual(exc.exception.value, 3)

        self._assertOpIterState(op_iter, "_STATE_ITER")

    def test_send_state_iter_arg_is_none_generator_awaits_returns_awaitable_yielded_value_state_iter(  # noqa: B950
        self,
    ):
        async def f():
            class Awaitable:
                def __init__(self, yield_value):
                    self._yield_value = yield_value

                def __await__(self):
                    yield self._yield_value

            try:
                yield 1
            except ValueError:
                await Awaitable(2)
                await Awaitable(3)

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 1'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make athrow op_iter which is primted to run inside try-except block
        op_iter = async_gen.athrow(ValueError, 4)

        # Execute the athrow operation which should take us to the ITER state
        # as we run 'await Awaitable()'
        op_iter.send(None)

        self.assertEqual(op_iter.send(None), 3)

        self._assertOpIterState(op_iter, "_STATE_ITER")

    def test_throw_state_closed_raises_stop_iteration_state_closed(self):
        async def f():
            yield 1

        # Make athrow op_iter in closed state
        op_iter = f().athrow(ValueError, 1)
        op_iter.close()

        with self.assertRaises(StopIteration) as exc:
            op_iter.throw(None, None, None)
        self.assertIsNone(exc.exception.value)

        self._assertOpIterState(op_iter, "_STATE_CLOSED")

    def test_throw_state_init_raises_runtime_error_state_init(self):
        async def f():
            yield 1

        # Make athrow op_iter
        op_iter = f().athrow(ValueError, 1)

        with self.assertRaises(RuntimeError):
            op_iter.throw(None, None, None)

        self._assertOpIterState(op_iter, "_STATE_INIT")

    def test_throw_state_iter_generator_raise_propagates_state_iter(self):
        async def f():
            class Awaitable:
                def __await__(self):
                    yield 1

            try:
                yield 2
            except ValueError:
                await Awaitable()

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 2'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make athrow op_iter which is primted to run inside try-except block
        op_iter = async_gen.athrow(ValueError, 3)

        # Execute the athrow operation which should take us to the ITER state
        # as we run 'await Awaitable()'
        op_iter.send(None)

        with self.assertRaises(ValueError) as exc:
            op_iter.throw(ValueError, 4, None)
        self.assertEqual(exc.exception.args, (4,))

        self._assertOpIterState(op_iter, "_STATE_ITER")

    def test_throw_state_iter_generator_yields_raises_stop_iteration_error_with_value_state_iter(  # noqa: B950
        self,
    ):
        async def f():
            class Awaitable:
                def __await__(self):
                    yield 1

            try:
                yield 2
            except ValueError:
                try:
                    await Awaitable()
                except ValueError:
                    yield 3

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 2'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make athrow op_iter which is primted to run inside try-except block
        op_iter = async_gen.athrow(ValueError, 4)

        # Execute the athrow operation which should take us to the ITER state
        # as we run 'await Awaitable()'
        op_iter.send(None)

        with self.assertRaises(StopIteration) as exc:
            op_iter.throw(ValueError, 5, None)
        self.assertEqual(exc.exception.value, 3)

        self._assertOpIterState(op_iter, "_STATE_ITER")

    def test_throw_state_iter_generator_awaits_returns_awaitable_yielded_value_state_iter(  # noqa: B950
        self,
    ):
        async def f():
            class Awaitable:
                def __init__(self, yield_value):
                    self._yield_value = yield_value

                def __await__(self):
                    yield self._yield_value

            try:
                yield 1
            except ValueError:
                try:
                    await Awaitable(2)
                except ValueError:
                    await Awaitable(3)

        # Make an async_generator for f
        async_gen = f()

        # Advance generator into try-except block, pausing after 'yield 2'
        try:
            async_gen.asend(None).send(None)
        except StopIteration:
            pass

        # Make athrow op_iter which is primted to run inside try-except block
        op_iter = async_gen.athrow(ValueError, 4)

        # Execute the athrow operation which should take us to the ITER state
        # as we run 'await Awaitable()'
        op_iter.send(None)

        self.assertEqual(op_iter.throw(ValueError, 5, None), 3)

        self._assertOpIterState(op_iter, "_STATE_ITER")


class AwaitablesTest(unittest.TestCase):
    def test_await_on_awaitable_raising_an_exception_propagates(self):
        class Awaitable:
            def __await__(self):
                raise ValueError

        async def f():
            return await Awaitable()

        with self.assertRaises(ValueError):
            f().send(None)

    def test_await_on_awaitable_returning_non_iterator_is_type_error(self):
        class Awaitable:
            def __await__(self):
                return 1

        async def f():
            return await Awaitable()

        with self.assertRaisesRegex(
            TypeError, "__await__.* returned non-iterator of type 'int'"
        ):
            f().send(None)

    def test_await_on_awaitable_returning_coroutine_raises_type_error(self):
        async def f():
            pass

        with contextlib.closing(f()) as coro:

            class Awaitable:
                def __await__(self):
                    return coro

            async def g():
                return await Awaitable()

            with self.assertRaisesRegex(TypeError, "__await__.* returned a coroutine"):
                g().send(None)

    def test_await_on_awaitable_returning_iterable_coroutine_raises_type_error(self):
        class Awaitable:
            def __await__(self):
                @types.coroutine
                def f():
                    yield

                return f()

        async def g():
            return await Awaitable()

        with self.assertRaisesRegex(TypeError, "__await__.* returned a coroutine"):
            g().send(None)

    def test_async_for_over_awaitable_raising_on_await_raises_type_error_with_cause_and_context(
        self,
    ):
        class AsyncIterator:
            def __aiter__(self):
                return self

            def __anext__(self):
                return self

            def __await__(self):
                raise ValueError

        async def f():
            async for _ in AsyncIterator():
                pass

        with self.assertRaisesRegex(
            TypeError, "an invalid object from __anext__"
        ) as exc:
            f().send(None)

        self.assertIsInstance(exc.exception.__cause__, ValueError)
        self.assertIsInstance(exc.exception.__context__, ValueError)

    def test_async_for_over_awaitable_raising_exact_exception_on_await_raises_type_error_with_ause_and_context(
        self,
    ):
        class AsyncIterator:
            value_error = ValueError()

            def __aiter__(self):
                return self

            def __anext__(self):
                return self

            def __await__(self):
                raise AsyncIterator.value_error

        async def f():
            async for _ in AsyncIterator():
                pass

        with self.assertRaisesRegex(
            TypeError, "an invalid object from __anext__"
        ) as exc:
            f().send(None)

        self.assertIs(exc.exception.__cause__, AsyncIterator.value_error)
        self.assertIs(exc.exception.__context__, AsyncIterator.value_error)


class BinTests(unittest.TestCase):
    def test_returns_string(self):
        self.assertEqual(bin(0), "0b0")
        self.assertEqual(bin(-1), "-0b1")
        self.assertEqual(bin(1), "0b1")
        self.assertEqual(bin(54321), "0b1101010000110001")
        self.assertEqual(bin(494991), "0b1111000110110001111")

    def test_with_large_int_returns_string(self):
        self.assertEqual(
            bin(1 << 63),
            "0b1000000000000000000000000000000000000000000000000000000000000000",
        )
        self.assertEqual(
            bin(1 << 64),
            "0b10000000000000000000000000000000000000000000000000000000000000000",
        )
        self.assertEqual(
            bin(0xDEE182DE2EC55F61B22A509ED1DC3EB),
            "0b1101111011100001100000101101111000101110110001010101111101"
            "1000011011001000101010010100001001111011010001110111000011111"
            "01011",
        )
        self.assertEqual(
            bin(-0x53ADC651E593B1323158BFA776E8173F60C76519277B2BD6),
            "-0b1010011101011011100011001010001111001011001001110110001001"
            "1001000110001010110001011111110100111011101101110100000010111"
            "0011111101100000110001110110010100011001001001110111101100101"
            "01111010110",
        )

    def test_calls_dunder_index(self):
        class C:
            def __int__(self):
                return 42

            def __index__(self):
                return -9

        self.assertEqual(bin(C()), "-0b1001")

    def test_with_int_subclass(self):
        class C(int):
            pass

        self.assertEqual(bin(C(51)), "0b110011")

    def test_with_non_int_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bin("not an int")
        self.assertEqual(
            str(context.exception), "'str' object cannot be interpreted as an integer"
        )


class BoolTests(unittest.TestCase):
    def test_dunder_hash(self):
        self.assertEqual(bool.__hash__(False), 0)
        self.assertEqual(bool.__hash__(True), 1)

    def test_dunder_hash_matches_int_dunder_hash(self):
        self.assertEqual(bool.__hash__(False), int.__hash__(0))
        self.assertEqual(bool.__hash__(True), int.__hash__(1))

    def test_dunder_and_success(self):
        self.assertEqual(bool.__and__(True, True), True)
        self.assertEqual(bool.__and__(True, False), False)
        self.assertEqual(bool.__and__(True, 1024 - 1), 1)
        self.assertEqual(bool.__and__(True, 1024), 0)
        self.assertEqual(bool.__and__(False, True), False)
        self.assertEqual(bool.__and__(False, False), False)
        self.assertEqual(bool.__and__(False, 1), 0)
        self.assertEqual(bool.__and__(False, 0), 0)

    def test_dunder_and_not_implemented(self):
        self.assertIs(bool.__and__(True, "string"), NotImplemented)
        self.assertIs(bool.__and__(True, 1.8), NotImplemented)

    def test_dunder_rand_success(self):
        self.assertEqual(bool.__rand__(True, True), True)
        self.assertEqual(bool.__rand__(False, True), False)
        self.assertEqual(bool.__rand__(True, 1024 - 1), 1)
        self.assertEqual(bool.__rand__(True, 1024), 0)
        self.assertEqual(bool.__rand__(True, False), False)
        self.assertEqual(bool.__rand__(False, False), False)
        self.assertEqual(bool.__rand__(False, 1), 0)
        self.assertEqual(bool.__rand__(False, 0), 0)

    def test_dunder_rand_not_implemented(self):
        self.assertIs(bool.__rand__(True, "string"), NotImplemented)
        self.assertIs(bool.__rand__(True, 1.8), NotImplemented)

    def test_dunder_or_with_non_bool_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__or__' requires a 'bool' object but .* 'int'",
            bool.__or__,
            1,
            1,
        )

    def test_dunder_or_success(self):
        self.assertEqual(bool.__or__(True, True), True)
        self.assertEqual(bool.__or__(True, False), True)
        self.assertEqual(bool.__or__(True, 1024 - 1), 1023)
        self.assertEqual(bool.__or__(True, 1024), 1025)
        self.assertEqual(bool.__or__(False, True), True)
        self.assertEqual(bool.__or__(False, False), False)
        self.assertEqual(bool.__or__(False, 1), 1)
        self.assertEqual(bool.__or__(False, 0), 0)

    def test_dunder_or_not_implemented(self):
        self.assertIs(bool.__or__(True, "string"), NotImplemented)
        self.assertIs(bool.__or__(True, 1.8), NotImplemented)

    def test_dunder_or_with_non_bool_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__ror__' requires a 'bool' object but .* 'int'",
            bool.__ror__,
            1,
            1,
        )

    def test_dunder_ror_success(self):
        self.assertEqual(bool.__ror__(True, True), True)
        self.assertEqual(bool.__ror__(False, True), True)
        self.assertEqual(bool.__ror__(True, 1024 - 1), 1023)
        self.assertEqual(bool.__ror__(True, 1024), 1025)
        self.assertEqual(bool.__ror__(True, False), True)
        self.assertEqual(bool.__ror__(False, False), False)
        self.assertEqual(bool.__ror__(False, 1), 1)
        self.assertEqual(bool.__ror__(False, 0), 0)

    def test_dunder_ror_not_implemented(self):
        self.assertIs(bool.__ror__(True, "string"), NotImplemented)
        self.assertIs(bool.__ror__(True, 1.8), NotImplemented)

    def test_dunder_xor_with_non_bool_raises_type_error(self):
        with self.assertRaises(TypeError) as ctx:
            bool.__xor__(1, 1)
        self.assertIn(
            "'__xor__' requires a 'bool' object but received a 'int'",
            str(ctx.exception),
        )

    def test_dunder_xor_success(self):
        self.assertEqual(bool.__xor__(True, True), False)
        self.assertEqual(bool.__xor__(True, False), True)
        self.assertEqual(bool.__xor__(True, 1024 - 1), 1022)
        self.assertEqual(bool.__xor__(True, 1024), 1025)
        self.assertEqual(bool.__xor__(False, True), True)
        self.assertEqual(bool.__xor__(False, False), False)
        self.assertEqual(bool.__xor__(False, 1), 1)
        self.assertEqual(bool.__xor__(False, 0), 0)

    def test_dunder_xor_with_non_int_returns_notimplemented(self):
        self.assertIs(bool.__xor__(True, "string"), NotImplemented)
        self.assertIs(bool.__xor__(True, 1.8), NotImplemented)

    def test_dunder_xor_with_non_bool_raises_type_error(self):
        with self.assertRaises(TypeError) as ctx:
            bool.__rxor__(1, 1)
        self.assertIn(
            "'__rxor__' requires a 'bool' object but received a 'int'",
            str(ctx.exception),
        )

    def test_dunder_rxor_success(self):
        self.assertEqual(bool.__rxor__(True, True), False)
        self.assertEqual(bool.__rxor__(False, True), True)
        self.assertEqual(bool.__rxor__(True, 1024 - 1), 1022)
        self.assertEqual(bool.__rxor__(True, 1024), 1025)
        self.assertEqual(bool.__rxor__(True, False), True)
        self.assertEqual(bool.__rxor__(False, False), False)
        self.assertEqual(bool.__rxor__(False, 1), 1)
        self.assertEqual(bool.__rxor__(False, 0), 0)

    def test_dunder_rxor_with_non_int_returns_notimplemented(self):
        self.assertIs(bool.__rxor__(True, "string"), NotImplemented)
        self.assertIs(bool.__rxor__(True, 1.8), NotImplemented)


class BoundMethodTests(unittest.TestCase):
    def test_bound_method_dunder_func(self):
        class Foo:
            def foo(self):
                pass

        self.assertIs(Foo.foo, Foo().foo.__func__)

    def test_bound_method_dunder_self(self):
        class Foo:
            def foo(self):
                pass

        f = Foo()
        self.assertIs(f, f.foo.__self__)

    def test_bound_method_doc(self):
        class Foo:
            def foo(self):
                "This is the docstring of foo"
                pass

        self.assertEqual(Foo().foo.__doc__, "This is the docstring of foo")
        self.assertIs(Foo.foo.__doc__, Foo().foo.__doc__)

    def test_bound_method_readonly_attributes(self):
        class Foo:
            def foo(self):
                "This is the docstring of foo"
                pass

        f = Foo().foo
        with self.assertRaises(AttributeError):
            f.__func__ = abs

        with self.assertRaises(AttributeError):
            f.__self__ = int

        with self.assertRaises(AttributeError):
            f.__doc__ = "hey!"

    def test_bound_method_getattribute(self):
        class C:
            def meth(self):
                pass

            meth.attr = 42

        c = C()
        bound = c.meth
        self.assertEqual(bound.attr, 42)

    def test_bound_method_dunder_eq_with_invalid_self_raises_type_error(self):
        class C:
            def meth(self):
                pass

        with self.assertRaises(TypeError):
            type(C().meth).__eq__(None, None)

    def test_bound_method_is_not_same_with_same_method_on_same_instance(self):
        class C:
            def meth(self):
                pass

        c = C()
        bound_meth1 = c.meth
        bound_meth2 = c.meth
        self.assertFalse(bound_meth1 is bound_meth2)

    def test_bound_method_equal_with_same_method_on_same_instance(self):
        class C:
            def meth(self):
                pass

        c = C()
        bound_meth1 = c.meth
        bound_meth2 = c.meth
        self.assertTrue(bound_meth1.__eq__(bound_meth2))

    def test_bound_method_not_equal_with_same_method_on_different_instances(self):
        class C:
            def meth(self):
                pass

        bound_meth1 = C().meth
        bound_meth2 = C().meth
        self.assertFalse(bound_meth1.__eq__(bound_meth2))

    def test_bound_method_not_equal_with_different_method_on_same_instances(self):
        class C:
            def meth1(self):
                pass

            def meth2(self):
                pass

        c = C()
        bound_meth1 = c.meth1
        bound_meth2 = c.meth2
        self.assertFalse(bound_meth1.__eq__(bound_meth2))

    def test_bound_method_compared_to_non_bound_method_returns_not_implemented(self):
        class C:
            def meth(self):
                pass

        self.assertEqual(C().meth.__eq__(None), NotImplemented)

    def test_bound_method_dunder_eq_respects_overriden_self_equality_for_non_identical_methods(
        self,
    ):
        class C:
            def meth(self):
                pass

            def __eq__(self, other):
                return True

        m0 = C().meth
        m1 = C().meth
        self.assertTrue(m0.__eq__(m1))

    def test_bound_method_dunder_eq_ignores_overriden_self_equality_for_identical_methods(
        self,
    ):
        class C:
            def meth(self):
                pass

            def __eq__(self, other):
                return False

        m = C().meth
        self.assertTrue(m.__eq__(m))

    def test_bound_method_dunder_eq_checks_func_before_self(self):
        class C:
            def meth1(self):
                pass

            def meth2(self):
                pass

            def __eq__(self, other):
                raise ValueError

        # This would raise ValueError if self were checked before func
        self.assertFalse(C().meth1.__eq__(C().meth2))


class CallableIteratorTest(unittest.TestCase):
    def test_callable_iterator_iterates_till_sentinel(self):
        class C:
            def __init__(self):
                self.x = 0

            def __call__(self):
                self.x += 1
                return self.x

        i = C()
        it = iter(i, 5)
        self.assertFalse(hasattr(it, "__len__"))
        self.assertEqual(list(it), [1, 2, 3, 4])

    @pyro_only
    def test_callable_iterator_dunder_init_initializes(self):
        class C:
            def __init__(self):
                self.x = -5

            def __call__(self):
                self.x += 3
                return self.x

        i = C()
        it = builtins.callable_iterator(i, 7)
        self.assertEqual(list(it), [-2, 1, 4])


class ChrTests(unittest.TestCase):
    def test_returns_string(self):
        self.assertEqual(chr(101), "e")
        self.assertEqual(chr(42), "*")
        self.assertEqual(chr(0x1F40D), "\U0001f40d")

    def test_with_int_subclass_returns_string(self):
        class C(int):
            pass

        self.assertEqual(chr(C(122)), "z")

    def test_with_unicode_max_returns_string(self):
        import sys

        self.assertEqual(ord(chr(sys.maxunicode)), sys.maxunicode)

    def test_with_unicode_max_plus_one_raises_value_error(self):
        import sys

        with self.assertRaises(ValueError) as context:
            chr(sys.maxunicode + 1)
        self.assertIn("chr() arg not in range", str(context.exception))

    def test_with_negative_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            chr(-1)
        self.assertIn("chr() arg not in range", str(context.exception))

    def test_non_int_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            chr(None)
        self.assertEqual(
            str(context.exception), "an integer is required (got type NoneType)"
        )

    def test_with_large_int_raises_overflow_error(self):
        with self.assertRaises(OverflowError) as context:
            chr(123456789012345678901234567890)
        self.assertEqual(
            str(context.exception), "Python int too large to convert to C long"
        )


class ClassMethodTests(unittest.TestCase):
    def test_dunder_abstractmethod_with_missing_attr_returns_false(self):
        def foo():
            pass

        method = classmethod(foo)
        self.assertIs(method.__isabstractmethod__, False)

    def test_dunder_abstractmethod_with_false_attr_returns_false(self):
        def foo():
            pass

        foo.__isabstractmethod__ = False
        with self.assertRaises(AttributeError):
            type(foo).__isabstractmethod__
        prop = property(foo)
        self.assertIs(prop.__isabstractmethod__, False)

    def test_dunder_abstractmethod_with_abstract_returns_true(self):
        def foo():
            pass

        foo.__isabstractmethod__ = ["random", "values"]
        with self.assertRaises(AttributeError):
            type(foo).__isabstractmethod__
        method = classmethod(foo)
        self.assertIs(method.__isabstractmethod__, True)

    def test_dunder_abstractmethod_with_non_classmethod_raises_type_error(self):
        with self.assertRaises(TypeError):
            classmethod.__dict__["__isabstractmethod__"].__get__(42)

    def test_dunder_class_setter_with_non_type_raises_type_error(self):
        class C:
            pass

        class D:
            pass

        obj = C()
        with self.assertRaisesRegex(TypeError, "__class__ must be"):
            obj.__class__ = 123

    def test_dunder_class_setter_on_builtin_types_raises_type_error(self):
        class C:
            pass

        class D:
            pass

        obj = "foo"
        with self.assertRaisesRegex(TypeError, "only supported for"):
            obj.__class__ = C
        with self.assertRaisesRegex(TypeError, "only supported for"):
            C.__class__ = D

    def test_dunder_class_setter_on_different_layout_raises_type_error(self):
        class C(float):
            pass

        class D(list):
            pass

        obj = C(12.3)
        with self.assertRaisesRegex(TypeError, "layout differs"):
            obj.__class__ = D

    def test_dunder_class_setter_changes_instance_type(self):
        class C:
            def foo(self):
                return 123

        class D:
            def foo(self):
                return 321

        obj = C()
        obj.__class__ = D
        self.assertIsInstance(obj, D)
        self.assertEqual(obj.foo(), 321)

    def test_dunder_class_setter_retains_original_attributes(self):
        class C:
            def __init__(self):
                self.a = 123
                self.b = 321

            pass

        class D:
            def __init__(self):
                self.a = 789
                self.b = 987

        obj = C()
        obj.__class__ = D
        self.assertIsInstance(obj, D)
        self.assertEqual(obj.a, 123)
        self.assertEqual(obj.b, 321)

    def test_dunder_func_returns_wrapped_function(self):
        class C:
            def foo(self):
                return 1

            bar = classmethod(foo)

        self.assertIs(C.bar.__func__, C.foo)

    def test_dunder_func_assignment_raises_attribute_error(self):
        class C:
            def foo(self):
                return 1

            bar = classmethod(foo)

        with self.assertRaises(AttributeError):
            C.bar.__func__ = C.foo

    def test_has_dunder_call(self):
        class C:
            @classmethod
            def bar(cls):
                pass

        C.bar.__getattribute__("__call__")

    def test_dunder_get_returns_value(self):
        class C:
            @classmethod
            def bar(cls):
                return 5

        self.assertEqual(C.bar(), 5)

    def test_dunder_get_with_subclassed_classmethod_returns_value(self):
        class foo(classmethod):
            pass

        class C:
            @foo
            def bar(self):
                return 5

        self.assertEqual(C.bar(), 5)

    def test_dunder_get_called_with_non_classmethod_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            def bar():
                pass

            classmethod.__get__(42.3, None, bar)
        self.assertIn(
            "'__get__' requires a 'classmethod' object but received a 'float'",
            str(context.exception),
        )

    def test_dunder_new_called_with_non_type_object_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            def bar():
                pass

            classmethod.__new__(42.3, bar)
        self.assertIn("not a type object", str(context.exception))

    def test_dunder_new_called_with_non_subtype_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            def bar():
                pass

            classmethod.__new__(float, bar)
        self.assertIn("not a subtype of classmethod", str(context.exception))

    def test_dunder_new_with_subclassed_classmethod_returns_instance_of_superclass(
        self,
    ):
        class foo(classmethod):
            pass

        def bar():
            pass

        self.assertIsInstance(classmethod.__new__(foo, bar), foo)


class CodeTests(unittest.TestCase):
    def foo(self):
        pass

    CodeType = type(foo.__code__)

    if sys.implementation.name == "pyro" or sys.version_info >= (3, 8):
        SAMPLE = CodeType(
            1,
            1,
            1,
            4,
            1,
            1,
            b"",
            (),
            (),
            ("a", "b"),
            "filename",
            "name",
            1,
            b"",
            (),
            (),
        )

    def test_dunder_hash_with_non_code_object_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType.__hash__(None)
        self.assertIn(
            "'__hash__' requires a 'code' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_dunder_hash_returns_stable_value_on_different_code_objects(self):
        def foo():
            return 4

        first_foo_code = foo.__code__

        def foo():
            return 4

        second_foo_code = foo.__code__

        self.assertIsNot(first_foo_code, second_foo_code)
        self.assertEqual(hash(first_foo_code), hash(second_foo_code))

    @supports_38_feature
    def test_dunder_new_with_non_int_argcount_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(
                "non_int",
                1,
                1,
                4,
                1,
                1,
                b"",
                (),
                (),
                ("a", "b"),
                "filename",
                "name",
                1,
                b"",
                (),
                (),
            )
        self.assertIn("an integer is required (got type str)", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_negative_argcount_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            self.CodeType(
                -1,
                1,
                1,
                4,
                1,
                1,
                b"",
                (),
                (),
                ("a", "b"),
                "filename",
                "name",
                1,
                b"",
                (),
                (),
            )
        self.assertIn("argcount must not be negative", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_non_int_posonlyargcount_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(
                1,
                "non_int",
                1,
                4,
                1,
                1,
                b"",
                (),
                (),
                ("a", "b"),
                "filename",
                "name",
                1,
                b"",
                (),
                (),
            )
        self.assertIn("an integer is required (got type str)", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_non_int_kwonlyargcount_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(
                1,
                1,
                "non_int",
                4,
                1,
                1,
                b"",
                (),
                (),
                ("a", "b"),
                "filename",
                "name",
                1,
                b"",
                (),
                (),
            )
        self.assertIn("an integer is required (got type str)", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_negative_kwonlyargcount_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            self.CodeType(
                1,
                0,
                -1,
                4,
                1,
                1,
                b"",
                (),
                (),
                ("a", "b"),
                "filename",
                "name",
                1,
                b"",
                (),
                (),
            )
        self.assertIn("kwonlyargcount must not be negative", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_non_int_nlocals_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(
                1,
                0,
                1,
                "non_int",
                1,
                1,
                b"",
                (),
                (),
                ("a", "b"),
                "filename",
                "name",
                1,
                b"",
                (),
                (),
            )
        self.assertIn("an integer is required (got type str)", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_negative_nlocals_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            self.CodeType(
                1,
                0,
                1,
                -4,
                1,
                1,
                b"",
                (),
                (),
                ("a", "b"),
                "filename",
                "name",
                1,
                b"",
                (),
                (),
            )
        self.assertIn("nlocals must not be negative", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_non_int_stacksize_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(
                1,
                0,
                1,
                1,
                "non_int",
                1,
                b"",
                (),
                (),
                ("a", "b"),
                "filename",
                "name",
                1,
                b"",
                (),
                (),
            )
        self.assertIn("an integer is required (got type str)", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_negative_stacksize_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            self.CodeType(
                1,
                0,
                1,
                4,
                -1,
                1,
                b"",
                (),
                (),
                ("a", "b"),
                "filename",
                "name",
                1,
                b"",
                (),
                (),
            )
        self.assertIn("stacksize must not be negative", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_non_int_flags_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(
                1,
                0,
                1,
                1,
                1,
                "non_int",
                b"",
                (),
                (),
                ("a", "b"),
                "filename",
                "name",
                1,
                b"",
                (),
                (),
            )
        self.assertIn("an integer is required (got type str)", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_negative_flags_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            self.CodeType(
                1,
                0,
                1,
                4,
                1,
                -1,
                b"",
                (),
                (),
                ("a", "b"),
                "filename",
                "name",
                1,
                b"",
                (),
                (),
            )
        self.assertIn("flags must not be negative", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_non_bytes_code_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(1, 0, 1, 1, 1, 1, "not_bytes", 1, 1, 1, 1, 1, 1, 1)
        self.assertIn("bytes", str(context.exception))
        self.assertIn("str", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_non_tuple_consts_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(1, 0, 1, 1, 1, 1, b"", "not_tuple", 1, 1, 1, 1, 1, 1)
        self.assertIn("tuple", str(context.exception))
        self.assertIn("str", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_non_tuple_names_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(1, 0, 1, 1, 1, 1, b"", (), "not_tuple", 1, 1, 1, 1, 1)
        self.assertIn("tuple", str(context.exception))
        self.assertIn("str", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_non_tuple_varnames_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(1, 0, 1, 1, 1, 1, b"", (), (), "not_tuple", 1, 1, 1, 1)
        self.assertIn("tuple", str(context.exception))
        self.assertIn("str", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_non_str_filename_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(1, 0, 1, 1, 1, 1, b"", (), (), (), b"not_str", 1, 1, 1)
        self.assertIn("str", str(context.exception))
        self.assertIn("bytes", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_non_str_name_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(
                1, 0, 1, 1, 1, 1, b"", (), (), (), "filename", b"not_str", 1, 1
            )
        self.assertIn("str", str(context.exception))
        self.assertIn("bytes", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_non_int_firstlineno_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(
                1,
                0,
                1,
                4,
                1,
                1,
                b"",
                (),
                (),
                ("a", "b"),
                "filename",
                "name",
                "non_int",
                b"",
                (),
                (),
            )
        self.assertIn("an integer is required (got type str)", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_negative_firstlineno_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            self.CodeType(
                1,
                0,
                1,
                4,
                1,
                1,
                b"",
                (),
                (),
                ("a", "b"),
                "filename",
                "name",
                -1,
                b"",
                (),
                (),
            )
        self.assertIn("firstlineno must not be negative", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_non_bytes_lnotab_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(
                1, 0, 1, 1, 1, 1, b"", (), (), (), "filename", "name", 1, "not_bytes"
            )
        self.assertIn("bytes", str(context.exception))
        self.assertIn("str", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_non_tuple_freevars_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(
                1,
                0,
                1,
                1,
                1,
                1,
                b"",
                (),
                (),
                (),
                "filename",
                "name",
                1,
                b"",
                "not_tuple",
            )
        self.assertIn("tuple", str(context.exception))
        self.assertIn("str", str(context.exception))

    @supports_38_feature
    def test_dunder_new_with_non_tuple_cellvars_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(
                1,
                0,
                1,
                1,
                1,
                1,
                b"",
                (),
                (),
                (),
                "filename",
                "name",
                1,
                b"",
                (),
                "not_tuple",
            )
        self.assertIn("tuple", str(context.exception))
        self.assertIn("str", str(context.exception))

    @supports_38_feature
    def test_dunder_new_returns_code(self):
        result = self.CodeType(
            1,
            0,
            1,
            4,
            1,
            1,
            b"",
            (),
            (),
            ("a", "b"),
            "filename",
            "name",
            1,
            b"",
            (),
            (),
        )
        self.assertIsInstance(result, self.CodeType)

    @supports_38_feature
    def test_replace_with_non_int_argcount_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.SAMPLE.replace(co_argcount="non_int")

    @supports_38_feature
    def test_replace_with_negative_argcount_raises_value_error(self):
        with self.assertRaises(ValueError):
            self.SAMPLE.replace(co_argcount=-7)

    @supports_38_feature
    def test_replace_with_argcount_replaces_argcount(self):
        self.assertNotEqual(self.SAMPLE.co_argcount, 0)
        result = self.SAMPLE.replace(co_argcount=0)
        self.assertEqual(result.co_argcount, 0)

    @supports_38_feature
    def test_replace_with_non_int_posonlyargcount_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.SAMPLE.replace(co_posonlyargcount="non_int")

    @supports_38_feature
    def test_replace_with_negative_posonlyargcount_raises_value_error(self):
        with self.assertRaises(ValueError):
            self.SAMPLE.replace(co_posonlyargcount=-7)

    @supports_38_feature
    def test_replace_with_posonlyargcount_replaces_posonlyargcount(self):
        self.assertNotEqual(self.SAMPLE.co_posonlyargcount, 0)
        result = self.SAMPLE.replace(co_posonlyargcount=0)
        self.assertEqual(result.co_posonlyargcount, 0)

    @supports_38_feature
    def test_replace_with_non_int_kwonlyargcount_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.SAMPLE.replace(co_kwonlyargcount="non_int")

    @supports_38_feature
    def test_replace_with_negative_kwonlyargcount_raises_value_error(self):
        with self.assertRaises(ValueError):
            self.SAMPLE.replace(co_kwonlyargcount=-7)

    @supports_38_feature
    def test_replace_with_kwonlyargcount_replaces_kwonlyargcount(self):
        self.assertNotEqual(self.SAMPLE.co_kwonlyargcount, 0)
        result = self.SAMPLE.replace(co_kwonlyargcount=0)
        self.assertEqual(result.co_kwonlyargcount, 0)

    @supports_38_feature
    def test_replace_with_non_int_nlocals_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.SAMPLE.replace(co_nlocals="non_int")

    @supports_38_feature
    def test_replace_with_negative_nlocals_raises_value_error(self):
        with self.assertRaises(ValueError):
            self.SAMPLE.replace(co_nlocals=-7)

    @supports_38_feature
    def test_replace_with_nlocals_replaces_nlocals(self):
        self.assertNotEqual(self.SAMPLE.co_nlocals, 5)
        result = self.SAMPLE.replace(co_nlocals=5)
        self.assertEqual(result.co_nlocals, 5)

    @supports_38_feature
    def test_replace_with_non_int_stacksize_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.SAMPLE.replace(co_stacksize="non_int")

    @supports_38_feature
    def test_replace_with_negative_stacksize_raises_value_error(self):
        with self.assertRaises(ValueError):
            self.SAMPLE.replace(co_stacksize=-7)

    @supports_38_feature
    def test_replace_with_stacksize_replaces_stacksize(self):
        self.assertNotEqual(self.SAMPLE.co_stacksize, 0)
        result = self.SAMPLE.replace(co_stacksize=0)
        self.assertEqual(result.co_stacksize, 0)

    @supports_38_feature
    def test_replace_with_non_int_flags_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.SAMPLE.replace(co_flags="non_int")

    @supports_38_feature
    def test_replace_with_negative_flags_raises_value_error(self):
        with self.assertRaises(ValueError):
            self.SAMPLE.replace(co_flags=-7)

    @supports_38_feature
    def test_replace_with_flags_replaces_flags(self):
        self.assertNotEqual(self.SAMPLE.co_flags, 64)
        result = self.SAMPLE.replace(co_flags=64)
        self.assertEqual(result.co_flags, 64)

    @supports_38_feature
    def test_replace_with_non_bytes_code_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.SAMPLE.replace(co_code="non_bytes")

    @supports_38_feature
    def test_replace_with_code_replaces_code(self):
        self.assertNotEqual(self.SAMPLE.co_code, b"123")
        result = self.SAMPLE.replace(co_code=b"123")
        self.assertEqual(result.co_code, b"123")

    @supports_38_feature
    def test_replace_with_non_tuple_consts_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.SAMPLE.replace(co_consts="non_tuple")

    @supports_38_feature
    def test_replace_with_consts_replaces_consts(self):
        self.assertNotEqual(self.SAMPLE.co_consts, (1, 2, 3))
        result = self.SAMPLE.replace(co_consts=(1, 2, 3))
        self.assertEqual(result.co_consts, (1, 2, 3))

    @supports_38_feature
    def test_replace_with_non_tuple_names_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.SAMPLE.replace(co_names="non_tuple")

    @supports_38_feature
    def test_replace_with_names_replaces_names(self):
        self.assertNotEqual(self.SAMPLE.co_names, ("foo", "bar"))
        result = self.SAMPLE.replace(co_names=("foo", "bar"))
        self.assertEqual(result.co_names, ("foo", "bar"))

    @supports_38_feature
    def test_replace_with_non_tuple_varnames_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.SAMPLE.replace(co_varnames="non_tuple")

    @supports_38_feature
    def test_replace_with_varnames_replaces_varnames(self):
        self.assertNotEqual(self.SAMPLE.co_varnames, ("foo", "bar"))
        result = self.SAMPLE.replace(co_varnames=("foo", "bar"))
        self.assertEqual(result.co_varnames, ("foo", "bar"))

    @supports_38_feature
    def test_replace_with_non_str_filename_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.SAMPLE.replace(co_filename=b"non_str")

    @supports_38_feature
    def test_replace_with_filename_replaces_filename(self):
        self.assertNotEqual(self.SAMPLE.co_filename, "newfilename")
        result = self.SAMPLE.replace(co_filename="newfilename")
        self.assertEqual(result.co_filename, "newfilename")

    @supports_38_feature
    def test_replace_with_non_str_name_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.SAMPLE.replace(co_name=b"non_str")

    @supports_38_feature
    def test_replace_with_name_replaces_name(self):
        self.assertNotEqual(self.SAMPLE.co_name, "newname")
        result = self.SAMPLE.replace(co_name="newname")
        self.assertEqual(result.co_name, "newname")

    @supports_38_feature
    def test_replace_with_non_int_firstlineno_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.SAMPLE.replace(co_firstlineno="non_int")

    @supports_38_feature
    def test_replace_with_negative_firstlineno_raises_value_error(self):
        with self.assertRaises(ValueError):
            self.SAMPLE.replace(co_firstlineno=-7)

    @supports_38_feature
    def test_replace_with_firstlineno_replaces_firstlineno(self):
        self.assertNotEqual(self.SAMPLE.co_firstlineno, 0)
        result = self.SAMPLE.replace(co_firstlineno=0)
        self.assertEqual(result.co_firstlineno, 0)

    @supports_38_feature
    def test_replace_with_non_bytes_lnotab_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.SAMPLE.replace(co_lnotab="non_bytes")

    @supports_38_feature
    def test_replace_with_lnotab_replaces_lnotab(self):
        self.assertNotEqual(self.SAMPLE.co_lnotab, b"123")
        result = self.SAMPLE.replace(co_lnotab=b"123")
        self.assertEqual(result.co_lnotab, b"123")

    @supports_38_feature
    def test_replace_with_non_tuple_freevars_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.SAMPLE.replace(co_freevars="non_tuple")

    @supports_38_feature
    def test_replace_with_freevars_replaces_freevars(self):
        self.assertNotEqual(self.SAMPLE.co_freevars, ("foo", "bar"))
        result = self.SAMPLE.replace(co_freevars=("foo", "bar"))
        self.assertEqual(result.co_freevars, ("foo", "bar"))

    @supports_38_feature
    def test_replace_with_non_tuple_cellvars_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.SAMPLE.replace(co_cellvars="non_tuple")

    @supports_38_feature
    def test_replace_with_cellvars_replaces_cellvars(self):
        self.assertNotEqual(self.SAMPLE.co_cellvars, ("foo", "bar"))
        result = self.SAMPLE.replace(co_cellvars=("foo", "bar"))
        self.assertEqual(result.co_cellvars, ("foo", "bar"))


class ContextManagerTests(unittest.TestCase):
    def test_dunder_exit_return(self):
        class Manager:
            def __init__(self, test_case):
                self._test_case = test_case

            def __enter__(self):
                pass

            def __exit__(self, ty, val, tb):
                self._test_case.assertIs(ty, None)
                self._test_case.assertIs(val, None)
                self._test_case.assertIs(tb, None)

        def f():
            with Manager(self):
                return 10

        self.assertEqual(f(), 10)


class CompileTests(unittest.TestCase):
    def test_compile_with_iterable_coroutine_flag_raises_value_error(self):
        self.assertRaises(ValueError, compile, "result = 42", "", "exec", 0x100)

    def test_compile_with_only_ast_flag_returns_ast(self):
        result = compile("result = 42", "", "exec", ast.PyCF_ONLY_AST)
        self.assertIsInstance(result, ast.AST)

    def test_compile_with_dont_inherit_and_only_ast_flag_returns_ast(self):
        result = compile("result = 42", "", "exec", ast.PyCF_ONLY_AST, True)
        self.assertIsInstance(result, ast.AST)

    def test_compile_with_negative_optimize_raises_value_error(self):
        with self.assertRaisesRegex(ValueError, r"compile\(\): invalid optimize value"):
            compile("True", "", "exec", 0, True, -2)

    def test_compile_with_too_high_optimize_raises_value_error(self):
        with self.assertRaisesRegex(ValueError, r"compile\(\): invalid optimize value"):
            compile("True", "", "exec", 0, True, 3)

    def test_compile_with_optimize_disables_asserts(self):
        code = compile("assert False", "", "exec", 0, True, 1)
        exec(code)  # should not raise

    def test_exec_mode_returns_code(self):
        from types import CodeType
        from types import ModuleType

        code = compile("result = 42", "", "exec", 0, True, -1)
        self.assertIsInstance(code, CodeType)
        module = ModuleType("")
        exec(code, module.__dict__)
        self.assertEqual(module.result, 42)

    def test_single_returns_code(self):
        from types import CodeType
        from types import ModuleType

        code = compile("result = 8", "", "exec", 0, True, -1)
        self.assertIsInstance(code, CodeType)
        module = ModuleType("")
        exec(code, module.__dict__)
        self.assertEqual(module.result, 8)

    def test_eval_mode_returns_code(self):
        from types import CodeType

        code = compile("7 * 9", "", "eval", 0, True, -1)
        self.assertIsInstance(code, CodeType)
        self.assertEqual(eval(code), 63)  # noqa: P204

    def test_with_flags_returns_code(self):
        import __future__

        from types import CodeType

        code = compile(
            "7 <> 9", "", "eval", __future__.CO_FUTURE_BARRY_AS_BDFL, True, -1
        )
        self.assertIsInstance(code, CodeType)

    def test_inherits_compile_flags(self):
        import __future__

        from types import CodeType

        code = compile(
            "compile('7 <> 9', '', 'eval')",
            "",
            "eval",
            __future__.CO_FUTURE_BARRY_AS_BDFL,
            True,
            -1,
        )
        self.assertIsInstance(code, CodeType)

    def test_with_bytes_source_returns_code(self):
        from types import CodeType

        code = compile(b"42", "", "eval")
        self.assertIsInstance(code, CodeType)

    def test_with_bytearray_source_returns_code(self):
        from types import CodeType

        code = compile(bytearray(b"42"), "", "eval")
        self.assertIsInstance(code, CodeType)

    def test_raises_syntax_error(self):
        with self.assertRaises(SyntaxError) as context:
            compile("$*@", "bar", "exec")
        self.assertEqual(context.exception.filename, "bar")
        self.assertEqual(context.exception.lineno, 1)
        with self.assertRaises(SyntaxError):
            compile("7 <> 9", "", "eval", 0, True, -1)

    def test_statement_with_eval_mode_raises_syntax_error(self):
        with self.assertRaises(SyntaxError):
            compile("pass", "", "eval")

    def test_statements_with_single_mode_raises_syntax_error(self):
        with self.assertRaises(SyntaxError) as context:
            compile("pass\npass", "", "single")
        self.assertIn(
            "multiple statements found while compiling a single statement",
            str(context.exception),
        )

    def test_with_invalid_source_raises_type_error(self):
        with self.assertRaises(TypeError):
            compile(42, "", "eval")

    def test_with_invalid_mode_raises_value_error(self):
        with self.assertRaises(ValueError):
            compile("", "", "not a valid mode")


class ComplexTests(unittest.TestCase):
    def test_dunder_abs_with_non_complex_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            complex.__abs__(1)

    def test_dunder_abs_calculates_magnitude_returns_float(self):
        nums = [complex(x / 3.0, y / 7.0) for x in range(-9, 9) for y in range(-9, 9)]
        for num in nums:
            self.assertAlmostEqual((num.real ** 2 + num.imag ** 2) ** 0.5, abs(num), 5)

    def test_dunder_add_with_non_complex_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            complex.__add__(1, 2)

    def test_dunder_add_with_non_number_type_returns_not_implemented(self):
        self.assertIs(complex(3.2, 0).__add__("not-num"), NotImplemented)

    def test_dunder_add_adds_numbers_together(self):
        self.assertEqual(complex(1.0, 1.0).__add__(0), complex(1, 1))
        self.assertEqual(complex(1.0, 1.0).__add__(1.0), complex(2, 1))
        self.assertEqual(complex(1.0, 1.0).__add__(complex(2, 2)), complex(3, 3))

    def test_dunder_add_with_int_subclass_returns_complex(self):
        class A(int):
            pass

        self.assertEqual(complex(1.0, 1.0).__add__(A(3)), complex(4, 1))

    def test_dunder_bool_returns_correct_boolean(self):
        self.assertTrue(complex.__bool__(complex(0.0, 1.0)))
        self.assertTrue(complex.__bool__(complex(1.0, 0.0)))
        self.assertTrue(complex.__bool__(complex(-1.0, 1.0)))

        self.assertFalse(complex.__bool__(complex(0.0, 0.0)))

    def test_dunder_hash_with_0_image_returns_float_hash(self):
        self.assertEqual(complex.__hash__(complex(0.0)), float.__hash__(0.0))
        self.assertEqual(complex.__hash__(complex(-0.0)), float.__hash__(-0.0))
        self.assertEqual(complex.__hash__(complex(1.0)), float.__hash__(1.0))
        self.assertEqual(complex.__hash__(complex(-1.0)), float.__hash__(-1.0))
        self.assertEqual(complex.__hash__(complex(42.0)), float.__hash__(42.0))
        self.assertEqual(complex.__hash__(complex(1e23)), float.__hash__(1e23))
        inf = float("inf")
        self.assertEqual(complex.__hash__(complex(inf)), float.__hash__(inf))
        nan = float("nan")
        self.assertEqual(complex.__hash__(complex(nan)), float.__hash__(nan))

    def test_dunder_hash_with_1_imag_returns_hash_info_imag(self):
        import sys

        imag_hash = sys.hash_info.imag
        self.assertEqual(complex.__hash__(1j), imag_hash)
        self.assertEqual(
            complex.__hash__(1.000000001j), float.__hash__(1.000000001) * imag_hash
        )
        self.assertEqual(
            complex.__hash__(complex(2.0, -3.0)),
            float.__hash__(2.0) + float.__hash__(-3.0) * imag_hash,
        )

    def test_dunder_eq_with_num_and_non_zero_imaginary_returns_false(self):
        self.assertFalse(complex(1, 1).__eq__(1))
        self.assertFalse(complex(1.0, 1).__eq__(1))

    def test_dunder_eq_with_num_compares_real_field(self):
        self.assertTrue(complex(2, 0).__eq__(2))
        self.assertTrue(complex(3.2, 0).__eq__(3.2))

    def test_dunder_eq_with_complex_compares_both_fields(self):
        self.assertTrue(complex(1.2, 3.4), complex(1.2, 3.4))
        self.assertFalse(complex(3.2, 0).__eq__(complex(3.2, 1)))
        self.assertFalse(complex(1, 1).__eq__(complex(3.2, 1)))

    def test_dunder_eq_with_non_num_or_complex_returns_false(self):
        self.assertIs(complex(3.2, 0).__eq__([3.2, 0]), NotImplemented)
        self.assertIs(complex(1, 1).__eq__("(1+1j)"), NotImplemented)

    def test_dunder_new_with_no_args_returns_complex_zero(self):
        c = complex()
        self.assertIsInstance(c, complex)
        self.assertEqual(c, 0)

    def test_dunder_new_with_int_returns_complex(self):
        c = complex(1)
        self.assertIsInstance(c, complex)
        self.assertEqual(c, 1)

    def test_dunder_new_with_float_returns_complex(self):
        c = complex(1.0)
        self.assertIsInstance(c, complex)
        self.assertEqual(c, 1.0)

    def test_dunder_new_with_complex_returns_same_complex(self):
        c1 = 1 + 2j
        c2 = complex(c1)
        self.assertIs(c1, c2)

    def test_dunder_new_with_str_returns_complex(self):
        c = complex("1")
        self.assertIsInstance(c, complex)
        self.assertEqual(c, 1)

    def test_dunder_new_with_str_subclass_returns_complex(self):
        class C(str):
            pass

        c = complex(C("1"))
        self.assertIsInstance(c, complex)
        self.assertEqual(c, 1)

    def test_dunder_new_with_str_subclass_does_not_call_dunder_float(self):
        class C(str):
            def __float__(self):
                return 2.0

        c = complex(C("1"))
        self.assertIsInstance(c, complex)
        self.assertEqual(c, 1)

    def test_dunder_new_with_empty_str_raises_value_error(self):
        with self.assertRaises(ValueError):
            complex("")

    def test_dunder_new_with_str_only_whitespace_raises_value_error(self):
        with self.assertRaises(ValueError):
            complex("    ")

    def test_dunder_new_with_str_and_leading_whitespace_returns_complex(self):
        c = complex("   1")
        self.assertIsInstance(c, complex)
        self.assertEqual(c, 1)

    def test_dunder_new_with_str_and_mismatched_parens_raises_value_error(self):
        with self.assertRaises(ValueError):
            complex("(1")

        with self.assertRaises(ValueError):
            complex(")1")

        with self.assertRaises(ValueError):
            complex("()1")

        with self.assertRaises(ValueError):
            complex("1()")

        with self.assertRaises(ValueError):
            complex("1(")

        with self.assertRaises(ValueError):
            complex("1)")

    def test_dunder_new_with_str_and_whitespace_inside_parens_returns_complex(self):
        c = complex("(   1)")
        self.assertIsInstance(c, complex)
        self.assertEqual(c, 1)

    def test_dunder_new_with_str_and_parens_inside_whitespace_returns_complex(self):
        c = complex("   (1)")
        self.assertIsInstance(c, complex)
        self.assertEqual(c, 1)

    def test_dunder_new_with_str_and_trailing_whitespace_returns_complex(self):
        c = complex("1   ")
        self.assertIsInstance(c, complex)
        self.assertEqual(c, 1)

    def test_dunder_new_with_str_and_leading_plus_returns_complex(self):
        c = complex("+1")
        self.assertIsInstance(c, complex)
        self.assertEqual(c, 1)

    def test_dunder_new_with_str_and_leading_minus_returns_complex(self):
        c = complex("-1")
        self.assertIsInstance(c, complex)
        self.assertEqual(c, -1)

    def test_dunder_new_with_str_and_e_returns_complex(self):
        c = complex("2e3")
        self.assertIsInstance(c, complex)
        self.assertEqual(c.real, 2000)
        self.assertEqual(c.imag, 0)

        c = complex("2E3")
        self.assertIsInstance(c, complex)
        self.assertEqual(c.real, 2000)
        self.assertEqual(c.imag, 0)

    def test_dunder_new_with_str_and_only_j_returns_complex(self):
        c = complex("j")
        self.assertIsInstance(c, complex)
        self.assertEqual(c.real, 0)
        self.assertEqual(c.imag, 1)

        c = complex("J")
        self.assertIsInstance(c, complex)
        self.assertEqual(c.real, 0)
        self.assertEqual(c.imag, 1)

    def test_dunder_new_with_str_and_only_imag_returns_complex(self):
        c = complex("2j")
        self.assertIsInstance(c, complex)
        self.assertEqual(c.real, 0)
        self.assertEqual(c.imag, 2)

    def test_dunder_new_with_str_and_real_and_imag_returns_complex(self):
        c = complex("1+2j")
        self.assertIsInstance(c, complex)
        self.assertEqual(c.real, 1)
        self.assertEqual(c.imag, 2)

        c = complex("1-2j")
        self.assertIsInstance(c, complex)
        self.assertEqual(c.real, 1)
        self.assertEqual(c.imag, -2)

    def test_dunder_new_with_str_and_exponent_imag_returns_complex(self):
        c = complex("1-2e3j")
        self.assertIsInstance(c, complex)
        self.assertEqual(c.real, 1)
        self.assertEqual(c.imag, -2000)

        c = complex("1-2E3j")
        self.assertIsInstance(c, complex)
        self.assertEqual(c.real, 1)
        self.assertEqual(c.imag, -2000)

    def test_dunder_new_with_str_and_fractional_real_and_imag_returns_complex(self):
        c = complex("1.234+5.678j")
        self.assertIsInstance(c, complex)
        self.assertEqual(c.real, 1.234)
        self.assertEqual(c.imag, 5.678)

    def test_dunder_new_with_str_and_imag_first_raises_value_error(self):
        with self.assertRaises(ValueError):
            complex("j+1")

    def test_dunder_new_with_str_and_imag_no_j_raises_value_error(self):
        with self.assertRaises(ValueError):
            complex("1-2")

    def test_dunder_new_with_str_and_sign_but_no_imag_raises_value_error(self):
        with self.assertRaises(ValueError):
            complex("1+")

        with self.assertRaises(ValueError):
            complex("1-")

    def test_dunder_new_with_subtype_returns_instance(self):
        class C(complex):
            pass

        c = C()
        self.assertIsInstance(c, complex)
        self.assertIs(type(c), C)

    def test_dunder_new_with_real_numbers_returns_complex(self):
        c = complex(1, 2.0)
        self.assertIsInstance(c, complex)
        self.assertEqual(c, 1 + 2j)

    def test_dunder_new_with_complex_numbers_returns_complex(self):
        c = complex(4 + 4j, 1 + 2j)
        self.assertIsInstance(c, complex)
        self.assertEqual(c, 2 + 5j)

    def test_dunder_new_with_string_real_and_imag_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            complex("foo", 1)
        self.assertEqual(
            str(context.exception),
            "complex() can't take second arg if first is a string",
        )

    def test_dunder_new_with_string_imag_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            complex(1, "foo")
        self.assertEqual(
            str(context.exception), "complex() second arg can't be a string"
        )

    def test_dunder_new_calls_dunder_complex(self):
        class C(int):
            def __complex__(self):
                return 1 + 0j

        c = complex(C())
        self.assertEqual(c, 1 + 0j)

    def test_dunder_new_with_non_complex_dunder_complex(self):
        class C:
            def __complex__(self):
                return 1

        with self.assertRaises(TypeError) as context:
            complex(C())
        self.assertEqual(
            str(context.exception), "__complex__ returned non-complex (type int)"
        )

    def test_dunder_new_with_bytes_real_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            complex(b"")
        self.assertEqual(
            str(context.exception),
            "complex() first argument must be a string or a number, not 'bytes'",
        )

    def test_dunder_new_with_bytes_imag_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            complex(1, b"")
        self.assertEqual(
            str(context.exception),
            "complex() second argument must be a number, not 'bytes'",
        )

    def test_dunder_neg_with_non_complex_raises_type_error(self):
        with self.assertRaises(TypeError):
            complex.__neg__(1.0)

    def test_dunder_neg_negates_both_fields_in_complex(self):
        self.assertEqual(complex(0, 0).__neg__(), complex(0, 0))
        self.assertEqual(complex(1, 2.2).__neg__(), complex(-1, -2.2))
        self.assertEqual(complex(-3.4, -5.6).__neg__(), complex(3.4, 5.6))

    def test_dunder_neg_with_complex_subclass_returns_complex(self):
        class A(complex):
            pass

        neg_complex_subclass = A(1.2, -2.1).__neg__()
        self.assertEqual(type(neg_complex_subclass), complex)
        self.assertEqual(neg_complex_subclass, complex(-1.2, 2.1))

    def test_dunder_pos_with_non_complex_raises_type_error(self):
        with self.assertRaises(TypeError):
            complex.__pos__(1.0)

    def test_dunder_pos_returns_unmodified_complex(self):
        self.assertEqual(complex(0, 0).__pos__(), complex(0, 0))
        self.assertEqual(complex(1, 2.2).__pos__(), complex(1, 2.2))
        self.assertEqual(complex(-3.4, -5.6).__pos__(), complex(-3.4, -5.6))

    def test_dunder_pos_with_complex_subclass_returns_complex(self):
        class A(complex):
            pass

        pos_complex_subclass = A(1, -2).__pos__()
        self.assertEqual(type(pos_complex_subclass), complex)
        self.assertEqual(pos_complex_subclass, complex(1, -2))

    def test_dunder_rsub_with_non_complex_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            complex.__rsub__(1, 2)

    def test_dunder_rsub_with_non_number_type_returns_not_implemented(self):
        self.assertIs(complex(3.2, 0).__rsub__("not-num"), NotImplemented)

    def test_dunder_rsub_subtracts_first_number_from_second_number(self):
        self.assertEqual(complex(1.0, 1.0).__rsub__(0), complex(-1, -1))
        self.assertEqual(complex(1.0, 1.0).__rsub__(1.0), complex(0, -1))
        self.assertEqual(complex(1.0, 1.0).__rsub__(complex(2, 2)), complex(1, 1))

    def test_dunder_rsub_with_int_subclass_returns_complex(self):
        class A(int):
            pass

        self.assertEqual(complex(1.0, 1.0).__rsub__(A(3)), complex(2, -1))

    def test_dunder_sub_with_non_complex_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            complex.__sub__(1, 2)

    def test_dunder_sub_with_non_number_type_returns_not_implemented(self):
        self.assertIs(complex(3.2, 0).__sub__("not-num"), NotImplemented)

    def test_dunder_sub_subtracts_first_number_from_second_number(self):
        self.assertEqual(complex(1.0, 1.0).__sub__(0), complex(1, 1))
        self.assertEqual(complex(1.0, 1.0).__sub__(1.0), complex(0, 1))
        self.assertEqual(complex(1.0, 1.0).__sub__(complex(2, 2)), complex(-1, -1))

    def test_dunder_sub_with_int_subclass_returns_complex(self):
        class A(int):
            pass

        self.assertEqual(complex(1.0, 1.0).__sub__(A(3)), complex(-2, 1))

    def test_imag_with_subclass_returns_float(self):
        class C(complex):
            pass

        c = C(1 + 2j)
        i = c.imag
        self.assertIsInstance(i, float)
        self.assertEqual(i, 2.0)

    def test_real_with_subclass_returns_float(self):
        class C(complex):
            pass

        c = C(1 + 2j)
        r = c.real
        self.assertIsInstance(r, float)
        self.assertEqual(r, 1.0)

    def test_dunder_mul_complex_returns_complex(self):
        self.assertEqual(complex(1, 2).__mul__(complex(-2, 3)), complex(-8, -1))
        self.assertEqual(complex(1, 2).__mul__(complex(2.5, -1)), complex(4.5, 4))
        self.assertEqual(complex(1, 2).__mul__(complex(0, 3)), complex(-6, 3))
        self.assertEqual(complex(1, 2).__mul__(complex(2, 0)), complex(2, 4))

    def test_dunder_mul_complex_with_nan_returns_nan_complex(self):
        import math

        res = complex(1, 2).__mul__(complex(2, float("nan")))
        self.assertTrue(math.isnan(res.real))
        self.assertTrue(math.isnan(res.imag))

    def test_dunder_rmul_complex_with_nan_returns_nan_complex(self):
        import math

        res = complex(1, 2).__rmul__(complex(2, float("nan")))
        self.assertTrue(math.isnan(res.real))
        self.assertTrue(math.isnan(res.imag))

    def test_dunder_rmul_complex_with_int_returns_complex(self):
        self.assertEqual(complex(1, 2).__rmul__(4), complex(4, 8))

    def test_dunder_rmul_complex_with_float_returns_complex(self):
        self.assertEqual(complex(1, 2).__rmul__(4.9), complex(4.9, 9.8))

    def test_dunder_rmul_complex_with_str_returns_notimplemented(self):
        self.assertIs(complex(1, 2).__rmul__("ciao"), NotImplemented)

    def test_dunder_rmul_complex_with_list_returns_notimplemented(self):
        self.assertIs(complex(1, 2).__rmul__([4]), NotImplemented)

    def test_dunder_div_complex_returns_complex(self):
        self.assertEqual(complex(1, 2).__truediv__(complex(1, 1)), complex(1.5, 0.5))
        self.assertEqual(
            complex(1, 2).__truediv__(complex(-2, 2)), complex(0.25, -0.75)
        )
        self.assertEqual(complex(2, 1).__truediv__(complex(0, 2)), complex(0.5, -1))
        self.assertEqual(complex(3, 2).__truediv__(complex(-0.5, 0)), complex(-6, -4))

    def test_dunder_div_complex_with_nan_returns_nan_complex(self):
        import math

        res = complex(1, 2).__truediv__(complex(2, float("nan")))
        self.assertTrue(math.isnan(res.real))
        self.assertTrue(math.isnan(res.imag))

    def test_dunder_div_zero_raises_error(self):
        with self.assertRaises(ZeroDivisionError):
            complex(1, 2).__truediv__(complex(0, 0))

    def test_dunder_repr_with_int_real_and_imag_returns_string(self):
        self.assertEqual(complex.__repr__(complex(1, 2)), "(1+2j)")
        self.assertEqual(complex.__repr__(complex(1, -2)), "(1-2j)")
        self.assertEqual(complex.__repr__(complex(-1, 2)), "(-1+2j)")
        self.assertEqual(complex.__repr__(complex(-1, -2)), "(-1-2j)")

    def test_dunder_repr_with_float_real_and_imag_returns_string(self):
        self.assertEqual(complex.__repr__(complex(1.1, 2.3)), "(1.1+2.3j)")
        self.assertEqual(complex.__repr__(complex(1.1, -2.3)), "(1.1-2.3j)")
        self.assertEqual(complex.__repr__(complex(-1.1, 2.3)), "(-1.1+2.3j)")
        self.assertEqual(complex.__repr__(complex(-1.1, -2.3)), "(-1.1-2.3j)")

    def test_dunder_repr_with_int_real_and_float_imag_returns_string(self):
        self.assertEqual(complex.__repr__(complex(1, 2.3)), "(1+2.3j)")
        self.assertEqual(complex.__repr__(complex(1, -2.3)), "(1-2.3j)")
        self.assertEqual(complex.__repr__(complex(-1, 2.3)), "(-1+2.3j)")
        self.assertEqual(complex.__repr__(complex(-1, -2.3)), "(-1-2.3j)")

    def test_dunder_repr_with_float_real_and_int_imag_returns_string(self):
        self.assertEqual(complex.__repr__(complex(1.1, 2)), "(1.1+2j)")
        self.assertEqual(complex.__repr__(complex(1.1, -2)), "(1.1-2j)")
        self.assertEqual(complex.__repr__(complex(-1.1, 2)), "(-1.1+2j)")
        self.assertEqual(complex.__repr__(complex(-1.1, -2)), "(-1.1-2j)")

    def test_dunder_repr_with_positive_zero_real_returns_string(self):
        self.assertEqual(complex.__repr__(complex(0, 2.3)), "2.3j")
        self.assertEqual(complex.__repr__(complex(0, -2.3)), "-2.3j")
        self.assertEqual(complex.__repr__(complex(0, 2)), "2j")
        self.assertEqual(complex.__repr__(complex(0, -2)), "-2j")

    def test_dunder_repr_with_negative_zero_real_returns_string(self):
        self.assertEqual(complex.__repr__(complex(-0.0, 2.3)), "(-0+2.3j)")
        self.assertEqual(complex.__repr__(complex(-0.0, -2.3)), "(-0-2.3j)")
        self.assertEqual(complex.__repr__(complex(-0.0, 2)), "(-0+2j)")
        self.assertEqual(complex.__repr__(complex(-0.0, -2)), "(-0-2j)")

    def test_dunder_repr_with_non_complex_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            complex.__repr__(1.0)
        with self.assertRaises(TypeError):
            complex.__repr__("a")
        with self.assertRaises(TypeError):
            complex.__repr__(None)

    def test_dunder_lt_with_complex_returns_not_implemented(self):
        self.assertIs(complex(1, 1).__lt__(complex(1, 1)), NotImplemented)
        self.assertIs(complex(1, 1).__lt__(float(1)), NotImplemented)

    def test_dunder_le_with_complex_returns_not_implemented(self):
        self.assertIs(complex(1, 1).__le__(complex(1, 1)), NotImplemented)
        self.assertIs(complex(1, 1).__le__(float(1)), NotImplemented)

    def test_dunder_gt_with_complex_returns_not_implemented(self):
        self.assertIs(complex(1, 1).__gt__(complex(1, 1)), NotImplemented)
        self.assertIs(complex(1, 1).__gt__(float(1)), NotImplemented)

    def test_dunder_ge_with_complex_returns_not_implemented(self):
        self.assertIs(complex(1, 1).__ge__(complex(1, 1)), NotImplemented)
        self.assertIs(complex(1, 1).__ge__(float(1)), NotImplemented)


class CoroutineTests(unittest.TestCase):
    class MyError(Exception):
        pass

    class Awaitable:
        def __init__(self, yield_value):
            self._yield_value = yield_value

        def __await__(self):
            yield self._yield_value

    @staticmethod
    async def simple_coro():
        await CoroutineTests.Awaitable(1)
        await CoroutineTests.Awaitable(2)

    @staticmethod
    async def catching_coro():
        try:
            await CoroutineTests.Awaitable(1)
        except CoroutineTests.MyError:
            await CoroutineTests.Awaitable("caught")

    @staticmethod
    async def catching_returning_coro():
        try:
            await CoroutineTests.Awaitable(1)
        except CoroutineTests.MyError:
            return "all done!"  # noqa

    def test_calling_iter_raises_type_error(self):
        with contextlib.closing(self.simple_coro()) as g:
            with self.assertRaises(TypeError):
                iter(g)

    def test_calling_next_raises_type_error(self):
        with contextlib.closing(self.simple_coro()) as g:
            with self.assertRaises(TypeError):
                next(g)

    def test_dunder_await_with_invalid_self_raises_type_error(self):
        with contextlib.closing(self.simple_coro()) as g:
            with self.assertRaises(TypeError):
                type(g).__await__(None)

    def test_dunder_await_returns_iterable(self):
        it = self.simple_coro().__await__()
        self.assertEqual(next(it), 1)
        self.assertEqual(next(it), 2)

    def test_close_with_invalid_self_raises_type_error(self):
        with contextlib.closing(self.simple_coro()) as g:
            with self.assertRaises(TypeError):
                type(g).close(None)

    def test_close_when_exhausted_returns_none(self):
        g = self.simple_coro()
        self.assertEqual(g.send(None), 1)
        self.assertEqual(g.send(None), 2)
        self.assertRaises(StopIteration, g.send, None)
        self.assertIsNone(g.close())

    def test_close_when_generator_exit_propagates_returns_none(self):
        saw_generator_exit = False

        async def f():
            nonlocal saw_generator_exit
            try:
                await CoroutineTests.Awaitable(1)
            except GeneratorExit:
                saw_generator_exit = True
                raise

        g = f()
        self.assertEqual(g.send(None), 1)
        self.assertIsNone(g.close())
        self.assertTrue(saw_generator_exit)

    def test_close_when_generator_exit_derived_exception_raised_returns_none(self):
        class GeneratorExitDerived(GeneratorExit):
            pass

        async def f():
            try:
                await CoroutineTests.Awaitable(1)
            except GeneratorExit:
                raise GeneratorExitDerived

        g = f()
        self.assertEqual(g.send(None), 1)
        self.assertIsNone(g.close())

    def test_close_when_stop_iteration_raised_returns_none(self):
        saw_generator_exit = False

        async def f():
            nonlocal saw_generator_exit
            try:
                await CoroutineTests.Awaitable(1)
            except GeneratorExit:
                saw_generator_exit = True
                # Implicitly raises StopIteration(2)
                return 2

        g = f()
        self.assertEqual(g.send(None), 1)
        self.assertIsNone(g.close())
        self.assertTrue(saw_generator_exit)

    def test_close_generator_raises_exception_propagates(self):
        async def f():
            try:
                await CoroutineTests.Awaitable(1)
            except GeneratorExit:
                raise ValueError

        g = f()
        self.assertEqual(g.send(None), 1)
        self.assertRaises(ValueError, g.close)

    def test_close_generator_awaits_raises_runtime_error(self):
        async def f():
            try:
                await CoroutineTests.Awaitable(1)
            except GeneratorExit:
                await CoroutineTests.Awaitable(2)

        g = f()
        self.assertEqual(g.send(None), 1)
        self.assertRaises(RuntimeError, g.close)

    def test_throw(self):
        g = self.simple_coro()
        self.assertRaises(CoroutineTests.MyError, g.throw, CoroutineTests.MyError())

    def test_throw_caught(self):
        g = self.catching_coro()
        self.assertEqual(g.send(None), 1)
        self.assertEqual(g.throw(CoroutineTests.MyError()), "caught")

    def test_throw_type(self):
        g = self.catching_coro()
        self.assertEqual(g.send(None), 1)
        self.assertEqual(g.throw(CoroutineTests.MyError), "caught")

    def test_throw_type_and_value(self):
        g = self.catching_coro()
        self.assertEqual(g.send(None), 1)
        self.assertEqual(
            g.throw(CoroutineTests.MyError, CoroutineTests.MyError()), "caught"
        )

    def test_throw_uncaught_type(self):
        g = self.catching_coro()
        self.assertEqual(g.send(None), 1)
        self.assertRaises(RuntimeError, g.throw, RuntimeError)

    def test_throw_finished(self):
        g = self.catching_returning_coro()
        self.assertEqual(g.send(None), 1)
        self.assertRaises(StopIteration, g.throw, CoroutineTests.MyError)

    def test_throw_two_values(self):
        g = self.catching_coro()
        self.assertEqual(g.send(None), 1)
        self.assertRaises(
            TypeError, g.throw, CoroutineTests.MyError(), CoroutineTests.MyError()
        )

    def test_throw_bad_traceback(self):
        g = self.catching_coro()
        self.assertEqual(g.send(None), 1)
        self.assertRaises(
            TypeError, g.throw, CoroutineTests.MyError, CoroutineTests.MyError(), 5
        )

    def test_throw_bad_type(self):
        g = self.catching_coro()
        self.assertEqual(g.send(None), 1)
        self.assertRaises(TypeError, g.throw, 1234)

    def test_coroutine_awaiting_on_further_coroutine_with_no_arguments(self):
        async def f():
            pass

        async def g():
            return await f()

        # Send in None to trigger initial execution of coroutine
        with self.assertRaises(StopIteration) as exc:
            g().send(None)

        self.assertIs(exc.exception.value, None)

    def test_coroutine_awaiting_on_further_coroutine_with_an_argument(self):
        v = "foo"

        async def f(v):
            return v

        async def g():
            return await f(v)

        # Send in None to trigger initial execution of coroutine
        with self.assertRaises(StopIteration) as exc:
            g().send(None)

        self.assertIs(exc.exception.value, v)

    def test_stop_iteration_raised_from_coroutine_turns_into_runtime_error(self):
        async def f():
            raise StopIteration

        # Send in None to trigger initial execution of coroutine
        with self.assertRaises(RuntimeError):
            f().send(None)

    def test_stop_iteration_raised_from_coroutine_but_caught_can_return_a_value(self):
        async def f():
            try:
                raise StopIteration
            except StopIteration:
                return 1

        # Send in None to trigger initial execution of coroutine
        with self.assertRaises(StopIteration) as exc:
            f().send(None)

        self.assertEqual(exc.exception.value, 1)

    def test_stop_iteration_raised_from_inner_coroutine_turns_into_runtime_error(self):
        async def f():
            raise StopIteration

        async def g():
            return await f()

        # Send in None to trigger initial execution of coroutine
        with self.assertRaises(RuntimeError):
            g().send(None)

    def test_sub_classed_stop_iteration_raised_from_coroutine_turns_into_runtime_error(
        self,
    ):
        class SubClassedStopIteration(AttributeError, StopIteration):
            pass

        async def f():
            raise SubClassedStopIteration

        # Send in None to trigger initial execution of coroutine
        with self.assertRaises(RuntimeError):
            f().send(None)

    def test_yield_from_coroutine_in_non_coroutine_iter_raises_exception(self):
        async def coro():
            pass

        def f(coro_inst):
            yield from coro_inst

        with contextlib.closing(coro()) as coro_inst:
            with self.assertRaises(TypeError):
                f(coro_inst).send(None)

    def test_awaiting_already_awaited_coroutine_raises_runtime_error(self):
        class Awaitable:
            def __await__(self):
                return self

            def __next__(self):
                return None

        async def f():
            await Awaitable()

        coro = f()

        async def g():
            nonlocal coro
            await coro

        coro.send(None)

        with self.assertRaisesRegex(RuntimeError, "coroutine is being awaited already"):
            g().send(None)

    def test_send_into_finished_coro_raises_runtime_error(self):
        g = self.simple_coro()
        self.assertEqual(g.send(None), 1)
        self.assertEqual(g.send(None), 2)
        with self.assertRaises(StopIteration):
            g.send(None)
        with self.assertRaises(RuntimeError):
            g.send(None)

    def test_throw_into_finshed_coro_raises_runtime_error(self):
        g = self.simple_coro()
        self.assertEqual(g.send(None), 1)
        self.assertEqual(g.send(None), 2)
        with self.assertRaises(StopIteration):
            g.send(None)
        with self.assertRaises(RuntimeError):
            g.throw(CoroutineTests.MyError())

    def test_close_finshed_coro_raises_is_no_op(self):
        g = self.simple_coro()
        self.assertEqual(g.send(None), 1)
        self.assertEqual(g.send(None), 2)
        with self.assertRaises(StopIteration):
            g.send(None)
        self.assertIsNone(g.close())

    def test_exception_context_captured_across_further_awaits(self):
        async def raises():
            raise ValueError

        async def double_raises():
            try:
                raise RuntimeError
            except RuntimeError:
                await raises()

        with self.assertRaises(ValueError) as exc:
            double_raises().send(None)

        self.assertIsInstance(exc.exception.__context__, RuntimeError)

    def test_coroutine_returning_explicit_stop_iteration_value_passes_through_unchanged(
        self,
    ):
        v = StopIteration(10)

        async def f():
            return v

        # Send in None to trigger initial execution of coroutine
        with self.assertRaises(StopIteration) as exc:
            f().send(None)

        self.assertIs(exc.exception.value, v)
        self.assertEqual(exc.exception.args, (v,))

    def test_coroutine_returning_stop_iteration_sub_class_passes_through_unchanged(
        self,
    ):
        class SubClassedStopIteration(StopIteration):
            pass

        v = SubClassedStopIteration(1)

        async def f():
            return v

        # Send in None to trigger initial execution of coroutine
        with self.assertRaises(StopIteration) as exc:
            f().send(None)

        self.assertIs(exc.exception.value, v)
        self.assertEqual(exc.exception.args, (v,))

    def test_coroutine_returning_tuple_passes_through_as_tuple(self):
        v = (1,)

        async def f():
            return v

        # Send in None to trigger initial execution of coroutine
        with self.assertRaises(StopIteration) as exc:
            f().send(None)

        self.assertIs(exc.exception.value, v)
        self.assertEqual(exc.exception.args, (v,))

    def test_coroutine_returning_tuple_sub_class_passes_through_unchanged(self):
        class NewTuple(tuple):
            pass

        v = NewTuple()

        async def f():
            return v

        # Send in None to trigger initial execution of coroutine
        with self.assertRaises(StopIteration) as exc:
            f().send(None)

        self.assertIs(exc.exception.value, v)
        self.assertEqual(exc.exception.args, (v,))

    def test_coroutine_returning_arbitrary_exception_does_not_get_context_updated(self):
        value_error = ValueError()

        async def f():
            nonlocal value_error
            return value_error

        async def g():
            try:
                raise KeyError
            except KeyError:
                return await f()

        self.assertIsNone(value_error.__context__)
        with self.assertRaises(StopIteration) as exc:
            g().send(None)
        self.assertIs(exc.exception.value, value_error)
        self.assertIsNone(exc.exception.value.__context__)


class CoroutineWrapperTests(unittest.TestCase):
    def test_dunder_iter_with_invalid_self_raises_type_error(self):
        async def f():
            pass

        with contextlib.closing(f()) as g:
            with self.assertRaises(TypeError):
                type(g.__await__()).__iter__(None)

    def test_dunder_next_with_invalid_self_raises_type_error(self):
        async def f():
            pass

        with contextlib.closing(f()) as g:
            with self.assertRaises(TypeError):
                type(g.__await__()).__next__(None)

    def test_close_with_invalid_self_raises_type_error(self):
        async def f():
            pass

        with contextlib.closing(f()) as g:
            with self.assertRaises(TypeError):
                type(g.__await__()).close(None)

    def test_send_with_invalid_self_raises_type_error(self):
        async def f():
            pass

        with contextlib.closing(f()) as g:
            with self.assertRaises(TypeError):
                type(g.__await__()).send(None, None)

    def test_throw_with_invalid_self_raises_type_error(self):
        async def f():
            pass

        with contextlib.closing(f()) as g:
            with self.assertRaises(TypeError):
                type(g.__await__()).throw(None, None, None, None)

    def test_dunder_iter_returns_self(self):
        async def f():
            pass

        with contextlib.closing(f()) as g:
            wrapper = g.__await__()
            self.assertIs(wrapper.__iter__(), wrapper)

    def test_dunder_next_raises_stop_iteration_with_yielded_value(self):
        async def f():
            return 1

        with contextlib.closing(f()) as g:
            with self.assertRaises(StopIteration) as exc:
                g.__await__().__next__()
        self.assertEqual(exc.exception.value, 1)

    def test_dunder_repr(self):
        async def f():
            return 1

        with contextlib.closing(f()) as g:
            self.assertTrue(
                g.__await__().__repr__().startswith("<coroutine_wrapper object at ")
            )

    def test_close_throws_in_generator_exit(self):
        saw_generator_exit = False

        async def f():
            nonlocal saw_generator_exit

            class Awaitable:
                def __await__(self):
                    yield 1

            try:
                await Awaitable()
            except GeneratorExit:
                saw_generator_exit = True

        g = f()
        self.assertEqual(g.send(None), 1)
        g.__await__().close()
        self.assertTrue(saw_generator_exit)

    def test_send_raises_stop_iteration_with_yielded_value(self):
        async def f():
            return 1

        with contextlib.closing(f()) as g:
            with self.assertRaises(StopIteration) as exc:
                g.__await__().send(None)
        self.assertEqual(exc.exception.value, 1)

    def test_throw_raises_in_generator(self):
        saw_value_error = False

        async def f():
            nonlocal saw_value_error

            class Awaitable:
                def __await__(self):
                    yield 1

            try:
                await Awaitable()
            except ValueError:
                saw_value_error = True

        with contextlib.closing(f()) as g:
            self.assertEqual(g.send(None), 1)
            with self.assertRaises(StopIteration):
                g.__await__().throw(ValueError, None)
        self.assertTrue(saw_value_error)


class DelattrTests(unittest.TestCase):
    def test_non_str_as_name_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            delattr("str instance", None)
        self.assertIn(
            "attribute name must be string, not 'NoneType'", str(context.exception)
        )

    def test_delete_non_existing_instance_attribute_raises_attribute_error(self):
        class C:
            fld = 4

        c = C()
        with self.assertRaises(AttributeError):
            delattr(c, "fld")

    def test_calls_dunder_delattr_and_returns_none_if_successful(self):
        class C:
            def __init__(self):
                self.fld = ""

            def __delattr__(self, name):
                self.fld = name
                return "unused value"

        c = C()
        self.assertIs(delattr(c, "passed to __delattr__"), None)
        self.assertEqual(c.fld, "passed to __delattr__")

    def test_passes_exception_raised_by_dunder_delattr(self):
        class C:
            def __delattr__(self, name):
                raise UserWarning("delattr failed")

        c = C()
        with self.assertRaises(UserWarning) as context:
            delattr(c, "foo")
        self.assertIn("delattr failed", str(context.exception))

    def test_deletes_existing_instance_attribute(self):
        class C:
            def __init__(self):
                self.fld = 0

        c = C()
        self.assertTrue(hasattr(c, "fld"))
        delattr(c, "fld")
        self.assertFalse(hasattr(c, "fld"))

    def test_accepts_str_subclass_as_name(self):
        class C:
            def __init__(self):
                self.fld = 0

        class S(str):
            pass

        c = C()
        self.assertTrue(hasattr(c, "fld"))
        delattr(c, S("fld"))
        self.assertFalse(hasattr(c, "fld"))


class DirTests(unittest.TestCase):
    def test_without_args_returns_locals_keys(self):
        def foo():
            a = 4
            a = a  # noqa: F841
            b = 5
            b = b  # noqa: F841
            return dir()

        result = foo()
        self.assertIsInstance(result, list)
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0], "a")
        self.assertEqual(result[1], "b")

    def test_with_arg_returns_dunder_dir_result(self):
        class C:
            def __dir__(self):
                return ["1", "2"]

        c = C()
        self.assertEqual(dir(c), ["1", "2"])


@unittest.skipIf(
    sys.implementation.name == "cpython" and sys.version_info < (3, 7),
    "requires at least CPython 3.7",
)
class DunderBuildClassTests(unittest.TestCase):
    def test_bases_updated_with_dunder_mro_entries(self):
        observed_bases = []

        class GenericAlias:
            def __init__(self, origin, item):
                self.origin = origin
                self.item = item

            def __mro_entries__(self, bases):
                nonlocal observed_bases
                observed_bases.append(bases)
                return (self.origin,)

            def __eq__(self, other):
                return self.origin == other.origin and self.item == other.item

        class NewList:
            def __class_getitem__(cls, item):
                return GenericAlias(cls, item)

        class NewTuple:
            def __class_getitem__(cls, item):
                return GenericAlias(cls, item)

        class Tokens(NewList[int], NewTuple[float]):
            pass

        self.assertIs(type(Tokens.__orig_bases__), tuple)
        self.assertEqual(Tokens.__orig_bases__, (NewList[int], NewTuple[float]))
        self.assertIs(type(Tokens.__bases__), tuple)
        self.assertEqual(Tokens.__bases__, (NewList, NewTuple))
        self.assertEqual(Tokens.__mro__, (Tokens, NewList, NewTuple, object))

        self.assertEqual(observed_bases, [Tokens.__orig_bases__, Tokens.__orig_bases__])

    def test_bases_with_non_class_invokes_metaclass(self):
        class Meta:  # noqa: B903
            def __init__(self, *args, **kwargs):
                self.init_args = args
                self.init_kwargs = kwargs

        pseudo_type = Meta()

        class C(pseudo_type):
            pass

        self.assertIsInstance(C, Meta)
        self.assertIsNot(C, pseudo_type)
        self.assertEqual(len(C.init_args), 3)
        self.assertEqual(C.init_args[0], "C")
        self.assertEqual(C.init_args[1], (pseudo_type,))
        self.assertIsInstance(C.init_args[2], dict)
        self.assertEqual(C.init_kwargs, {})

    def test_exceptions_propagated_from_dunder_mro_entries(self):
        class GenericAlias:
            def __init__(self, origin, item):
                pass

            def __mro_entries__(self, bases):
                raise UserWarning("GenericAlias.__mro__entries__")

        class NewList:
            def __class_getitem__(cls, item):
                return GenericAlias(cls, item)

        class NewTuple:
            def __class_getitem__(cls, item):
                return GenericAlias(cls, item)

        with self.assertRaises(UserWarning) as context:

            class Tokens(NewList[int]):
                pass

        self.assertIn("GenericAlias.__mro__entries__", str(context.exception))


class DunderSlotsTests(unittest.TestCase):
    def test_dunder_slots_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            class C:
                __slots__ = "a", 1

        self.assertIn(
            "__slots__ items must be strings, not 'int'", str(context.exception)
        )

    def test_dunder_slots_with_non_identifier_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            class C:
                __slots__ = "a", ";;"

        self.assertIn("__slots__ must be identifiers", str(context.exception))

    def test_dunder_slots_with_duplicated_dunder_dict_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            class C:
                __slots__ = "a", "__dict__", "__dict__"

        self.assertIn(
            "__dict__ slot disallowed: we already got one", str(context.exception)
        )

    def test_dunder_slots_with_conflicting_name_raises_value_error(self):
        with self.assertRaises(ValueError) as context:

            class C:
                __slots__ = "a"
                a = "conflicting"

        self.assertIn(
            "'a' in __slots__ conflicts with class variable", str(context.exception)
        )

    def test_dunder_slots_without_dunder_dict_removes_dunder_dict_type_attribute(self):
        class C:
            __slots__ = "a"

        self.assertNotIn("__dict__", C.__dict__)

    def test_dunder_slots_with_dunder_dict_keeps_dunder_dict_type_attribute(self):
        class C:
            __slots__ = "a", "__dict__"

        self.assertIn("__dict__", C.__dict__)

    def test_slot_descriptor_dunder_get_with_none_returns_type(self):
        class C:
            __slots__ = "a"

        descriptor = C.__dict__["a"]
        slot_descriptor_type = type(descriptor)
        self.assertIs(descriptor.__get__(None, slot_descriptor_type), descriptor)

    def test_slot_descriptor_dunder_get_with_instance_returns_attribute(self):
        class C:
            __slots__ = "a"

            def __init__(self):
                self.a = 10

        descriptor = C.__dict__["a"]
        instance = C()
        self.assertEqual(descriptor.__get__(instance), 10)

    def test_slot_descriptor_dunder_get_with_instance_and_none_owner_returns_attribute(
        self,
    ):
        class C:
            __slots__ = "a"

            def __init__(self):
                self.a = 10

        descriptor = C.__dict__["a"]
        instance = C()
        self.assertEqual(descriptor.__get__(instance, None), 10)

    def test_slot_descriptor_dunder_get_with_none_instance_and_none_owner_raises_type_error(
        self,
    ):
        class C:
            __slots__ = "a"

        descriptor = C.__dict__["a"]

        with self.assertRaises(TypeError) as context:
            descriptor.__get__(None, None)
        self.assertEqual(
            str(context.exception),
            "__get__(None, None) is invalid",
        )

    def test_slot_descriptor_dunder_delete_returns_none(self):
        class C:
            __slots__ = "a"

            def __init__(self):
                self.a = 10

        descriptor = C.__dict__["a"]
        instance = C()
        self.assertEqual(descriptor.__delete__(instance), None)

    def test_slot_descriptor_dunder_delete_none_raises_type_error(self):
        class C:
            __slots__ = "a"

        descriptor = C.__dict__["a"]

        with self.assertRaises(TypeError) as context:
            descriptor.__delete__(None)
        self.assertEqual(
            str(context.exception),
            "descriptor 'a' for 'C' objects doesn't apply to 'NoneType' object",
        )

    def test_dunder_slots_creates_type_attributes(self):
        class C:
            __slots__ = "a", "b", "c"

        self.assertIn("a", C.__dict__)
        self.assertIn("b", C.__dict__)
        self.assertIn("c", C.__dict__)

        slot_descriptor_type = type(C.a)
        self.assertTrue(hasattr(slot_descriptor_type, "__get__"))
        self.assertTrue(hasattr(slot_descriptor_type, "__set__"))
        self.assertIsInstance(C.a, slot_descriptor_type)
        self.assertIsInstance(C.b, slot_descriptor_type)
        self.assertIsInstance(C.c, slot_descriptor_type)

    def test_dunder_slots_sharing_same_layout_base_can_servce_as_bases(self):
        class C:
            __slots__ = "x"

        class D(C):
            pass

        class E(C):
            __slots__ = "y"

        class F(D, E):
            pass

        self.assertTrue(hasattr(F, "x"))
        self.assertTrue(hasattr(F, "y"))

    def test_dunder_slots_attributes_raises_attribute_error_before_set(self):
        class C:
            __slots__ = "x"

        c = C()
        self.assertFalse(hasattr(c, "x"))
        with self.assertRaises(AttributeError):
            c.x

    def test_dunder_slots_attributes_return_set_attributes(self):
        class C:
            __slots__ = "x"

        c = C()
        c.x = 500
        self.assertEqual(c.x, 500)

    def test_dunder_slots_attributes_raises_attribute_error_after_deletion(self):
        class C:
            __slots__ = "x"

        c = C()
        c.x = 50
        del c.x
        with self.assertRaises(AttributeError):
            c.x

    def test_dunder_slots_attributes_delete_raises_attribute_before_set(self):
        class C:
            __slots__ = "x"

        c = C()
        with self.assertRaises(AttributeError):
            del c.x

    def test_dunder_slots_with_sealed_base_seals_type(self):
        class C:
            __slots__ = ()

        c = C()
        with self.assertRaises(AttributeError):
            c.x = 500
        self.assertFalse(hasattr(c, "__dict__"))

    def test_dunder_slots_with_unsealed_base_unseals_type(self):
        class C:
            pass

        class D(C):
            __slots__ = ()

        d = D()
        d.x = 500
        self.assertEqual(d.x, 500)
        self.assertTrue(hasattr(d, "__dict__"))

    def test_dunder_slots_inherit_from_bases(self):
        class C:
            __slots__ = ()

        class D(C):
            __slots__ = "x", "y"

        class E(D, C):
            __slots__ = "z"

        e = E()
        e.x = 1
        e.y = 2
        e.z = 3
        self.assertEqual(e.x, 1)
        self.assertEqual(e.y, 2)
        self.assertEqual(e.z, 3)
        with self.assertRaises(AttributeError):
            e.w = 500
        self.assertFalse(hasattr(e, "__dict__"))

    def test_dunder_slots_inherit_from_builtin_type(self):
        class C(dict):
            __slots__ = "x"

        c = C()
        c.x = 500
        c["p"] = 4
        c["q"] = 5
        self.assertEqual(c.x, 500)
        self.assertEqual(c["p"], 4)
        self.assertEqual(c["q"], 5)

    def test_dunder_slots_are_independent_from_inherited_slots(self):
        class C:
            __slots__ = ("a", "c")

        class D(C):
            __slots__ = ("b", "c")

        obj = D()
        obj.a = 1
        obj.b = 2
        obj.c = 3
        C.c.__set__(obj, 33)

        self.assertEqual(obj.a, 1)
        self.assertEqual(C.a.__get__(obj), 1)
        self.assertEqual(D.a.__get__(obj), 1)
        self.assertEqual(obj.b, 2)
        with self.assertRaises(AttributeError):
            C.b
        self.assertEqual(D.b.__get__(obj), 2)
        self.assertEqual(obj.c, 3)
        self.assertEqual(C.c.__get__(obj), 33)
        self.assertEqual(D.c.__get__(obj), 3)

    def test_dunder_slots_member_descriptor_works_only_for_subtypes(self):
        class C:
            __slots__ = "x"

        class D(C):
            __slots__ = "y"

        class E:
            __slots__ = "x"

        d = D()
        # C's member_descriptor for "x" works for D instances.
        C.x.__set__(d, 500)
        self.assertEqual(C.x.__get__(d), 500)
        self.assertEqual(d.x, 500)

        e = E()
        with self.assertRaises(TypeError) as context:
            C.x.__set__(e, 500)
        self.assertEqual(
            str(context.exception),
            "descriptor 'x' for 'C' objects doesn't apply to 'E' object",
        )

        e.x = 600
        with self.assertRaises(TypeError) as context:
            C.x.__get__(e)
        self.assertEqual(
            str(context.exception),
            "descriptor 'x' for 'C' objects doesn't apply to 'E' object",
        )


class EllipsisTypeTests(unittest.TestCase):
    def test_repr_returns_not_implemented(self):
        self.assertEqual(Ellipsis.__repr__(), "Ellipsis")


class ExceptionTests(unittest.TestCase):
    def test_oserror_with_errno_enoent_returns_file_not_found_error(self):
        result = OSError(errno.ENOENT, "foo", "bar.py")
        self.assertIsInstance(result, FileNotFoundError)
        self.assertEqual(result.errno, errno.ENOENT)
        self.assertEqual(result.strerror, "foo")
        self.assertEqual(result.filename, "bar.py")

    def test_oserror_with_not_whitelisted_errno_returns_os_error(self):
        result = OSError(999, "foo", "bar.py")
        self.assertIsInstance(result, OSError)
        self.assertEqual(result.errno, 999)
        self.assertEqual(result.strerror, "foo")
        self.assertEqual(result.filename, "bar.py")

    def test_oserror_with_errno_eacces_returns_permission_error(self):
        result = OSError(errno.EACCES, "foo", "bar.py")
        self.assertIsInstance(result, PermissionError)

    def test_oserror_with_errno_eagain_returns_blocking_io_error(self):
        result = OSError(errno.EAGAIN, "foo", "bar.py")
        self.assertIsInstance(result, BlockingIOError)

    def test_oserror_with_errno_ealready_returns_blocking_io_error(self):
        result = OSError(errno.EALREADY, "foo", "bar.py")
        self.assertIsInstance(result, BlockingIOError)

    def test_oserror_with_errno_einprogress_returns_blocking_io_error(self):
        result = OSError(errno.EINPROGRESS, "foo", "bar.py")
        self.assertIsInstance(result, BlockingIOError)

    def test_oserror_with_errno_echild_returns_child_process_error(self):
        result = OSError(errno.ECHILD, "foo", "bar.py")
        self.assertIsInstance(result, ChildProcessError)

    def test_oserror_with_errno_econnaborted_returns_connection_aborted_error(self):
        result = OSError(errno.ECONNABORTED, "foo", "bar.py")
        self.assertIsInstance(result, ConnectionAbortedError)

    def test_oserror_with_errno_econnrefused_returns_connection_refused_error(self):
        result = OSError(errno.ECONNREFUSED, "foo", "bar.py")
        self.assertIsInstance(result, ConnectionRefusedError)

    def test_oserror_with_errno_econnreset_returns_connection_reset_error(self):
        result = OSError(errno.ECONNRESET, "foo", "bar.py")
        self.assertIsInstance(result, ConnectionResetError)

    def test_oserror_with_errno_eexist_returns_file_exists_error(self):
        result = OSError(errno.EEXIST, "foo", "bar.py")
        self.assertIsInstance(result, FileExistsError)

    def test_oserror_with_errno_eintr_returns_interrupted_error(self):
        result = OSError(errno.EINTR, "foo", "bar.py")
        self.assertIsInstance(result, InterruptedError)

    def test_oserror_with_errno_eisdir_returns_is_a_directory_error(self):
        result = OSError(errno.EISDIR, "foo", "bar.py")
        self.assertIsInstance(result, IsADirectoryError)

    def test_oserror_with_errno_enotdir_returns_not_a_directory_error(self):
        result = OSError(errno.ENOTDIR, "foo", "bar.py")
        self.assertIsInstance(result, NotADirectoryError)

    def test_oserror_with_errno_eperm_returns_permission_error(self):
        result = OSError(errno.EPERM, "foo", "bar.py")
        self.assertIsInstance(result, PermissionError)

    def test_oserror_with_errno_epipe_returns_broken_pipe_error(self):
        result = OSError(errno.EPIPE, "foo", "bar.py")
        self.assertIsInstance(result, BrokenPipeError)

    def test_oserror_with_esrch_returns_process_lookup_error(self):
        result = OSError(errno.ESRCH, "foo", "bar.py")
        self.assertIsInstance(result, ProcessLookupError)

    def test_oserror_with_errno_etimedout_returns_timeout_error(self):
        result = OSError(errno.ETIMEDOUT, "foo", "bar.py")
        self.assertIsInstance(result, TimeoutError)

    def test_oserror_with_errno_ewouldblock_returns_blocking_io_error(self):
        result = OSError(errno.EWOULDBLOCK, "foo", "bar.py")
        self.assertIsInstance(result, BlockingIOError)

    def test_oserror_dunder_new_with_subclass_return_subclass_instance(self):
        result = OSError.__new__(FileNotFoundError)
        self.assertIsInstance(result, FileNotFoundError)

    def test_oserror_dunder_init_with_no_args_does_not_set_attrs(self):
        exc = OSError()
        self.assertIs(exc.errno, None)
        self.assertIs(exc.strerror, None)
        self.assertIs(exc.filename, None)
        self.assertIs(exc.filename2, None)

    def test_oserror_dunder_init_with_args_sets_attrs(self):
        exc = OSError(1, "some error", "file", 0, "file2")
        self.assertIs(exc.errno, 1)
        self.assertIs(exc.strerror, "some error")
        self.assertIs(exc.filename, "file")
        self.assertIs(exc.filename2, "file2")

    def test_oserror_dunder_str_without_errno_or_strerror(self):
        exc = OSError()
        self.assertEqual(OSError.__str__(exc), "")

        exc = OSError(999)
        self.assertEqual(OSError.__str__(exc), "999")

    def test_oserror_dunder_str_without_filename(self):
        exc = OSError(999, "some error")
        self.assertEqual(OSError.__str__(exc), "[Errno 999] some error")

    def test_oserror_dunder_str_with_filename(self):
        exc = OSError(999, "some error", "file")
        self.assertEqual(OSError.__str__(exc), "[Errno 999] some error: 'file'")

    def test_oserror_dunder_str_with_filename_and_filename2(self):
        exc = OSError(999, "some error", "file", 0, "file2")
        self.assertEqual(
            OSError.__str__(exc), "[Errno 999] some error: 'file' -> 'file2'"
        )

    def test_maybe_unbound_attributes(self):
        exc = BaseException()
        exc2 = BaseException()
        self.assertIs(exc.__cause__, None)
        self.assertIs(exc.__context__, None)
        self.assertIs(exc.__traceback__, None)

        # Test setter for __cause__.
        self.assertRaises(TypeError, setattr, exc, "__cause__", 123)
        exc.__cause__ = exc2
        self.assertIs(exc.__cause__, exc2)
        exc.__cause__ = None
        self.assertIs(exc.__cause__, None)

        # Test setter for __context__.
        self.assertRaises(TypeError, setattr, exc, "__context__", 456)
        exc.__context__ = exc2
        self.assertIs(exc.__context__, exc2)
        exc.__context__ = None
        self.assertIs(exc.__context__, None)

        # Test setter for __traceback__.
        self.assertRaises(TypeError, setattr, "__traceback__", "some string")
        # TODO(bsimmers): Set a real traceback once we support them.
        exc.__traceback__ = None
        self.assertIs(exc.__traceback__, None)

    def test_context_chaining(self):
        inner_exc = None
        outer_exc = None
        try:
            try:
                raise RuntimeError("whoops")
            except RuntimeError as exc:
                inner_exc = exc
                raise TypeError("darn")
        except TypeError as exc:
            outer_exc = exc

        self.assertIsInstance(inner_exc, RuntimeError)
        self.assertIsInstance(outer_exc, TypeError)
        self.assertIs(outer_exc.__context__, inner_exc)
        self.assertIsNone(inner_exc.__context__)

    def test_context_chaining_cycle_avoidance(self):
        exc0 = None
        exc1 = None
        exc2 = None
        exc3 = None
        try:
            try:
                try:
                    try:
                        raise RuntimeError("inner")
                    except RuntimeError as exc:
                        exc0 = exc
                        raise RuntimeError("middle")
                except RuntimeError as exc:
                    exc1 = exc
                    raise RuntimeError("outer")
            except RuntimeError as exc:
                exc2 = exc
                # The __context__ link between exc1 and exc0 should be broken
                # by this raise.
                raise exc0
        except RuntimeError as exc:
            exc3 = exc

        self.assertIs(exc3, exc0)
        self.assertIs(exc3.__context__, exc2)
        self.assertIs(exc2.__context__, exc1)
        self.assertIs(exc1.__context__, None)

    def test_with_traceback_sets_none_as_dunder_traceback(self):
        e = BaseException()
        self.assertIs(e, e.with_traceback(None))
        self.assertIs(e.__traceback__, None)

    def test_with_traceback_sets_traceback_as_dunder_traceback(self):
        traceback_obj = None
        try:
            raise Exception("test")
        except Exception as e:
            traceback_obj = e.__traceback__
        self.assertIsNot(traceback_obj, None)

        e = BaseException()
        self.assertIs(e, e.with_traceback(traceback_obj))
        self.assertIs(e.__traceback__, traceback_obj)

    def test_with_traceback_with_non_traceback_raises_type_error(self):
        e = BaseException()
        with self.assertRaises(TypeError):
            e.with_traceback("not_a_traceback_obj")


class EvalTests(unittest.TestCase):
    def test_globals_none_accesses_function_globals(self):
        from types import ModuleType

        module = ModuleType("")
        exec("def foo(): return eval('globals(), locals()', None, {})", module.__dict__)
        eval_globals, eval_locals = module.foo()
        self.assertIs(eval_globals, module.__dict__)
        self.assertEqual(eval_locals, {})

    def test_globals_and_locals_none_accesses_function_locals(self):
        from types import ModuleType

        module = ModuleType("")
        exec(
            "def foo(): bar = 42; return eval('globals(), locals(), bar', None, None)",
            module.__dict__,
        )
        eval_globals, eval_locals, bar = module.foo()
        self.assertIs(eval_globals, module.__dict__)
        self.assertEqual(eval_locals, {"bar": 42})
        self.assertEqual(bar, 42)

    def test_globals_and_locals_none_accesses_implicit_globals(self):
        class C:
            foo = 42
            globals, locals = eval("globals(), locals()", None, None)

        self.assertIs(C.globals, globals())
        self.assertIn("foo", C.locals)
        self.assertEqual(C.locals["foo"], 42)

    def test_locals_none_accesses_globals(self):
        from types import ModuleType

        module = ModuleType("")
        module.foo = 42
        eval_globals, eval_locals = eval("globals(), locals()", module.__dict__, None)
        self.assertIs(eval_globals, module.__dict__)
        self.assertIs(eval_locals, module.__dict__)

    def test_builtins_is_added(self):
        import builtins

        globals = {}
        exec("", globals, globals)
        self.assertIn("__builtins__", globals)
        self.assertIs(globals["__builtins__"], builtins.__dict__)

    def test_bytes_source(self):
        self.assertEqual(eval(b"4 * 5"), 20)  # noqa: P204

    def test_bytearray_source(self):
        self.assertEqual(eval(bytearray(b"20 / 5")), 4)  # noqa: P204

    def test_str_with_left_whitespace_returns(self):
        res = eval(" 1 + 1")
        self.assertEqual(res, 2)

    def test_bytes_with_left_whitespace_returns(self):
        res = eval(b" 1 + 1")  # noqa: P204
        self.assertEqual(res, 2)

    def test_bytearray_with_left_whitespace_returns(self):
        res = eval(bytearray(b" 1 + 1"))  # noqa: P204
        self.assertEqual(res, 2)

    def test_str_with_right_whitespace_returns(self):
        res = eval("1 + 1 ")
        self.assertEqual(res, 2)

    def test_bytes_with_right_whitespace_returns(self):
        res = eval(b"1 + 1 ")  # noqa: P204
        self.assertEqual(res, 2)

    def test_bytearray_with_left_whitespace_returns(self):
        res = eval(bytearray(b"1 + 1 "))  # noqa: P204
        self.assertEqual(res, 2)

    def test_code_source(self):
        from types import CodeType

        code = compile("20 - 4", "", "eval", 0, True, -1)
        self.assertIsInstance(code, CodeType)
        self.assertEqual(eval(code), 16)  # noqa: P204

    @unittest.skipIf(True, "TODO(T78726269): Investigate why this test fails")
    def test_inherits_compile_flags(self):
        import __future__

        from types import CodeType

        code = compile(
            "eval('8 <> 9')", "", "eval", __future__.CO_FUTURE_BARRY_AS_BDFL, True, -1
        )
        self.assertIsInstance(code, CodeType)
        self.assertTrue(code.co_flags & __future__.CO_FUTURE_BARRY_AS_BDFL != 0)
        self.assertTrue(eval(code))  # noqa: P204

    def test_int_source_raises_type_error(self):
        with self.assertRaises(TypeError):
            eval(123)

    def test_non_dict_globals_raise_type_error(self):
        with self.assertRaises(TypeError):
            eval("0", "not a dict")

        class Mapping:
            def __setitem__(self, key, name):
                pass

            def __getitem__(self, key):
                pass

        with self.assertRaises(TypeError):
            eval("0", Mapping())

    def test_non_mapping_locals_raises_type_error(self):
        with self.assertRaises(TypeError):
            not_a_mapping = 42.4
            eval("0", None, not_a_mapping)

    def test_code_with_freevars_raises_type_error(self):
        def func(x):
            return lambda: x

        code = func(42).__code__
        with self.assertRaisesRegex(TypeError, "may not contain free variables"):
            eval(code)  # noqa: P204

    def test_globals_dict_reads_dict(self):
        d = {"a": 1}
        res = eval("a + 1", d)
        self.assertEqual(res, 2)

    def test_globals_dict_updates_dict(self):
        d = {"a": 1}
        code = compile("a = 2", "<string>", "exec")
        eval(code, d)  # noqa: P204
        self.assertEqual(d["a"], 2)


class ExecTests(unittest.TestCase):
    def test_globals_none_accesses_function_globals(self):
        from types import ModuleType

        module = ModuleType("")
        module.foo = 42
        module.locals = {}
        exec("exec('result = foo', None, locals)", module.__dict__)
        self.assertIn("result", module.locals)
        self.assertEqual(module.locals["result"], 42)

    def test_globals_and_locals_none_accesses_function_locals(self):
        def f():
            foo = 42  # noqa: F841
            result = []
            exec("result.append(foo)", None, None)
            return result

        self.assertEqual(f(), [42])

    def test_globals_and_locals_none_accesses_implicit_globals(self):
        class C:
            foo = 42
            exec("result = foo", None, None)

        self.assertTrue(hasattr(C, "result"))
        self.assertEqual(C.result, 42)

    def test_locals_none_accesses_globals(self):
        from types import ModuleType

        module = ModuleType("")
        module.foo = 42
        exec("result = foo", module.__dict__, None)
        self.assertTrue(hasattr(module, "result"))
        self.assertEqual(module.result, 42)

    def test_bytes_source(self):
        from types import ModuleType

        module = ModuleType("")
        module.foo = 13
        exec(b"result = foo", module.__dict__, None)
        self.assertTrue(hasattr(module, "result"))
        self.assertEqual(module.result, 13)

    def test_bytearray_source(self):
        from types import ModuleType

        module = ModuleType("")
        module.foo = 7
        exec(bytearray(b"result = foo"), module.__dict__, None)
        self.assertTrue(hasattr(module, "result"))
        self.assertEqual(module.result, 7)

    def test_code_source(self):
        from types import CodeType
        from types import ModuleType

        code = compile("result = foo", "", "exec", 0, True, -1)
        self.assertIsInstance(code, CodeType)
        module = ModuleType("")
        module.foo = 99
        self.assertIs(exec(code, module.__dict__), None)
        self.assertEqual(module.result, 99)

    def test_inherits_compile_flags(self):
        import __future__

        from types import CodeType
        from types import ModuleType

        code = compile(
            "result0 = 4 <> 4\nexec('result1 = 8 <> 9')",
            "",
            "exec",
            __future__.CO_FUTURE_BARRY_AS_BDFL,
            True,
            -1,
        )
        self.assertIsInstance(code, CodeType)
        self.assertTrue(code.co_flags & __future__.CO_FUTURE_BARRY_AS_BDFL != 0)
        module = ModuleType("")
        self.assertIs(exec(code, module.__dict__), None)
        self.assertFalse(module.result0)
        self.assertTrue(module.result1)

    def test_int_source_raises_type_error(self):
        with self.assertRaises(TypeError):
            exec(123)

    def test_non_dict_globals_raise_type_error(self):
        with self.assertRaises(TypeError):
            exec("pass", "not a dict")

        class Mapping:
            def __setitem__(self, key, name):
                pass

            def __getitem__(self, key):
                pass

        with self.assertRaises(TypeError):
            exec("pass", Mapping())

    def test_non_mapping_locals_raises_type_error(self):
        with self.assertRaises(TypeError):
            not_a_mapping = 42.4
            exec("pass", None, not_a_mapping)

    def test_code_with_freevars_raises_type_error(self):
        def func(x):
            return lambda: x

        code = func(42).__code__
        with self.assertRaisesRegex(TypeError, "may not contain free variables"):
            exec(code)

    def test_globals_dict_reads_dict(self):
        d = {"a": 1}
        exec("b = a + 1", d)
        self.assertEqual(d["b"], 2)

    def test_globals_dict_updates_dict(self):
        d = {"a": 1}
        exec("a = 2", d)
        self.assertEqual(d["a"], 2)

    def test_coroutine_returning_arbitrary_exception_passes_through_unchanged(self):
        v = RuntimeError("banana")

        async def f():
            return v

        # Send in None to trigger initial execution of coroutine
        with self.assertRaises(StopIteration) as exc:
            f().send(None)

        self.assertIs(exc.exception.value, v)
        self.assertEqual(exc.exception.args, (v,))


class FrozensetTests(unittest.TestCase):
    def test_deepcopy_with_frozenset_returns_frozenset(self):
        s = frozenset([1, 2, 3])
        self.assertEqual(s, copy.deepcopy(s))

    def test_deepcopy_with_frozenset_subclass_returns_subclass(self):
        class C(frozenset):
            pass

        s = C([1, 2, 3])
        self.assertEqual(s, copy.deepcopy(s))

    def test_dunder_and_with_non_frozenset_raises_type_error(self):
        with self.assertRaises(TypeError):
            frozenset.__and__(set(), frozenset())

    def test_dunder_hash_returns_int(self):
        self.assertEqual(frozenset.__hash__(frozenset()), 133146708735736)
        self.assertEqual(frozenset.__hash__(frozenset((1, 2, 3))), -272375401224217160)

    def test_dunder_new_with_failing_iterable_propagates_error(self):
        class C:
            def __iter__(self):
                return self

            def __next__(self):
                raise RuntimeError("foo")

        with self.assertRaises(RuntimeError) as context:
            frozenset(C())
        self.assertEqual(str(context.exception), "foo")

    def test_dunder_new_with_subclass_and_iterable_creates_instance_of_subclass(self):
        class Bad:
            def __iter__(self):
                return self

            def __next__(self):
                raise RuntimeError("foo")

        class C(frozenset):
            pass

        with self.assertRaises(RuntimeError) as context:
            frozenset.__new__(frozenset, Bad())
        self.assertEqual(str(context.exception), "foo")

        result = frozenset.__new__(C, [1, 2, 3])
        self.assertIsInstance(result, C)

    def test_dunder_or_with_non_frozenset_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            frozenset.__or__(set(), frozenset())

    def test_dunder_or_with_missing_other_raises_type_error(self):
        with self.assertRaises(TypeError):
            frozenset.__or__(frozenset())

    def test_dunder_or_with_non_anyset_other_returns_notimplemented(self):
        self.assertIs(frozenset.__or__(frozenset(), []), NotImplemented)

    def test_dunder_or_with_id_equal_self_and_other_returns_self_copy(self):
        left = frozenset((1, 2, 3))
        result = left.__or__(left)
        self.assertIsInstance(result, frozenset)
        self.assertIsNot(result, left)
        self.assertEqual(result, left)

    def test_dunder_or_returns_union(self):
        left = frozenset((1, 2, 3))
        result = left.__or__({2, 3, 4})
        self.assertIsInstance(result, frozenset)
        self.assertEqual(result, frozenset((1, 2, 3, 4)))

    def test_dunder_or_with_empty_left_returns_elements_from_right(self):
        result = frozenset().__or__({2, 3, 4})
        self.assertIsInstance(result, frozenset)
        self.assertEqual(result, frozenset((2, 3, 4)))

    def test_dunder_or_with_empty_right_returns_elements_from_left(self):
        result = frozenset((1, 2, 3)).__or__(set())
        self.assertIsInstance(result, frozenset)
        self.assertEqual(result, frozenset((1, 2, 3)))

    def test_dunder_or_with_empty_left_and_right_returns_empty_frozenset(self):
        empty = frozenset()
        result = empty.__or__(set())
        self.assertIsInstance(result, frozenset)
        self.assertIsNot(result, empty)
        self.assertEqual(result, frozenset())

    def test_dunder_or_with_subclass_returns_exact_frozenset(self):
        class C(frozenset):
            pass

        left = C()
        right = C()
        self.assertIs(type(left | right), frozenset)
        self.assertIs(type(left | left), frozenset)

    def test_dunder_reduce_with_frozenset_returns_tuple(self):
        a_frozenset = frozenset({1, 2, 3})
        result = a_frozenset.__reduce__()
        cls, value, state = result
        self.assertEqual(type(result), tuple)
        self.assertEqual(type(cls), type)
        self.assertEqual(cls, frozenset)
        self.assertEqual(type(value), tuple)
        self.assertEqual(len(value), 1)
        self.assertEqual(value[0], [1, 2, 3])
        self.assertEqual(state, None)

    def test_dunder_reduce_with_frozenset_subclass_returns_tuple(self):
        class C(frozenset):
            pass

        c = C({1, 2, 3})
        c.test_value = "test_value"
        result = c.__reduce__()
        cls, value, state = result
        self.assertEqual(type(result), tuple)
        self.assertEqual(type(cls), type)
        self.assertEqual(cls, C)
        self.assertEqual(type(value), tuple)
        self.assertEqual(len(value), 1)
        self.assertEqual(type(value[0]), list)
        self.assertEqual(value[0], [1, 2, 3])
        self.assertEqual(len(state), 1)
        self.assertEqual(state["test_value"], "test_value")

    def test_dunder_repr_with_non_frozenset_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            frozenset.difference(set())

    def test_dunder_repr_with_empty_frozenset(self):
        self.assertEqual(frozenset().__repr__(), "frozenset()")

    def test_dunder_repr_with_non_empty_frozenset(self):
        self.assertEqual(frozenset({1, 2, 3}).__repr__(), "frozenset({1, 2, 3})")

    def test_dunder_repr_with_subclass_prints_subclass_name(self):
        class C(frozenset):
            pass

        self.assertEqual(C().__repr__(), "C()")
        self.assertEqual(C({1, 2, 3}).__repr__(), "C({1, 2, 3})")

    def test_dunder_repr_with_recursive_frozenset_prints_ellipsis(self):
        class C:
            def __init__(self):
                self.value = frozenset((self,))

            def __hash__(self):
                return 1

            def __repr__(self):
                return self.value.__repr__()

        self.assertEqual(C().__repr__(), "frozenset({frozenset(...)})")

    def test_dunder_sub_with_non_frozenset_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            frozenset.__sub__(set(), frozenset())

    def test_dunder_sub_same_frozensets_returns_empty_frozenset(self):
        a_frozenset = frozenset({1, 2, 3})
        self.assertFalse(frozenset.__sub__(a_frozenset, a_frozenset))

    def test_dunder_sub_two_frozensets_returns_difference(self):
        frozenset1 = frozenset({1, 2, 3, 4, 5, 6, 7})
        frozenset2 = frozenset({1, 3, 5, 7})
        self.assertEqual(frozenset.__sub__(frozenset1, frozenset2), {2, 4, 6})

    def test_dunder_sub_set_from_frozenset_returns_difference(self):
        frozenset1 = frozenset({1, 2, 3, 4, 5, 6, 7})
        set2 = set({1, 3, 5, 7})
        self.assertEqual(frozenset.__sub__(frozenset1, set2), {2, 4, 6})

    def test_difference_with_non_frozenset_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            frozenset.difference(set(), frozenset())

    def test_difference_no_others_copies_self(self):
        a_frozenset = frozenset({1, 2, 3})
        self.assertIsNot(frozenset.difference(a_frozenset), a_frozenset)

    def test_difference_same_frozensets_returns_empty_frozenset(self):
        a_frozenset = frozenset({1, 2, 3})
        self.assertFalse(frozenset.difference(a_frozenset, a_frozenset))

    def test_difference_two_frozensets_returns_difference(self):
        frozenset1 = frozenset({1, 2, 3, 4, 5, 6, 7})
        frozenset2 = frozenset({1, 3, 5, 7})
        self.assertEqual(frozenset.difference(frozenset1, frozenset2), {2, 4, 6})

    def test_difference_many_frozensets_returns_difference(self):
        a_frozenset = frozenset({1, 10, 100, 1000})
        self.assertEqual(frozenset.difference(a_frozenset, {10}, {100}, {1000}), {1})

    def test_issuperset_with_non_frozenset_raises_type_error(self):
        with self.assertRaises(TypeError):
            frozenset.issuperset(None, frozenset())

    def test_issuperset_with_set_raises_type_error(self):
        with self.assertRaises(TypeError):
            frozenset.issuperset(set(), frozenset())

    def test_issuperset_with_empty_sets_returns_true(self):
        self.assertTrue(frozenset.issuperset(frozenset(), frozenset()))
        self.assertTrue(frozenset.issuperset(frozenset(), set()))

    def test_issuperset_with_anyset_subset_returns_true(self):
        self.assertTrue(frozenset({1}).issuperset(set()))
        self.assertTrue(frozenset({1}).issuperset(frozenset()))

        self.assertTrue(frozenset({1}).issuperset({1}))
        self.assertTrue(frozenset({1}).issuperset(frozenset({1})))

        self.assertTrue(frozenset({1, 2}).issuperset({1}))
        self.assertTrue(frozenset({1, 2}).issuperset(frozenset({1})))

        self.assertTrue(frozenset({1, 2}).issuperset({1, 2}))
        self.assertTrue(frozenset({1, 2}).issuperset(frozenset({1, 2})))

        self.assertTrue(frozenset({1, 2, 3}).issuperset({1, 2}))
        self.assertTrue(frozenset({1, 2, 3}).issuperset(frozenset({1, 2})))

    def test_issuperset_with_iterable_subset_returns_true(self):
        self.assertTrue(frozenset({1}).issuperset([]))
        self.assertTrue(frozenset({1}).issuperset(range(1, 1)))

        self.assertTrue(frozenset({1}).issuperset([1]))
        self.assertTrue(frozenset({1}).issuperset(range(1, 2)))

        self.assertTrue(frozenset({1, 2}).issuperset([1]))
        self.assertTrue(frozenset({1, 2}).issuperset(range(1, 2)))

        self.assertTrue(frozenset({1, 2}).issuperset([1, 2]))
        self.assertTrue(frozenset({1, 2}).issuperset(range(1, 3)))

        self.assertTrue(frozenset({1, 2, 3}).issuperset([1, 2]))
        self.assertTrue(frozenset({1, 2, 3}).issuperset(range(1, 3)))

    def test_issuperset_with_superset_returns_false(self):
        self.assertFalse(frozenset({}).issuperset({1}))
        self.assertFalse(frozenset({}).issuperset(frozenset({1})))
        self.assertFalse(frozenset({}).issuperset([1]))
        self.assertFalse(frozenset({}).issuperset(range(1, 2)))

        self.assertFalse(frozenset({1}).issuperset({1, 2}))
        self.assertFalse(frozenset({1}).issuperset(frozenset({1, 2})))
        self.assertFalse(frozenset({1}).issuperset([1, 2]))
        self.assertFalse(frozenset({1}).issuperset(range(1, 3)))

        self.assertFalse(frozenset({1, 2}).issuperset({1, 2, 3}))
        self.assertFalse(frozenset({1, 2}).issuperset(frozenset({1, 2, 3})))
        self.assertFalse(frozenset({1, 2}).issuperset([1, 2, 3]))
        self.assertFalse(frozenset({1, 2}).issuperset(range(1, 4)))

    def test_union_with_non_frozenset_as_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            frozenset.union(set(), set())

    def test_union_with_frozenset_returns_union(self):
        set1 = frozenset({1, 2})
        set2 = frozenset({2, 3})
        set3 = frozenset({4, 5})
        self.assertEqual(frozenset.union(set1, set2, set3), frozenset({1, 2, 3, 4, 5}))

    def test_union_with_self_returns_copy(self):
        a_set = frozenset({1, 2, 3})
        self.assertIs(type(frozenset.union(a_set)), frozenset)
        self.assertIsNot(frozenset.union(a_set), a_set)
        self.assertIsNot(frozenset.union(a_set, a_set), a_set)
        self.assertEqual(frozenset.union(a_set, a_set), a_set)

    def test_union_with_iterable_contains_iterable_items(self):
        a_set = frozenset({1, 2})
        a_dict = {2: True, 3: True}
        self.assertEqual(frozenset.union(a_set, a_dict), frozenset({1, 2, 3}))

    def test_union_with_custom_iterable(self):
        class C:
            def __init__(self, start, end):
                self.pos = start
                self.end = end

            def __iter__(self):
                return self

            def __next__(self):
                if self.pos == self.end:
                    raise StopIteration
                result = self.pos
                self.pos += 1
                return result

        self.assertEqual(
            frozenset.union(frozenset(), C(1, 3), C(6, 9)), frozenset({1, 2, 6, 7, 8})
        )

    def test_union_with_non_iterable_raises_typeerror(self):
        with self.assertRaises(TypeError):
            frozenset.union(frozenset({1, 2, 3}), 1.5)


class FunctionTests(unittest.TestCase):
    def test_dunder_closure_returns_none_if_function_is_not_a_closure(self):
        def foo():
            pass

        self.assertIsNone(foo.__closure__)

    def test_dunder_closure_returns_tuple_with_cell_if_function_is_closure(self):
        def foo(x):
            def bar(n):
                return x + n

            return bar

        add_three = foo(3)
        self.assertIsInstance(add_three.__closure__, tuple)
        self.assertEqual(len(add_three.__closure__), 1)
        self.assertEqual(type(add_three.__closure__[0]).__name__, "cell")
        self.assertEqual(add_three.__closure__[0].cell_contents, 3)

    def test_dunder_closure_is_read_only_attribute(self):
        def foo():
            pass

        with self.assertRaises(AttributeError):
            foo.__closure__ = 8

    def test_dunder_dict_matches_attributes(self):
        def foo():
            pass

        d = foo.__dict__
        self.assertEqual(len(d), 0)
        foo.bar = 42
        self.assertEqual(d["bar"], 42)
        d["baz"] = -8
        self.assertEqual(foo.baz, -8)

    def test_dunder_dict_setter_update_matches_attributes(self):
        def foo():
            pass

        foo.__dict__ = {"baz": 500}
        self.assertEqual(len(foo.__dict__), 1)
        self.assertEqual(foo.baz, 500)

    def test_dunder_dict_setters_with_non_dict_raises_type_error(self):
        def foo():
            pass

        with self.assertRaises(TypeError):
            foo.__dict__ = 100

    def test_dunder_dict_after_dunder_class_succeeds(self):
        class C:
            pass

        class D:
            def __init__(self):
                self.x = 10
                self.y = 20

        c = C()
        c.__class__ = D
        c.__dict__ = {"baz": 100}
        self.assertIs(c.baz, 100)

    def test_dunder_dict_with_in_object_attributes_after_dunder_class_succeeds(self):
        class C:
            def __init__(self):
                self.fish = -100

        class D:
            def __init__(self):
                self.x = 10
                self.y = 20

        c = C()
        c.__class__ = D
        c.__dict__ = {"baz": 100}
        self.assertIs(c.baz, 100)

    def test_dunder_dict_after_dunder_class_multiple_times_succeeds(self):
        class C:
            def __init__(self):
                self.fish = -100

        class D:
            def __init__(self):
                self.x = 10
                self.y = 20

        class E:
            def __init__(self):
                self.z = 5000

        c = C()
        c.__class__ = D
        c.__dict__ = {"baz": 100}
        self.assertIs(c.baz, 100)

        c.__class__ = E
        c.__dict__ = {"baf": 200}
        self.assertIs(c.baf, 200)

    def test_dunder_globals_returns_identical_object(self):
        def foo():
            pass

        self.assertIs(foo.__globals__, foo.__globals__)

    def test_dunder_globals_returns_dict_of_defining_module_dict(self):
        from types import ModuleType

        module = ModuleType("test_module")
        module_code = """
def foo():
  pass
        """
        exec(module_code, module.__dict__)
        self.assertIs(module.__dict__["foo"].__globals__, module.__dict__)

    @pyro_only
    def test_dunder_globals_with_extension_function_returns_empty_dict(self):
        import atexit

        self.assertIn("BuiltinImporter", str(atexit.__loader__))
        dunder_globals = atexit.register.__globals__
        self.assertEqual(dunder_globals, {})
        self.assertIsInstance(dunder_globals, dict)

    def test_dunder_defaults_returns_defaults(self):
        def foo(arg=42):
            return arg

        self.assertEqual(foo.__defaults__, (42,))

    def test_dunder_set_defaults(self):
        def foo(arg="bar"):
            return arg

        self.assertEqual(foo(), "bar")
        foo.__defaults__ = ("baz",)
        self.assertEqual(foo(), "baz")

    def test_dunder_set_defaults_with_tuple_subclass(self):
        class T(tuple):
            pass

        def whereami(office=770):
            return office

        self.assertEqual(whereami(), 770)
        whereami.__defaults__ = T((225,))
        self.assertEqual(whereami(), 225)

    def test_dunder_set_defaults_with_non_tuple_raises_typeerror(self):
        def foo():
            pass

        with self.assertRaises(TypeError):
            foo.__defaults__ = {}

    def test_dunder_set_defaults_with_none_makes_function_require_arg(self):
        def foo(arg="bar"):
            return arg

        foo.__defaults__ = None
        self.assertEqual(foo.__defaults__, None)
        self.assertEqual(foo(arg="abc"), "abc")
        with self.assertRaises(TypeError):
            foo()

    def test_dunder_dir_returns_list(self):
        def foo():
            pass

        self.assertIsInstance(foo.__dir__(), list)

    def test_dunder_kwdefaults_returns_kwdefaults(self):
        def foo(*args, kwarg=42):
            return kwarg

        self.assertEqual(foo.__kwdefaults__, {"kwarg": 42})

    def test_dunder_set_kwdefaults(self):
        def foo(*args, kwarg="bar"):
            return kwarg

        self.assertEqual(foo(), "bar")
        foo.__kwdefaults__ = {"kwarg": "baz"}
        self.assertEqual(foo(), "baz")

    def test_dunder_set_kwdefaults_with_dict_subclass(self):
        class D(dict):
            pass

        def whereami(*args, office=770):
            return office

        self.assertEqual(whereami(), 770)
        whereami.__kwdefaults__ = D((("office", 225),))
        self.assertEqual(whereami(), 225)

    def test_dunder_set_kwdefaults_with_none_makes_function_require_kwarg(self):
        def foo(*args, kwarg="bar"):
            return kwarg

        foo.__kwdefaults__ = None
        self.assertEqual(foo.__kwdefaults__, None)
        self.assertEqual(foo(kwarg="abc"), "abc")
        with self.assertRaises(TypeError):
            foo()

    def test_dunder_set_kwdefaults_with_non_dict_raises_typeerror(self):
        def foo():
            pass

        with self.assertRaises(TypeError):
            foo.__kwdefaults__ = "not a dict"

    def test_dunder_annotations_returns_annotations(self):
        def foo(arg: str):
            pass

        self.assertEqual(foo.__annotations__, {"arg": str})

    def test_dunder_set_annotations(self):
        def foo(arg: str):
            pass

        foo.__annotations__ = {"arg": int}
        self.assertEqual(foo.__annotations__, {"arg": int})
        foo.__annotations__ = None
        self.assertEqual(foo.__annotations__, {})

    def test_dunder_set_annotations_with_dict_subclass(self):
        class D(dict):
            pass

        def foo(arg: str):
            pass

        foo.__annotations__ = D((("arg", int),))
        self.assertEqual(foo.__annotations__, {"arg": int})

    def test_dunder_set_annotations_with_non_dict_raises_typeerror(self):
        def foo():
            pass

        with self.assertRaises(TypeError):
            foo.__annotations__ = (1, 2, 3)

    def test_dunder_new_returns_function_object(self):
        import types

        def _f():
            pass

        new_func = types.FunctionType(_f.__code__, _f.__globals__)
        self.assertIsInstance(new_func, types.FunctionType)
        self.assertEqual(new_func.__name__, "_f")

    def test_dunder_new_with_all_args_returns_function_object(self):
        import types

        def _f():
            pass

        new_func = types.FunctionType(
            _f.__code__, _f.__globals__, "hi", _f.__defaults__, _f.__closure__
        )
        self.assertIsInstance(new_func, types.FunctionType)
        self.assertEqual(new_func.__name__, "hi")
        self.assertEqual(new_func.__defaults__, _f.__defaults__)
        self.assertEqual(new_func.__closure__, _f.__closure__)

    def test_dunder_new_with_name_override_as_subclassed_str_returns_function_object(
        self,
    ):
        import types

        def _f():
            pass

        class _s(str):
            pass

        str_subclass = _s("reb00t")
        new_func = types.FunctionType(_f.__code__, _f.__globals__, name=str_subclass)
        self.assertIsInstance(new_func, types.FunctionType)
        self.assertEqual(new_func.__name__, "reb00t")

    def test_dunder_new_with_closure_as_subclassed_tuple_returns_function_object(self):
        import types

        def _f():
            pass

        class _t(tuple):
            pass

        tuple_subclass = _t()
        new_func = types.FunctionType(
            _f.__code__, _f.__globals__, closure=tuple_subclass
        )
        self.assertIsInstance(new_func, types.FunctionType)
        self.assertEqual(new_func.__closure__, ())

    def test_dunder_new_with_non_code_type_value_raises_type_error(self):
        import types

        def _f():
            pass

        with self.assertRaises(TypeError):
            types.FunctionType("", _f.__globals__, name="hi")

    def test_dunder_new_with_non_dict_globals_value_raises_type_error(self):
        import types

        def _f():
            pass

        with self.assertRaises(TypeError):
            types.FunctionType(_f.__code__, "", name="hi")

    def test_dunder_new_with_dict_globals_returns_function(self):
        import types

        def _f():
            global foo
            return foo

        result = types.FunctionType(_f.__code__, {"foo": "bar"}, name="hi")
        self.assertIsInstance(result, types.FunctionType)
        self.assertEqual(result(), "bar")

    def test_dunder_new_with_non_str_names_value_raises_type_error(self):
        import types

        def _f():
            pass

        with self.assertRaises(TypeError):
            types.FunctionType(_f.__code__, _f.__globals__, name=[])

    def test_dunder_new_with_non_tuple_argdefs_value_raises_type_error(self):
        import types

        def _f():
            pass

        with self.assertRaises(TypeError):
            types.FunctionType(_f.__code__, _f.__globals__, argdefs="not a tuple")

    def test_dunder_new_with_non_tuple_closure_value_raises_type_error(self):
        import types

        def _f():
            pass

        with self.assertRaises(TypeError):
            types.FunctionType(_f.__code__, _f.__globals__, closure="not a tuple")


class GeneratorTests(unittest.TestCase):
    def test_managed_yield_from_stop_iteration_turns_into_runtime_error(self):
        def inner_gen():
            raise StopIteration

        def outer_gen():
            val = yield from inner_gen()
            yield val

        g = outer_gen()
        with self.assertRaises(RuntimeError):
            next(g)

    def test_managed_stop_iteration_turns_into_runtime_error(self):
        def gen():
            raise StopIteration
            yield

        with self.assertRaises(RuntimeError):
            next(gen())

    def test_gi_running(self):
        def gen():
            self.assertTrue(g.gi_running)
            yield 1
            self.assertTrue(g.gi_running)
            yield 2

        g = gen()
        self.assertFalse(g.gi_running)
        next(g)
        self.assertFalse(g.gi_running)
        next(g)
        self.assertFalse(g.gi_running)

    def test_gi_running_readonly(self):
        def gen():
            yield None

        g = gen()
        self.assertRaises(AttributeError, setattr, g, "gi_running", 1234)

    def test_running_gen_raises(self):
        def gen():
            self.assertRaises(ValueError, next, g)
            yield "done"

        g = gen()
        self.assertEqual(next(g), "done")

    class MyError(Exception):
        pass

    @staticmethod
    def simple_gen():
        yield 1
        yield 2

    @staticmethod
    def catching_gen():
        try:
            yield 1
        except GeneratorTests.MyError:
            yield "caught"

    @staticmethod
    def catching_returning_gen():
        try:
            yield 1
        except GeneratorTests.MyError:
            return "all done!"  # noqa

    @staticmethod
    def delegate_gen(g):
        r = yield from g
        yield r

    def test_close_with_invalid_self_raises_type_error(self):
        g = self.simple_gen()
        with self.assertRaises(TypeError):
            type(g).close(None)

    def test_close_when_exhausted_returns_none(self):
        g = self.simple_gen()
        self.assertEqual(next(g), 1)
        self.assertEqual(next(g), 2)
        self.assertIsNone(g.close())

    def test_close_when_generator_exit_propagates_returns_none(self):
        saw_generator_exit = False

        def f():
            nonlocal saw_generator_exit
            try:
                yield 1
            except GeneratorExit:
                saw_generator_exit = True
                raise

        g = f()
        self.assertEqual(next(g), 1)
        self.assertIsNone(g.close())
        self.assertTrue(saw_generator_exit)

    def test_close_when_generator_exit_derived_exception_raised_returns_none(self):
        class GeneratorExitDerived(GeneratorExit):
            pass

        def f():
            try:
                yield 1
            except GeneratorExit:
                raise GeneratorExitDerived

        g = f()
        self.assertEqual(next(g), 1)
        self.assertIsNone(g.close())

    def test_close_when_stop_iteration_raised_returns_none(self):
        saw_generator_exit = False

        def f():
            nonlocal saw_generator_exit
            try:
                yield 1
            except GeneratorExit:
                saw_generator_exit = True
                # Implicitly raises StopIteration(2)
                return 2  # noqa

        g = f()
        self.assertEqual(next(g), 1)
        self.assertIsNone(g.close())
        self.assertTrue(saw_generator_exit)

    def test_close_generator_raises_exception_propagates(self):
        def f():
            try:
                yield 1
            except GeneratorExit:
                raise ValueError

        g = f()
        self.assertEqual(next(g), 1)
        self.assertRaises(ValueError, g.close)

    def test_close_generator_yields_raises_runtime_error(self):
        def f():
            try:
                yield 1
            except GeneratorExit:
                yield 2

        g = f()
        self.assertEqual(g.send(None), 1)
        self.assertRaises(RuntimeError, g.close)

    def test_throw(self):
        g = self.simple_gen()
        self.assertRaises(GeneratorTests.MyError, g.throw, GeneratorTests.MyError())

    def test_throw_caught(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertEqual(g.throw(GeneratorTests.MyError()), "caught")

    def test_throw_type(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertEqual(g.throw(GeneratorTests.MyError), "caught")

    def test_throw_type_and_value(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertEqual(
            g.throw(GeneratorTests.MyError, GeneratorTests.MyError()), "caught"
        )

    def test_throw_uncaught_type(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(RuntimeError, g.throw, RuntimeError)

    def test_throw_finished(self):
        g = self.catching_returning_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(StopIteration, g.throw, GeneratorTests.MyError)

    def test_throw_two_values(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(
            TypeError, g.throw, GeneratorTests.MyError(), GeneratorTests.MyError()
        )

    def test_throw_bad_traceback(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(
            TypeError, g.throw, GeneratorTests.MyError, GeneratorTests.MyError(), 5
        )

    def test_throw_bad_type(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(TypeError, g.throw, 1234)

    def test_throw_not_started(self):
        g = self.simple_gen()
        self.assertRaises(GeneratorTests.MyError, g.throw, GeneratorTests.MyError())
        self.assertRaises(StopIteration, next, g)

    def test_throw_stopped(self):
        g = self.simple_gen()
        self.assertEqual(next(g), 1)
        self.assertEqual(next(g), 2)
        self.assertRaises(StopIteration, next, g)
        self.assertRaises(GeneratorTests.MyError, g.throw, GeneratorTests.MyError())

    def test_throw_yield_from(self):
        g = self.delegate_gen(self.simple_gen())
        self.assertEqual(next(g), 1)
        self.assertRaises(GeneratorTests.MyError, g.throw, GeneratorTests.MyError)

    def test_throw_yield_from_caught(self):
        g = self.delegate_gen(self.catching_gen())
        self.assertEqual(next(g), 1)
        self.assertEqual(g.throw(GeneratorTests.MyError), "caught")

    def test_throw_yield_from_finishes(self):
        g = self.delegate_gen(self.catching_returning_gen())
        self.assertEqual(next(g), 1)
        self.assertEqual(g.throw(GeneratorTests.MyError), "all done!")

    def test_throw_yield_from_non_gen(self):
        g = self.delegate_gen([1, 2, 3, 4])
        self.assertEqual(next(g), 1)
        self.assertRaises(RuntimeError, g.throw, RuntimeError)

    def test_dunder_repr(self):
        def foo():
            yield 5

        self.assertTrue(
            foo()
            .__repr__()
            .startswith(
                "<generator object GeneratorTests.test_dunder_repr.<locals>.foo at "
            )
        )

    def test_exception_context_captured_across_further_yield_from(self):
        def raises():
            raise ValueError
            yield None

        def double_raises():
            try:
                raise RuntimeError
            except RuntimeError:
                yield from raises()

        with self.assertRaises(ValueError) as exc:
            double_raises().send(None)

        self.assertIsInstance(exc.exception.__context__, RuntimeError)


class GlobalsTests(unittest.TestCase):
    def test_returns_module_dunder_dict(self):
        import sys

        self.assertIs(globals(), sys.modules[__name__].__dict__)

    def test_with_module_subclass_returns_module_proxy(self):
        from types import ModuleType

        class C(ModuleType):
            pass

        m_name = "testing.test_with_module_subclass_returns_module_proxy"
        m = C(m_name)
        import sys

        try:
            # We currently only support `globals()` for modules registered in
            # `sys.modules`.
            sys.modules[m_name] = m
            exec("def foo(): return globals()", m.__dict__, m.__dict__)
            self.assertIs(m.foo(), m.__dict__)
        finally:
            del sys.modules[m_name]


class HasattrTests(unittest.TestCase):
    def test_hasattr_calls_dunder_getattribute(self):
        class C:
            def __getattribute__(self, name):
                if name == "foo":
                    return 42
                raise AttributeError(name)

        i = C()
        self.assertTrue(hasattr(i, "foo"))
        self.assertFalse(hasattr(i, "bar"))

    def test_hasattr_propagates_error_from_dunder_getattribute(self):
        class C:
            def __getattribute__(self, name):
                raise UserWarning()

        i = C()
        with self.assertRaises(UserWarning):
            hasattr(i, "foo")

    def test_hasattr_calls_dunder_getattr(self):
        class C:
            def __getattribute__(self, name):
                nonlocal getattribute_called
                getattribute_called = True
                raise AttributeError(name)

            def __getattr__(self, name):
                if name == "foo":
                    return 42
                raise AttributeError(name)

        i = C()
        getattribute_called = False
        self.assertTrue(hasattr(i, "foo"))
        self.assertTrue(getattribute_called)

        getattribute_called = False
        self.assertFalse(hasattr(i, "bar"))
        self.assertTrue(getattribute_called)

    def test_hasattr_propagates_error_from_dunder_getattr(self):
        class C:
            def __getattribute__(self, name):
                raise AttributeError(name)

            def __getattr__(self, name):
                raise UserWarning()

        i = C()
        with self.assertRaises(UserWarning):
            hasattr(i, "foo")

    def test_hasattr_with_non_string_attr_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            hasattr(None, 42)
        self.assertIn("attribute name must be string", str(context.exception))


class HashTests(unittest.TestCase):
    def test_hash_with_raising_dunder_hash_descriptor_raises_type_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("failed")

        class Foo:
            __hash__ = Desc()

        foo = Foo()
        with self.assertRaises(TypeError) as context:
            hash(foo)
        self.assertEqual(str(context.exception), "unhashable type: 'Foo'")

    def test_hash_with_raising_dunder_hash_propagates_exceptions(self):
        class C:
            def __hash__(self):
                raise UserWarning()

        i = C()
        with self.assertRaises(UserWarning):
            hash(i)

    def test_hash_with_none_dunder_hash_raises_type_error(self):
        class Foo:
            __hash__ = None

        foo = Foo()
        with self.assertRaises(TypeError) as context:
            hash(foo)
        self.assertEqual(str(context.exception), "unhashable type: 'Foo'")

    def test_hash_with_non_int_dunder_hash_raises_type_error(self):
        class Foo:
            def __hash__(self):
                return "not an int"

        foo = Foo()
        with self.assertRaises(TypeError) as context:
            hash(foo)
        self.assertEqual(
            str(context.exception), "__hash__ method should return an integer"
        )

    def test_hash_with_int_subclass_dunder_hash_returns_int(self):
        class SubInt(int):
            pass

        class Foo:
            def __hash__(self):
                return SubInt(42)

        foo = Foo()
        result = hash(foo)
        self.assertEqual(42, result)
        self.assertEqual(type(42), int)

    def test_hash_with_false_returns_zero(self):
        class C:
            def __hash__(self):
                return False

        self.assertIs(type(hash(C())), int)
        self.assertEqual(hash(C()), 0)

    def test_hash_with_true_returns_one(self):
        class C:
            def __hash__(self):
                return True

        self.assertIs(type(hash(C())), int)
        self.assertEqual(hash(C()), 1)

    def test_hash_replaces_minus_one_with_minus_two(self):
        class C:
            def __hash__(self):
                return -1

        self.assertEqual(hash(C()), -2)

    def test_hash_with_large_int_value_hashes_result(self):
        class C:
            def __hash__(self):
                return 0xB228AA4BB7326F5D697F47B0CC9230D4

        self.assertEqual(hash(C()), int.__hash__(0xB228AA4BB7326F5D697F47B0CC9230D4))

    def test_hash_returns_hash_value(self):
        class C(int):
            def __hash__(self):
                return self

        self.assertEqual(hash(C(0)), 0)
        self.assertEqual(hash(C(1)), 1)
        self.assertEqual(hash(C(42)), 42)
        self.assertEqual(hash(C(-5)), -5)

    def test_hash_with_max_hash_returns_max_hash(self):
        import sys

        max_hash = (1 << (sys.hash_info.width - 1)) - 1

        class C:
            def __hash__(self):
                nonlocal max_hash
                return max_hash

        self.assertEqual(hash(C()), max_hash)


class HexTests(unittest.TestCase):
    def test_returns_string(self):
        self.assertEqual(hex(0), "0x0")
        self.assertEqual(hex(-1), "-0x1")
        self.assertEqual(hex(1), "0x1")
        self.assertEqual(hex(54321), "0xd431")
        self.assertEqual(hex(81985529216486895), "0x123456789abcdef")
        self.assertEqual(hex(18364758544493064720), "0xfedcba9876543210")

    def test_with_large_int_returns_string(self):
        self.assertEqual(hex(1 << 63), "0x8000000000000000")
        self.assertEqual(hex(1 << 64), "0x10000000000000000")
        self.assertEqual(
            hex(0xDEE182DE2EC55F61B22A509ED1DC3EB), "0xdee182de2ec55f61b22a509ed1dc3eb"
        )
        self.assertEqual(
            hex(-0x53ADC651E593B1323158BFA776E8173F60C76519277B2BD6),
            "-0x53adc651e593b1323158bfa776e8173f60c76519277b2bd6",
        )

    def test_calls_dunder_index(self):
        class C:
            def __int__(self):
                return 42

            def __index__(self):
                return -99

        self.assertEqual(hex(C()), "-0x63")

    def test_with_int_subclass(self):
        class C(int):
            pass

        self.assertEqual(hex(C(51)), "0x33")

    def test_with_non_int_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            hex("not an int")
        self.assertEqual(
            str(context.exception), "'str' object cannot be interpreted as an integer"
        )


class DeleteStream(contextlib.AbstractContextManager):
    _stream = None

    def __init__(self):
        self._old_targets = []

    def __enter__(self):
        self._old_targets.append(getattr(sys, self._stream))
        delattr(sys, self._stream)

    def __exit__(self, exctype, excinst, exctb):
        setattr(sys, self._stream, self._old_targets.pop())


class delete_stderr(DeleteStream):
    _stream = "stderr"


class delete_stdin(DeleteStream):
    _stream = "stdin"


class delete_stdout(DeleteStream):
    _stream = "stdout"


class redirect_stdin(contextlib._RedirectStream):
    _stream = "stdin"


class InputTests(unittest.TestCase):
    def test_input_with_deleted_stderr_raises_runtime_error(self):

        with delete_stderr():
            with self.assertRaises(RuntimeError):
                input()

    def test_input_with_deleted_stdin_raises_runtime_error(self):

        with delete_stdin():
            with self.assertRaises(RuntimeError):
                input()

    def test_input_with_deleted_stdout_raises_runtime_error(self):

        with delete_stdout():
            with self.assertRaises(RuntimeError):
                input()

    def test_input_with_none_stderr_raises_runtime_error(self):

        with contextlib.redirect_stderr(None):
            with self.assertRaises(RuntimeError):
                input()

    def test_input_with_none_stdin_raises_runtime_error(self):

        with redirect_stdin(None):
            with self.assertRaises(RuntimeError):
                input()

    def test_input_with_none_stdout_raises_runtime_error(self):

        with contextlib.redirect_stdout(None):
            with self.assertRaises(RuntimeError):
                input()

    def test_input_strips_trailing_newline(self):
        class MyInStream:
            def readline(self):
                return "foobar\n"

        with redirect_stdin(MyInStream()):
            self.assertEqual(input(), "foobar")

    def test_input_keeps_trailing_non_newline_char(self):
        class MyInStream:
            def readline(self):
                return "foobarX"

        with redirect_stdin(MyInStream()):
            self.assertEqual(input(), "foobarX")

    def test_input_does_not_strip_trailing_cr_nl(self):
        class MyInStream:
            def readline(self):
                return "foobar\r\n"

        with redirect_stdin(MyInStream()):
            self.assertEqual(input(), "foobar\r")

    def test_input_with_empty_input_raises_eof_error(self):
        class MyInStream:
            def readline(self):
                return ""

        with redirect_stdin(MyInStream()):
            self.assertRaises(EOFError, input)

    def test_input_with_prompt_writes_prompt(self):
        class MyInStream:
            def readline(self):
                return "foobar\n"

        class MyOutStream:
            def write(self, value):
                self.value = value

        with redirect_stdin(MyInStream()):
            with contextlib.redirect_stdout(MyOutStream()):
                self.assertEqual(input("> "), "foobar")
                self.assertEqual(sys.stdout.value, "> ")

    def test_input_calls_stdout_flush(self):
        class MyInStream:
            def readline(self):
                return "foobar\n"

        class MyOutStream:
            flush = Mock(name="flush")

        with redirect_stdin(MyInStream()):
            with contextlib.redirect_stdout(MyOutStream()):
                self.assertEqual(input(), "foobar")
        MyOutStream.flush.assert_called_once()

    def test_input_calls_stderr_flush(self):
        class MyInStream:
            def readline(self):
                return "foobar\n"

        class MyOutStream:
            flush = Mock(name="flush")

        with redirect_stdin(MyInStream()):
            with contextlib.redirect_stderr(MyOutStream()):
                self.assertEqual(input(), "foobar")
        MyOutStream.flush.assert_called_once()


@pyro_only
class InstanceProxyTests(unittest.TestCase):
    def test_dunder_eq_with_id_equal_proxy_returns_true(self):
        class C:
            pass

        instance = C()
        proxy = instance.__dict__
        self.assertIs(instance_proxy.__eq__(proxy, proxy), True)

    def test_dunder_eq_with_non_dict_other_returns_not_implemented(self):
        class C:
            pass

        instance = C()
        proxy = instance.__dict__
        self.assertIs(instance_proxy.__eq__(proxy, 5), NotImplemented)

    def test_dunder_eq_with_unequal_length_proxy_returns_false(self):
        class C:
            pass

        left = C()
        right = C()
        right.a = 1
        self.assertIs(instance_proxy.__eq__(left.__dict__, right.__dict__), False)

    def test_dunder_eq_with_unequal_length_dict_returns_false(self):
        class C:
            pass

        left = C()
        self.assertIs(instance_proxy.__eq__(left.__dict__, {"a": 1}), False)

    def test_dunder_eq_with_empty_proxies_returns_true(self):
        class C:
            pass

        left = C()
        right = C()
        self.assertIs(instance_proxy.__eq__(left.__dict__, right.__dict__), True)

    def test_dunder_eq_with_empty_proxy_and_empty_dict_returns_true(self):
        class C:
            pass

        left = C()
        self.assertIs(instance_proxy.__eq__(left.__dict__, {}), True)

    def test_dunder_eq_with_key_not_in_other_returns_false(self):
        class C:
            pass

        left = C()
        left.a = 1
        right = C()
        right.b = 2
        self.assertIs(instance_proxy.__eq__(left.__dict__, right.__dict__), False)

    def test_dunder_eq_with_key_not_in_other_dict_returns_false(self):
        class C:
            pass

        left = C()
        left.a = 1
        self.assertIs(instance_proxy.__eq__(left.__dict__, {"b": 2}), False)

    def test_dunder_eq_with_value_not_equal_in_other_returns_false(self):
        class C:
            pass

        left = C()
        left.a = 1
        right = C()
        right.a = 2
        self.assertIs(instance_proxy.__eq__(left.__dict__, right.__dict__), False)

    def test_dunder_eq_with_value_not_equal_in_other_dict_returns_false(self):
        class C:
            pass

        left = C()
        left.a = 1
        self.assertIs(instance_proxy.__eq__(left.__dict__, {"a": 2}), False)

    def test_dunder_eq_with_key_and_value_equal_in_other_returns_true(self):
        class C:
            pass

        left = C()
        left.a = 1
        right = C()
        right.a = 1
        self.assertIs(instance_proxy.__eq__(left.__dict__, right.__dict__), True)

    def test_dunder_eq_with_key_and_value_equal_in_other_dict_returns_true(self):
        class C:
            pass

        left = C()
        left.a = 1
        self.assertIs(instance_proxy.__eq__(left.__dict__, {"a": 1}), True)

    def test_dunder_eq_with_id_equal_values_returns_true(self):
        # There's no test for id equal keys but overridden __eq__ because we
        # don't support str subclasses with overridden __eq__ for attributes
        class C:
            pass

        class D:
            def __eq__(self, other):
                raise RuntimeError("foo")

        left = C()
        value = D()
        left.a = value
        self.assertIs(instance_proxy.__eq__(left.__dict__, {"a": value}), True)

    def test_dunder_init_with_non_instance_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            builtins.instance_proxy(42)
        self.assertEqual(
            str(context.exception),
            "'__init__' requires a 'instance' object but received a 'int'",
        )

    def test_copy_with_empty_instance_returns_empty_dict(self):
        class C:
            pass

        result = C().__dict__.copy()
        self.assertIsInstance(result, dict)
        self.assertEqual(result, {})

    def test_copy_returns_dict_with_attributes(self):
        class C:
            pass

        instance = C()
        instance.a = 1
        instance.b = 2
        result = instance.__dict__.copy()
        self.assertIsInstance(result, dict)
        self.assertIsNot(result, instance.__dict__)
        self.assertEqual(result, {"a": 1, "b": 2})

    def test_get_with_extant_key_returns_value(self):
        class C:
            pass

        left = C()
        value = object()
        left.a = value
        self.assertIs(instance_proxy.get(left.__dict__, "a"), value)

    def test_get_with_nonexistent_key_returns_none(self):
        class C:
            pass

        left = C()
        self.assertIs(instance_proxy.get(left.__dict__, "a"), None)

    def test_get_with_nonexistent_key_returns_default(self):
        class C:
            pass

        left = C()
        value = object()
        self.assertIs(instance_proxy.get(left.__dict__, "a", value), value)

    def test_dunder_repr_with_recursive_instance_prints_ellipsis(self):
        class C:
            pass

        instance = C()
        instance.a = 1
        instance.b = instance.__dict__
        self.assertEqual(
            instance_proxy.__repr__(instance.__dict__),
            "instance_proxy({'a': 1, 'b': instance_proxy({'a': 1, 'b': instance_proxy({...})})})",  # noqa
        )


class InstanceTests(unittest.TestCase):
    def test_dunder_dict_updates_instance_attributes(self):
        class C:
            def __init__(self):
                self.foo = 10
                self.bar = 20

        c = C()
        d = {"bar": -10, "baz": -20}
        c.__dict__ = d

        self.assertFalse(hasattr(c, "foo"))
        # Existing "bar" was overwritten.
        self.assertTrue(hasattr(c, "bar"))
        self.assertTrue(hasattr(c, "baz"))

        self.assertEqual(c.bar, -10)
        self.assertEqual(c.baz, -20)

        d["foo"] = "1"
        d["bar"] = "2"
        d["baz"] = "3"

        self.assertEqual(c.foo, "1")
        self.assertEqual(c.bar, "2")
        self.assertEqual(c.baz, "3")

    def test_dunder_dict_updates_instances_during_init(self):
        class C:
            shared_defaults = {"a": 42, "b": 13}

            def __init__(self):
                self.__dict__ = C.shared_defaults

        c0 = C()
        self.assertEqual(c0.a, 42)
        self.assertEqual(c0.b, 13)

        c1 = C()
        c0.new_attribute = 99

        self.assertEqual(c1.new_attribute, 99)

    def test_dunder_dict_updates_with_non_dict_raises_type_error(self):
        class C:
            pass

        c = C()
        with self.assertRaises(TypeError) as context:
            c.__dict__ = None
        self.assertEqual(
            str(context.exception),
            "__dict__ must be set to a dictionary, not a 'NoneType'",
        )

    def test_dunder_dict_keys_with_instance_with_dict_overflow_returns_keys(self):
        class C:
            def __init__(self):
                self.a = 1
                self.b = 2

        instance = C()
        # This is the current idiomatic way to force an instance's layout into
        # the dict overflow state.
        instance.__dict__ = {"hello": "world"}
        self.assertEqual(list(instance.__dict__.keys()), ["hello"])


class IsInstanceTests(unittest.TestCase):
    @staticmethod
    def get_isinstance_error_message():
        if sys.implementation.name == "cpython" and sys.version_info < (3, 8):
            return "isinstance() arg 2 must be a type or tuple of types"
        else:
            return "isinstance() arg 2 must be a type, a tuple of types or a union"

    def test_isinstance_with_same_types_returns_true(self):
        self.assertIs(isinstance(1, int), True)

    def test_isinstance_with_subclass_returns_true(self):
        self.assertIs(isinstance(False, int), True)

    def test_isinstance_with_superclass_returns_false(self):
        self.assertIs(isinstance(2, bool), False)

    def test_isinstance_with_type_and_metaclass_returns_true(self):
        self.assertIs(isinstance(list, type), True)

    def test_isinstance_with_type_returns_true(self):
        self.assertIs(isinstance(type, type), True)

    def test_isinstance_with_object_type_returns_true(self):
        self.assertIs(isinstance(object, object), True)

    def test_isinstance_with_int_type_returns_false(self):
        self.assertIs(isinstance(int, int), False)

    def test_isinstance_with_unrelated_types_returns_false(self):
        self.assertIs(isinstance(int, (dict, bytes, str)), False)

    def test_isinstance_with_superclass_tuple_returns_true(self):
        self.assertIs(isinstance(True, (int, "bad - not a type")), True)

    def test_isinstance_with_non_type_superclass_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            isinstance(4, "bad - not a type")
        self.assertEqual(
            str(context.exception),
            self.get_isinstance_error_message(),
        )

    def test_isinstance_with_non_type_in_tuple_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            isinstance(5, ("bad - not a type", int))
        self.assertEqual(
            str(context.exception),
            self.get_isinstance_error_message(),
        )

    def test_isinstance_with_multiple_inheritance_returns_true(self):
        class A:
            pass

        class B(A):
            pass

        class C(A):
            pass

        class D(B, C):
            pass

        d = D()

        # D() is an instance of all specified superclasses
        self.assertIs(isinstance(d, A), True)
        self.assertIs(isinstance(d, B), True)
        self.assertIs(isinstance(d, C), True)
        self.assertIs(isinstance(d, D), True)

        # D() is not an instance of builtin types except object
        self.assertIs(isinstance(d, object), True)
        self.assertIs(isinstance(d, list), False)

        # D is an instance type, but D() is not
        self.assertIs(isinstance(D, type), True)
        self.assertIs(isinstance(d, type), False)

    def test_isinstance_with_type_checks_instance_type_and_dunder_class(self):
        class A(int):
            __class__ = list

        a = A()
        self.assertIs(isinstance(a, int), True)
        self.assertIs(isinstance(a, list), True)

    def test_isinstance_with_nontype_checks_dunder_bases_and_dunder_class(self):
        class A:
            __bases__ = ()

        a = A()

        class B:
            __bases__ = (a,)

        b = B()

        class C(int):
            __class__ = b
            __bases__ = (int,)

        c = C()
        self.assertIs(isinstance(c, a), True)
        self.assertIs(isinstance(c, b), True)
        self.assertIs(isinstance(c, c), False)

    def test_isinstance_with_non_tuple_dunder_bases_raises_type_error(self):
        class A:
            __bases__ = 5

        with self.assertRaises(TypeError) as context:
            isinstance(5, A())
        self.assertEqual(
            str(context.exception),
            self.get_isinstance_error_message(),
        )

    def test_isinstance_calls_custom_instancecheck_true(self):
        class Meta(type):
            def __instancecheck__(cls, obj):
                return [1]

        class A(metaclass=Meta):
            pass

        self.assertIs(isinstance(0, A), True)

    def test_isinstance_calls_custom_instancecheck_false(self):
        class Meta(type):
            def __instancecheck__(cls, obj):
                return None

        class A(metaclass=Meta):
            pass

        class B(A):
            pass

        self.assertIs(isinstance(A(), A), True)
        self.assertIs(isinstance(B(), A), False)

    def test_isinstance_with_raising_instancecheck_propagates_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("failed")

        class Meta(type):
            __instancecheck__ = Desc()

        class A(metaclass=Meta):
            pass

        with self.assertRaises(AttributeError) as context:
            isinstance(2, A)
        self.assertEqual(str(context.exception), "failed")

    def test_dunder_dict_copy_with_empty_instance_returns_empty_dict(self):
        class C:
            pass

        result = C().__dict__.copy()
        self.assertIsInstance(result, dict)
        self.assertEqual(result, {})

    def test_dunder_dict_copy_returns_dict_with_attributes(self):
        class C:
            pass

        instance = C()
        instance.a = 1
        instance.b = 2
        result = instance.__dict__.copy()
        self.assertIsInstance(result, dict)
        self.assertEqual(result, {"a": 1, "b": 2})


class IsSubclassTests(unittest.TestCase):
    def test_issubclass_with_same_types_returns_true(self):
        self.assertIs(issubclass(int, int), True)

    def test_issubclass_with_subclass_returns_true(self):
        self.assertIs(issubclass(bool, int), True)

    def test_issubclass_with_superclass_returns_false(self):
        self.assertIs(issubclass(int, bool), False)

    def test_issubclass_with_unrelated_types_returns_false(self):
        self.assertIs(issubclass(int, (dict, bytes, str)), False)

    def test_issubclass_with_superclass_tuple_returns_true(self):
        self.assertIs(issubclass(bool, (int, "bad - not a type")), True)

    def test_issubclass_with_non_type_subclass_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            issubclass("bad - not a type", str)
        self.assertEqual(str(context.exception), "issubclass() arg 1 must be a class")

    def test_issubclass_with_non_type_superclass_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            issubclass(int, "bad - not a type")
        self.assertEqual(
            str(context.exception),
            "issubclass() arg 2 must be a class or tuple of classes",
        )

    def test_issubclass_with_non_type_in_tuple_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            issubclass(bool, ("bad - not a type", int))
        self.assertEqual(
            str(context.exception),
            "issubclass() arg 2 must be a class or tuple of classes",
        )

    def test_issubclass_with_non_type_subclass_uses_bases(self):
        class A:
            __bases__ = (list,)

        self.assertIs(issubclass(A(), list), True)

    def test_issubclass_calls_custom_subclasscheck_true(self):
        class Meta(type):
            def __subclasscheck__(cls, subclass):
                return 1

        class A(metaclass=Meta):
            pass

        self.assertIs(issubclass(list, A), True)

    def test_issubclass_calls_custom_subclasscheck_false(self):
        class Meta(type):
            def __subclasscheck__(cls, subclass):
                return []

        class A(metaclass=Meta):
            pass

        class B(A):
            pass

        self.assertIs(issubclass(B, A), False)

    def test_issubclass_with_raising_subclasscheck_propagates_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("failed")

        class Meta(type):
            __subclasscheck__ = Desc()

        class A(metaclass=Meta):
            pass

        with self.assertRaises(AttributeError) as context:
            issubclass(bool, A)
        self.assertEqual(str(context.exception), "failed")


class IterTests(unittest.TestCase):
    def test_iter_with_no_dunder_iter_raises_type_error(self):
        class C:
            pass

        c = C()

        with self.assertRaises(TypeError) as context:
            iter(c)

        self.assertEqual(str(context.exception), "'C' object is not iterable")

    def test_iter_with_dunder_iter_calls_dunder_iter(self):
        class C:
            def __iter__(self):
                raise UserWarning("foo")

        c = C()

        with self.assertRaises(UserWarning) as context:
            iter(c)

        self.assertEqual(str(context.exception), "foo")

    def test_iter_with_raising_descriptor_dunder_iter_raises_type_error(self):
        dunder_get_called = False

        class Desc:
            def __get__(self, obj, type):
                nonlocal dunder_get_called
                dunder_get_called = True
                raise UserWarning("foo")

        class C:
            __iter__ = Desc()

        c = C()

        with self.assertRaises(TypeError) as context:
            iter(c)

        self.assertEqual(str(context.exception), "'C' object is not iterable")
        self.assertTrue(dunder_get_called)

    def test_iter_with_non_iterator_raises_type_error(self):
        class NonIter:
            pass

        class Foo:
            def __iter__(self):
                return NonIter()

        foo = Foo()
        with self.assertRaises(TypeError) as context:
            iter(foo)
        self.assertEqual(
            str(context.exception), "iter() returned non-iterator of type 'NonIter'"
        )

    def test_iter_with_none_dunder_iter_raises_type_error(self):
        class Foo:
            __iter__ = None

        foo = Foo()
        with self.assertRaises(TypeError) as context:
            iter(foo)
        self.assertEqual(str(context.exception), "'Foo' object is not iterable")


class LenTests(unittest.TestCase):
    def test_len_without_class_dunder_len_raises_type_error(self):
        class Foo:
            pass

        foo = Foo()
        foo.__len__ = lambda: 0
        with self.assertRaises(TypeError) as context:
            len(foo)
        self.assertEqual(str(context.exception), "object of type 'Foo' has no len()")

    def test_len_without_non_int_dunder_len_raises_type_error(self):
        class Foo:
            def __len__(self):
                return "not an int"

        foo = Foo()
        with self.assertRaises(TypeError) as context:
            len(foo)
        self.assertEqual(
            str(context.exception), "'str' object cannot be interpreted as an integer"
        )

    def test_len_with_dunder_len_returns_int(self):
        class Foo:
            def __len__(self):
                return 5

        self.assertEqual(len(Foo()), 5)

    def test_len_with_list_returns_list_length(self):
        self.assertEqual(len([1, 2, 3]), 3)

    def test_len_with_non_container_raises_type_error(self):
        self.assertRaises(TypeError, len, 1)


class ListTests(unittest.TestCase):
    def test_list_with_empty_iterable(self):
        actual = list()  # noqa: C408
        self.assertEqual(actual, [])

    def test_list_with_iterable_as_list(self):
        actual = list([1, 2, 3])  # noqa: C410
        self.assertEqual(actual, [1, 2, 3])

    def test_list_with_iterable_as_tuple(self):
        actual = list((1, 2, 3))  # noqa: C410
        self.assertEqual(actual, [1, 2, 3])

    def test_list_with_general_iterable(self):
        actual = list({1: -1, 2: -1, 3: -1})  # noqa: C410
        self.assertEqual(actual, [1, 2, 3])


class LocalsTests(unittest.TestCase):
    def test_returns_local_vars(self):
        def foo():
            a = 4
            b = 5
            return locals()

        result = foo()
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result), 2)
        self.assertEqual(result["a"], 4)
        self.assertEqual(result["b"], 5)

    def test_returns_free_vars(self):
        def foo():
            a = 4

            def bar():
                nonlocal a
                a = 5
                return locals()

            return bar()

        result = foo()
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result), 1)
        self.assertEqual(result["a"], 5)

    def test_returns_cell_vars(self):
        def foo():
            a = 4

            def bar(b):
                return a + b

            return locals()

        result = foo()
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result), 2)
        self.assertEqual(result["a"], 4)
        from types import FunctionType

        self.assertIsInstance(result["bar"], FunctionType)

    def test_caller_locals_in_class_body_returns_dict_with_class_variables(self):
        class C:
            foo = 4
            bar = 5
            baz = locals()

        result = C.baz
        self.assertIsInstance(result, dict)
        self.assertEqual(result["foo"], 4)
        self.assertEqual(result["bar"], 5)

    def test_caller_locals_in_exec_scope_returns_given_locals_instance(self):
        result_key = None
        result_value = None

        class C:
            def __getitem__(self, key):
                if key == "locals":
                    return locals
                raise Exception

            def __setitem__(self, key, value):
                nonlocal result_key
                nonlocal result_value
                result_key = key
                result_value = value

        c = C()

        exec("result = locals()", {}, c)
        self.assertEqual(result_key, "result")
        self.assertIs(result_value, c)


class LongRangeIteratorTests(unittest.TestCase):
    def test_dunder_iter_returns_self(self):
        large_int = 2 ** 123
        it = iter(range(large_int))
        self.assertEqual(iter(it), it)

    def test_dunder_length_hint_returns_pending_length(self):
        large_int = 2 ** 123
        it = iter(range(large_int))
        self.assertEqual(it.__length_hint__(), large_int)
        it.__next__()
        self.assertEqual(it.__length_hint__(), large_int - 1)

    def test_dunder_next_returns_ints(self):
        large_int = 2 ** 123
        it = iter(range(large_int))
        for i in [0, 1, 2, 3]:
            self.assertEqual(it.__next__(), i)


class MappingProxyTests(unittest.TestCase):
    def setUp(self):
        self.mappingproxy_type = type(type.__dict__)
        self.proxy = self.mappingproxy_type({"a": 4})

    def test_dunder_contains_with_non_mappingproxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.mappingproxy_type.__contains__(None, None)

    def test_dunder_contains_returns_result_from_mapping(self):
        self.assertTrue("a" in self.proxy)
        self.assertFalse("b" in self.proxy)

    def test_dunder_delitem_doesn_not_exist(self):
        self.assertFalse(hasattr(self.mappingproxy_type, "__delitem__"))

    def test_dunder_getitem_with_non_mappingproxy_type_error(self):
        with self.assertRaises(TypeError):
            self.mappingproxy_type.__getitem__(None, None)

    def test_dunder_getitem_from_mapping(self):
        self.assertEqual(self.proxy["a"], 4)

    def test_dunder_init_with_non_mapping_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.mappingproxy_type(1)
        self.assertIn(
            "mappingproxy() argument must be a mapping, not int", str(context.exception)
        )

    def test_dunder_init_with_mapping_returns_mappingproxy(self):
        m = self.mappingproxy_type({"age": 999, "dexterity": -34})
        self.assertIsInstance(m, self.mappingproxy_type)

    def test_dunder_iter_with_non_mappingproxy_type_error(self):
        with self.assertRaises(TypeError):
            self.mappingproxy_type.__iter__(None)

    def test_dunder_iter_returns_result_from_mapping(self):
        it = iter(self.proxy)
        self.assertEqual(next(it), "a")
        with self.assertRaises(StopIteration):
            next(it)

    def test_dunder_len_with_non_mappingproxy_type_error(self):
        with self.assertRaises(TypeError):
            self.mappingproxy_type.__len__(None)

    def test_dunder_len_returns_result_from_mapping(self):
        self.assertEqual(len(self.proxy), 1)

    def test_dunder_repr_with_non_mappingproxy_type_error(self):
        with self.assertRaises(TypeError):
            self.mappingproxy_type.__repr__(None)

    def test_dunder_repr_returns_result_from_mapping_with_mappingproxy_prefix(self):
        self.assertEqual(repr(self.proxy), "mappingproxy({'a': 4})")

    def test_dunder_setitem_doesn_not_exist(self):
        self.assertFalse(hasattr(self.mappingproxy_type, "__setitem__"))

    def test_clear_doesn_not_exist(self):
        self.assertFalse(hasattr(self.mappingproxy_type, "clear"))

    def test_copy_with_non_mappingproxy_type_error(self):
        with self.assertRaises(TypeError):
            self.mappingproxy_type.copy(None)

    def test_items_returns_result_from_mapping(self):
        it = iter(self.proxy.items())
        self.assertEqual(next(it), ("a", 4))
        with self.assertRaises(StopIteration):
            next(it)

    def test_keys_with_non_mappingproxy_type_error(self):
        with self.assertRaises(TypeError):
            self.mappingproxy_type.keys(None)

    def test_keys_returns_result_from_mapping(self):
        it = iter(self.proxy.keys())
        self.assertEqual(next(it), "a")
        with self.assertRaises(StopIteration):
            next(it)

    def test_pop_doesn_not_exist(self):
        self.assertFalse(hasattr(self.mappingproxy_type, "pop"))

    def test_popitem_doesn_not_exist(self):
        self.assertFalse(hasattr(self.mappingproxy_type, "popitem"))

    def test_setdefault_doesn_not_exist(self):
        self.assertFalse(hasattr(self.mappingproxy_type, "setdefault"))

    def test_update_doesn_not_exist(self):
        self.assertFalse(hasattr(self.mappingproxy_type, "update"))

    def test_values_with_non_mappingproxy_type_error(self):
        with self.assertRaises(TypeError):
            self.mappingproxy_type.values(None)

    def test_values_with_non_mappingproxy_type_error(self):
        it = iter(self.proxy.values())
        self.assertEqual(next(it), 4)
        with self.assertRaises(StopIteration):
            next(it)


class MemoryviewTests(unittest.TestCase):
    def test_strides_returns_default_value(self):
        view = memoryview(b"foobar")
        self.assertEqual(view.strides, (1,))

    def test_shape_returns_default_value(self):
        view = memoryview(b"foobar")
        self.assertEqual(view.shape, (6,))

    def test_dunder_eq_with_non_memoryview_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            memoryview.__eq__(1, 2)

    def test_dunder_eq_with_non_memoryview_other_returns_not_implemented(self):
        view = memoryview(b"foobar")
        self.assertIs(view.__eq__(1), NotImplemented)

    def test_dunder_eq_with_same_object_returns_true(self):
        view = memoryview(b"foobar")
        self.assertIs(True, view.__eq__(view))

    def test_dunder_eq_with_same_bytes_object_returns_true(self):
        content = b"foobar"
        view = memoryview(content)
        comp = memoryview(content)
        self.assertIs(True, view.__eq__(comp))

    def test_dunder_eq_with_matching_bytes_returns_true(self):
        content = b"foobar"
        view = memoryview(content)
        self.assertIs(True, view.__eq__(content))

    def test_dunder_eq_with_matching_bytearray_returns_true(self):
        content = bytearray(b"foobar")
        view = memoryview(content)
        self.assertIs(True, view.__eq__(content))

    def test_dunder_eq_with_sliced_memoryview_returns_false(self):
        view = memoryview(b"123456")
        sliced = view[:2]
        result = memoryview(b"12")
        self.assertIs(True, sliced.__eq__(result))

    def test_dunder_eq_different_memoryview_values_returns_false(self):
        view = memoryview(b"123456")
        comp = memoryview(b"135")
        self.assertIs(False, view.__eq__(comp))

    def test_dunder_enter_and_dunder_exit(self):
        view = memoryview(b"foobar")
        with view as ctx:
            self.assertIs(view, ctx)

    def test_format_returns_format_string(self):
        view = memoryview(b"asde")
        self.assertEqual(view.format, "B")
        self.assertEqual(view.cast("h").format, "h")

    def test_format_is_read_only(self):
        view = memoryview(b"asde")
        with self.assertRaises(AttributeError):
            view.format = "h"

    def test_object_with_bytes_returns_original_object(self):
        b = b"foo"
        bytes_mem = memoryview(b)
        self.assertIs(bytes_mem.obj, b)

        ba = bytearray(b"foo")
        bytearray_mem = memoryview(ba)
        self.assertIs(bytearray_mem.obj, ba)

    def test_object_with_bytes_subclass_returns_original_object(self):
        class BytesSub(bytes):
            pass

        bytes_sub = BytesSub(b"foo")
        bytes_sub_mem = memoryview(bytes_sub)
        self.assertIs(bytes_sub_mem.obj, bytes_sub)

    def test_object_with_mmap_returns_original_object(self):
        from mmap import mmap

        memory = mmap(-1, 4)
        memory_mem = memoryview(memory)
        self.assertIs(memory_mem.obj, memory)
        memory_mem_mem = memoryview(memory_mem)[1:]
        self.assertIs(memory_mem_mem.obj, memory)

    def test_dunder_getitem_with_non_memoryview_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            memoryview.__getitem__(None, 1)
        self.assertIn("requires a 'memoryview' object", str(context.exception))

    def test_dunder_getitem_index_too_large_raises_index_error(self):
        view = memoryview(b"12345678").cast("I")
        with self.assertRaises(IndexError) as context:
            memoryview.__getitem__(view, 2)
        self.assertIn("index out of bounds", str(context.exception))

    def test_dunder_getitem_index_too_small_raises_index_error(self):
        view = memoryview(bytes())
        with self.assertRaises(IndexError) as context:
            memoryview.__getitem__(view, -1)
        self.assertIn("index out of bounds", str(context.exception))

    def test_dunder_getitem_with_positive_index_returns_value(self):
        view = memoryview(b"hello")
        self.assertEqual(view[1], ord("e"))

    def test_dunder_getitem_with_positive_index_returns_value_on_sliced_mv(self):
        view = memoryview(b"drive")
        sliced = view[2:]
        self.assertEqual(sliced[1], ord("v"))

    def test_dunder_getitem_with_negative_index_returns_value(self):
        view = memoryview(b"negative")
        self.assertEqual(view[-2], ord("v"))

    def test_dunder_getitem_with_negative_index_returns_value_on_sliced_mv(self):
        view = memoryview(b"negativo")
        sliced = view[:3]
        self.assertEqual(sliced[-2], ord("e"))

    def test_dunder_getitem_with_negative_index_relative_to_end_value(self):
        view = memoryview(b"bam")
        self.assertEqual(memoryview.__getitem__(view, -3), ord("b"))

    def test_dunder_getitem_with_valid_indices_returns_submemoryview(self):
        view = memoryview(b"movie")
        self.assertEqual(memoryview.__getitem__(view, slice(2, -1)), b"vi")

    def test_dunder_getitem_with_negative_start_returns_trailing(self):
        view = memoryview(b"bam")
        self.assertEqual(memoryview.__getitem__(view, slice(-2, 5)), b"am")

    def test_dunder_getitem_with_positive_stop_returns_leading(self):
        view = memoryview(b"bam")
        self.assertEqual(memoryview.__getitem__(view, slice(2)), b"ba")

    def test_dunder_getitem_with_negative_stop_returns_all_but_trailing(self):
        view = memoryview(b"bam")
        self.assertEqual(memoryview.__getitem__(view, slice(-2)), b"b")

    def test_dunder_getitem_with_large_negative_start_returns_copy(self):
        view = memoryview(b"bam")
        self.assertEqual(memoryview.__getitem__(view, slice(-10, 10)), view)

    def test_dunder_getitem_with_large_positive_start_returns_empty(self):
        view = memoryview(b"bam")
        self.assertEqual(memoryview.__getitem__(view, slice(10, 10)), b"")

    def test_dunder_getitem_with_large_negative_stop_returns_empty(self):
        view = memoryview(b"bam")
        self.assertEqual(memoryview.__getitem__(view, slice(-10)), b"")

    def test_dunder_getitem_with_int_subclass_does_not_call_dunder_index(self):
        class C(int):
            def __index__(self):
                raise ValueError("foo")

        view = memoryview(b"asde")
        self.assertEqual(memoryview.__getitem__(view, C(0)), ord("a"))

    def test_dunder_getitem_with_raising_descriptor_propagates_exception(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("foo")

        class C:
            __index__ = Desc()

        view = memoryview(b"asde")
        with self.assertRaises(AttributeError) as context:
            memoryview.__getitem__(view, C())
        self.assertEqual(str(context.exception), "foo")

    def test_dunder_getitem_with_non_int_raises_type_error(self):
        view = memoryview(b"asde")
        with self.assertRaises(TypeError):
            memoryview.__getitem__(view, "3")

    def test_dunder_getitem_with_dunder_index_calls_dunder_index(self):
        class C:
            def __index__(self):
                return 2

        view = memoryview(b"asde")
        self.assertEqual(memoryview.__getitem__(view, C()), ord("d"))

    def test_dunder_getitem_with_class_with_index(self):
        class IndSet:
            def __index__(self):
                return 0

        view = memoryview(b"hello")
        self.assertEqual(view[IndSet()], ord("h"))

    def test_dunder_new_with_bytes_subclass_returns_memoryview(self):
        class C(bytes):
            pass

        view = memoryview(C(b"hello"))
        self.assertIsInstance(view, memoryview)
        self.assertEqual(view, b"hello")

    def test_itemsize_returns_size_of_item_chars(self):
        src = b"abcd"
        view = memoryview(src)
        self.assertEqual(view.itemsize, 1)

    def test_itemsize_returns_size_of_item_ints(self):
        src = b"abcdefgh"
        view = memoryview(src).cast("i")
        self.assertEqual(view.itemsize, 4)

    def test_mmap_creates_memoryview_with_correct_permissions(self):
        import mmap

        writable = mmap.mmap(-1, 6)
        writable_view = memoryview(writable)
        self.assertEqual(writable_view.readonly, False)
        self.assertEqual(writable_view.format, "B")
        self.assertEqual(writable_view.nbytes, 6)
        del writable_view
        writable.close()

        readable = mmap.mmap(-1, 6, access=mmap.ACCESS_READ)
        readable_view = memoryview(readable)
        self.assertEqual(readable_view.readonly, True)
        self.assertEqual(readable_view.format, "B")
        self.assertEqual(readable_view.nbytes, 6)
        del readable_view
        readable.close()

    def test_nbytes_returns_size_of_memoryview(self):
        view = memoryview(b"foobar")
        self.assertEqual(view.nbytes, 6)

    def test_release_does_nothing(self):
        view = memoryview(b"foobar")
        view.release()

    def test_setitem_with_non_memoryview_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            memoryview.__setitem__(None, 1, 1)
        self.assertIn("requires a 'memoryview' object", str(context.exception))

    def test_setitem_with_bytes_memoryview_raises_type_error(self):
        view = memoryview(b"foobar")
        with self.assertRaises(TypeError) as context:
            view[0] = 1
        self.assertIn("cannot modify read-only memory", str(context.exception))

    def test_setitem_with_too_big_index_raises_index_error(self):
        view = memoryview(bytearray(b"12345678")).cast("I")
        with self.assertRaises(IndexError) as context:
            view[2] = None
        self.assertIn("index out of bounds", str(context.exception))

    def test_setitem_with_negative_index(self):
        view = memoryview(bytearray(b"hello"))
        m = ord("m")
        view[-2] = m
        self.assertEqual(view[3], m)

    def test_setitem_with_class_with_index(self):
        class IndSet:
            def __index__(self):
                return 0

        view = memoryview(bytearray(b"hello"))
        m = ord("m")
        view[IndSet()] = m
        self.assertEqual(view[0], m)

    def test_setitem_with_slices_and_non_byteslike_raises_type_error(self):
        view = memoryview(bytearray(b"hello"))
        with self.assertRaises(TypeError) as context:
            view[:1] = 0
        self.assertIn(
            "a bytes-like object is required, not 'int'", str(context.exception)
        )

    def test_setitem_with_bytes_and_different_format_raises_value_error(self):
        view = memoryview(bytearray(b"0000")).cast("h")
        with self.assertRaises(ValueError) as context:
            view[:1] = b"a"
        self.assertIn(
            "memoryview assignment: lvalue and rvalue have different structures",
            str(context.exception),
        )

    def test_setitem_with_bytearray_and_different_format_raises_value_error(self):
        view = memoryview(bytearray(b"0000")).cast("h")
        with self.assertRaises(ValueError) as context:
            view[:1] = bytearray(b"a")
        self.assertIn(
            "memoryview assignment: lvalue and rvalue have different structures",
            str(context.exception),
        )

    def test_setitem_with_memoryview_and_different_format_raises_value_error(self):
        view = memoryview(bytearray(b"0000")).cast("h")
        with self.assertRaises(ValueError) as context:
            view[:1] = memoryview(b"a")
        self.assertIn(
            "memoryview assignment: lvalue and rvalue have different structures",
            str(context.exception),
        )

    def test_setitem_with_byteslike_of_wrong_size_raises_value_error(self):
        view = memoryview(bytearray(b"0000"))
        with self.assertRaises(ValueError) as context:
            view[:2] = memoryview(b"a")
        self.assertIn(
            "memoryview assignment: lvalue and rvalue have different structures",
            str(context.exception),
        )

    def test_setitem_with_zero_len_slice_and_empty_bytes_raises_type_error(self):
        content = b"foobar"
        view = memoryview(content)
        with self.assertRaises(TypeError) as context:
            view[:0] = b""
        self.assertIn("cannot modify read-only memory", str(context.exception))

    def test_setitem_with_zero_len_slice_and_nonempty_bytes_raises_value_error(self):
        content = bytearray(b"foobar")
        view = memoryview(content)
        with self.assertRaises(ValueError) as context:
            view[:0] = b"123"
        self.assertIn(
            "memoryview assignment: lvalue and rvalue have different structures",
            str(context.exception),
        )

    def test_setitem_with_zero_len_slice_and_empty_bytes_value_returns_mv_unmodified(
        self,
    ):
        content = bytearray(b"foobar")
        view = memoryview(content)
        view[:0] = b""
        self.assertEqual(view, b"foobar")

    def test_setitem_with_zero_len_slice_and_empty_mv_value_returns_mv_unmodified(self):
        content = bytearray(b"foobar")
        view = memoryview(content)
        view[:0] = memoryview(b"")
        self.assertEqual(view, b"foobar")

    def test_setitem_with_zero_len_slice_and_negative_stop_returns_mv_unmodified(self):
        content = bytearray(b"foobar")
        view = memoryview(content)
        view[:-6] = b""
        self.assertEqual(view, b"foobar")

    def test_setitem_with_slices_and_bytes_and_step_1(self):
        view = memoryview(bytearray(b"0000"))
        view[:2] = b"zz"
        self.assertEqual(view, b"zz00")

    def test_setitem_with_slices_and_bytearray_and_step_1(self):
        view = memoryview(bytearray(b"0000"))
        view[1:3] = bytearray(b"zz")
        self.assertEqual(view, b"0zz0")

    def test_setitem_with_slices_and_memoryview_byte_format_and_step_1(self):
        view = memoryview(bytearray(b"0000"))
        view[:2] = memoryview(b"zz")
        self.assertEqual(view, b"zz00")

    def test_setitem_with_slices_and_memoryview_short_format_and_step_1(self):
        view = memoryview(bytearray(b"0000"))
        short_view = view.cast("h")
        short_view[:1] = memoryview(b"zz").cast("h")
        self.assertEqual(view, b"zz00")

    def test_setitem_with_slices_and_bytes_and_step_2(self):
        view = memoryview(bytearray(b"0000"))
        view[::2] = b"zz"
        self.assertEqual(view, b"z0z0")

    def test_setitem_with_slices_and_bytearray_and_step_2(self):
        view = memoryview(bytearray(b"0000"))
        view[1::2] = bytearray(b"zz")
        self.assertEqual(view, b"0z0z")

    def test_setitem_with_slices_and_memoryview_byte_format_and_step_2(self):
        view = memoryview(bytearray(b"0000"))
        view[::2] = memoryview(b"zz")
        self.assertEqual(view, b"z0z0")

    def test_setitem_with_slices_and_memoryview_short_format_and_step_2(self):
        view = memoryview(bytearray(b"000000"))
        short_view = view.cast("h")
        short_view[1::2] = memoryview(b"zz").cast("h")
        self.assertEqual(view, b"00zz00")

    def test_tolist_with_non_memoryview_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            memoryview.tolist(None)
        self.assertIn(
            "'tolist' requires a 'memoryview' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_tolist_returns_list_of_elements(self):
        src = b"hello"
        view = memoryview(src)
        self.assertEqual(view.tolist(), [104, 101, 108, 108, 111])

    def test_tolist_withitemsize_greater_than_one(self):
        src = b"abcd"
        view = memoryview(src).cast("i")
        self.assertEqual(view.tolist(), [1684234849])

    def test_tolist_with_empty_memoryview_returns_empty_list(self):
        src = b""
        view = memoryview(src)
        self.assertEqual(view.tolist(), [])


class MethodTests(unittest.TestCase):
    def test_dunder_new_with_non_callable_func_raises_type_error(self):
        class C:
            def method(self):
                pass

        instance = C()
        method_type = type(instance.method)
        with self.assertRaises(TypeError) as context:
            method_type.__new__(method_type, 1, "foo")

        self.assertEqual(str(context.exception), "first argument must be callable")

    def test_dunder_new_with_none_self_raises_type_error(self):
        class C:
            def method(self):
                pass

        def non_method_function(self):
            return self.value

        instance = C()
        method_type = type(instance.method)
        with self.assertRaises(TypeError) as context:
            method_type.__new__(method_type, non_method_function, None)

        self.assertEqual(str(context.exception), "self must not be None")

    def test_dunder_new_returns_bound_method(self):
        class C:
            def __init__(self, value):
                self.value = value

            def method(self):
                pass

        def non_method_function(self):
            return self.value

        instance = C(10)
        method_type = type(instance.method)
        result = method_type.__new__(method_type, non_method_function, instance)
        self.assertIsInstance(result, method_type)
        self.assertEqual(result.__func__, non_method_function)
        self.assertEqual(result.__self__, instance)
        self.assertEqual(result(), 10)

    def test_has_dunder_call(self):
        class C:
            def bar(self):
                pass

        C().bar.__getattribute__("__call__")


class ModuleTests(unittest.TestCase):
    def test_del_invalidates_cache(self):
        from types import ModuleType

        mymodule = ModuleType("mymodule")
        mymodule.x = 40
        exec("def foo(): return x", mymodule.__dict__)
        result = mymodule.foo()
        self.assertEqual(result, 40)

        del mymodule.x

        with self.assertRaises(NameError) as context:
            mymodule.foo()
        self.assertEqual(str(context.exception), "name 'x' is not defined")

    def test_dunder_dict_with_non_module_raises_type_error(self):
        from types import ModuleType

        with self.assertRaises(TypeError):
            ModuleType.__dict__["__dict__"].__get__(42)

    def test_dunder_dir_returns_newly_created_list_object(self):
        from types import ModuleType

        mymodule = ModuleType("mymodule")
        self.assertEqual(type(mymodule.__dir__()), list)
        self.assertIsNot(mymodule.__dir__(), mymodule.__dir__())

    def test_dunder_dir_returns_list_containing_module_attributes(self):
        from types import ModuleType

        mymodule = ModuleType("mymodule")
        mymodule.x = 40
        mymodule.y = 50
        result = mymodule.__dir__()
        self.assertIn("x", result)
        self.assertIn("y", result)

    def test_dunder_dir_returns_list_containing_added_module_attributes(self):
        from types import ModuleType

        mymodule = ModuleType("mymodule")
        self.assertNotIn("z", mymodule.__dir__())

        mymodule.z = 60
        self.assertIn("z", mymodule.__dir__())

    def test_dunder_dir_returns_list_not_containing_deleted_module_attributes(self):
        from types import ModuleType

        mymodule = ModuleType("mymodule")
        mymodule.x = 40
        self.assertIn("x", mymodule.__dir__())

        del mymodule.x
        self.assertNotIn("x", mymodule.__dir__())

    def test_dunder_new_with_subclass_returns_object(self):
        from types import ModuleType

        class C(ModuleType):
            pass

        m = ModuleType.__new__(C)
        self.assertEqual(type(m), C)
        self.assertIsInstance(m, ModuleType)
        # Do some sanity checking that half-initialized modules do not crash.
        self.assertEqual(repr(m), "<module '?'>")
        with self.assertRaises(AttributeError) as context:
            ModuleType.__getattribute__(m, "foo")
        self.assertEqual(str(context.exception), "module has no attribute 'foo'")

    def test_dunder_new_accepts_extra_arguments(self):
        from types import ModuleType

        m = ModuleType.__new__(ModuleType, 42, "foo", bar=None)
        self.assertIsInstance(m, ModuleType)
        self.assertEqual(repr(m), "<module '?'>")

    def test_dunder_init_sets_fields(self):
        from types import ModuleType

        m = ModuleType.__new__(ModuleType)
        ModuleType.__init__(m, "foo")
        self.assertEqual(
            dir(m), ["__doc__", "__loader__", "__name__", "__package__", "__spec__"]
        )
        self.assertIs(m.__doc__, None)
        self.assertIs(m.__loader__, None)
        self.assertEqual(m.__name__, "foo")
        self.assertIs(m.__package__, None)
        self.assertIs(m.__spec__, None)

    def test_dunder_new_with_non_type_raises_type_error(self):
        from types import ModuleType

        with self.assertRaises(TypeError) as context:
            ModuleType.__new__(42, "")
        self.assertEqual(
            str(context.exception), "module.__new__(X): X is not a type object (int)"
        )

    def test_dunder_new_with_non_module_subtype_raise_type_error(self):
        from types import ModuleType

        class C:
            pass

        with self.assertRaises(TypeError) as context:
            ModuleType.__new__(C, "")
        self.assertEqual(
            str(context.exception), "module.__new__(C): C is not a subtype of module"
        )

    def test_non_module_with_module_getattribute(self):
        from types import ModuleType

        class C:
            __getattribute__ = ModuleType.__getattribute__

        c = C()
        with self.assertRaises(TypeError):
            c.foo

    def test_module_subclass_getattribute(self):
        from types import ModuleType

        class M(ModuleType):
            pass

        mod = M("a_module")
        mod.foo = "bar"
        # TODO(T58719879): Add a test that reliably uses LOAD_ATTR_MODULE as
        # appropriate in Pyro.
        for _i in range(2):
            self.assertEqual(mod.foo, "bar")


class ModuleProxyTests(unittest.TestCase):
    def setUp(self):
        from types import ModuleType

        self.module = ModuleType("test_module")
        self.module_proxy = self.module.__dict__

        # Create a placeholder in the module dict for a builtin.
        module_code = """
def make_placeholder():
    return placeholder
        """
        exec(module_code, self.module_proxy)

        builtins = ModuleType("builtins")
        builtins.placeholder = "builtin_value"
        self.module.__builtins__ = builtins
        self.assertEqual(self.module.make_placeholder(), "builtin_value")

    def test_dunder_contains_with_non_module_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.module_proxy).__contains__(None, None)

    def test_dunder_contains_returns_true_for_existing_item(self):
        self.module.x = 40
        self.assertTrue(self.module_proxy.__contains__("x"))

    def test_dunder_contains_returns_false_for_not_existing_item(self):
        self.assertFalse(self.module_proxy.__contains__("x"))

    def test_dunder_contains_returns_false_for_placeholder(self):
        self.assertFalse(self.module_proxy.__contains__("placeholder"))

    def test_dunder_repr_returns_string(self):
        from types import ModuleType

        module_proxy = ModuleType("my_module").__dict__
        for key in tuple(module_proxy.keys()):
            if key != "__name__":
                del module_proxy[key]

        self.assertEqual(module_proxy.__repr__(), "{'__name__': 'my_module'}")

    def test_dunder_repr_with_recursion_returns_string_with_ellipsis(self):
        from types import ModuleType

        module_proxy = ModuleType("my_module").__dict__
        for key in tuple(module_proxy.keys()):
            del module_proxy[key]
        module_proxy["self"] = module_proxy

        self.assertEqual(module_proxy.__repr__(), "{'self': {...}}")

    def test_copy_with_non_module_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.module_proxy).copy(None)

    def test_copy_returns_dict_copy(self):
        self.module.x = 40
        result = self.module_proxy.copy()
        self.assertEqual(type(result), dict)
        self.assertEqual(result["x"], 40)
        self.module.y = 50
        self.assertNotIn("y", result)

    def test_dunder_delitem_with_non_module_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.module_proxy.__delitem__(None, None)

    def test_dunder_delitem_deletes_module_variable_and_raises_name_error(self):
        module_code = """
x = 40
def foo():
    return x
foo()
        """
        exec(module_code, self.module_proxy)
        self.assertEqual(self.module.foo(), 40)

        self.module_proxy.__delitem__("x")

        with self.assertRaises(NameError) as context:
            self.module.foo()
        self.assertIn("name 'x' is not defined", str(context.exception))

    def test_dunder_delitem_for_non_existing_key_raises_key_error(self):
        with self.assertRaises(KeyError) as context:
            self.module_proxy.__delitem__("x")
        self.assertIn("'x'", str(context.exception))

    def test_clear_with_non_module_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.module_proxy.clear(None)

    def test_clear_deletes_module_variables_and_raises_attribute_error(self):
        module_code = """
x = 40
def foo():
    return x
foo()
        """
        exec(module_code, self.module_proxy)
        self.assertEqual(self.module.foo(), 40)

        self.module_proxy.clear()

        self.assertEqual(len(self.module_proxy), 0)
        self.assertNotIn("x", self.module_proxy)
        self.assertNotIn("foo", self.module_proxy)

        with self.assertRaises(AttributeError) as context:
            self.module.foo()
        self.assertIn("has no attribute 'foo'", str(context.exception))

    def test_dunder_getitem_with_non_module_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.module_proxy).__getitem__(None, None)

    def test_dunder_getitem_for_existing_key_returns_that_item(self):
        self.module.x = 40
        self.assertEqual(self.module_proxy.__getitem__("x"), 40)

    def test_dunder_getitem_for_not_existing_key_raises_key_error(self):
        with self.assertRaises(KeyError) as context:
            self.module_proxy.__getitem__("x")
        self.assertIn("'x'", str(context.exception))

    def test_dunder_getitem_for_placeholder_raises_key_error(self):
        with self.assertRaises(KeyError) as context:
            self.module_proxy.__getitem__("placeholder")
        self.assertIn("'placeholder'", str(context.exception))

    def test_dunder_iter_with_non_module_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.module_proxy).__iter__(None)

    def test_dunder_iter_returns_key_iterator(self):
        self.module.x = 40
        self.module.y = 50
        result = self.module_proxy.__iter__()
        self.assertTrue(hasattr(result, "__next__"))
        result_list = list(result)
        self.assertIn("x", result_list)
        self.assertIn("y", result_list)

    def test_dunder_len_with_non_module_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.module_proxy).__len__(None)

    def test_dunder_len_returns_num_items(self):
        length = self.module_proxy.__len__()
        self.module.x = 40
        self.assertEqual(self.module_proxy.__len__(), length + 1)

    def test_dunder_len_returns_num_items_excluding_placeholder(self):
        length = self.module_proxy.__len__()
        # Overwrite the existing placeholder by creating a real one under the same name.
        self.module.placeholder = 1
        self.assertEqual(self.module_proxy.__len__(), length + 1)

    def test_dunder_repr_with_non_module_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.module_proxy).__repr__(None)

    def test_dunder_repr_returns_str_containing_existing_items(self):
        self.module.x = 40
        self.module.y = 50
        result = self.module_proxy.__repr__()
        self.assertIsInstance(result, str)
        self.assertIn("'x': 40", result)
        self.assertIn("'y': 50", result)

    def test_dunder_repr_returns_str_not_containing_placeholder(self):
        result = self.module_proxy.__repr__()
        self.assertNotIn("'placeholder'", result)

    def test_dunder_setitem_with_non_module_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.module_proxy).__setitem__(None, None, None)

    def test_dunder_setitem_sets_item_showing_up_in_module(self):
        self.module_proxy.__setitem__("a", 1)
        self.assertEqual(self.module.a, 1)

    def test_dunder_setitem_with_existing_item_updates_module_variable(self):
        module_code = """
x = 40
def foo():
    return x
foo()
        """
        exec(module_code, self.module_proxy)
        self.assertEqual(self.module.foo(), 40)

        self.module_proxy.__setitem__("x", 50)
        self.assertEqual(self.module.foo(), 50)

    def test_dunder_setitem_with_placeholder_updates_module_variable(self):
        module_code = """
def foo():
    return x
        """
        exec(module_code, self.module_proxy)

        from types import ModuleType

        builtins = ModuleType("builtins")
        builtins.x = 40
        self.module.__builtins__ = builtins
        self.assertEqual(self.module.foo(), 40)

        self.module_proxy.__setitem__("x", 50)
        self.assertEqual(self.module.foo(), 50)

    def test_get_with_non_module_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.module_proxy).get(None, None)

    def test_get_returns_existing_item_value(self):
        self.module.x = 40
        self.assertEqual(self.module_proxy.get("x"), 40)

    def test_get_with_default_for_non_existing_item_value_returns_that_default(self):
        self.assertEqual(self.module_proxy.get("x", -1), -1)

    def test_get_for_non_existing_item_returns_none(self):
        self.assertIs(self.module_proxy.get("x"), None)

    def test_get_for_placeholder_returns_none(self):
        self.assertIs(self.module_proxy.get("placeholder"), None)

    def test_items_with_non_module_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.module_proxy).items(None)

    def test_items_returns_container_for_key_value_pairs(self):
        self.module.x = 40
        self.module.y = 50
        result = self.module_proxy.items()
        self.assertTrue(hasattr(result, "__iter__"))
        result_list = list(iter(result))
        self.assertIn(("x", 40), result_list)
        self.assertIn(("y", 50), result_list)

    def test_keys_with_non_module_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.module_proxy).keys(None)

    def test_keys_returns_container_for_keys(self):
        self.module.x = 40
        self.module.y = 50
        result = self.module_proxy.keys()
        self.assertTrue(hasattr(result, "__iter__"))
        result_list = list(iter(result))
        self.assertIn("x", result_list)
        self.assertIn("y", result_list)

    def test_keys_returns_key_iterator_excluding_placeholder(self):
        result = self.module_proxy.keys()
        self.assertNotIn("placeholder", result)

    def test_pop_with_non_module_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.module_proxy).pop(None)

    def test_pop_for_existing_item_deletes_and_returns_that_item_value(self):
        self.module.x = 40
        value = self.module_proxy.pop("x")
        self.assertEqual(value, 40)
        self.assertFalse(self.module_proxy.__contains__("x"))

    def test_pop_for_not_existing_item_raises_key_error(self):
        with self.assertRaises(KeyError) as context:
            self.module_proxy.pop("x")
        self.assertIn("'x'", str(context.exception))

    def test_pop_with_default_for_not_existing_item_returns_default(self):
        value = self.module_proxy.pop("x", -1)
        self.assertEqual(value, -1)

    def test_pop_for_placeholder_raises_key_error(self):
        with self.assertRaises(KeyError) as context:
            self.module_proxy.pop("placeholder")
        self.assertIn("'placeholder'", str(context.exception))

    def test_setdefault_with_non_module_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.module_proxy).setdefault(None, None)

    def test_setdefault_for_existing_item_does_nothing(self):
        self.module.x = 40
        self.module_proxy.setdefault("x", -1)
        self.assertEqual(self.module_proxy.get("x"), 40)

    def test_setdefault_for_not_existing_item_sets_default_value(self):
        self.module_proxy.setdefault("x", -1)
        self.assertEqual(self.module_proxy.get("x"), -1)

    def test_update_with_non_module_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.module_proxy).update(None)

    def test_update_with_multiple_positional_arguments_raises_type_error(self):
        with self.assertRaises(TypeError):
            self.module_proxy.update({"x": 40}, {"y": 50})

    def test_update_with_dict_updates_module_proxy_and_module(self):
        d = {"x": 40, "y": 50}
        self.assertIsNone(self.module_proxy.update(d))
        self.assertEqual(self.module_proxy.get("x"), 40)
        self.assertEqual(self.module_proxy.get("y"), 50)
        self.assertEqual(self.module.x, 40)
        self.assertEqual(self.module.y, 50)

    def test_update_with_iterable_updates_module_proxy_and_module(self):
        class Iterable:
            def __iter__(self):
                return iter([("x", 40), ("y", 50)])

        iterable = Iterable()
        self.assertIsNone(self.module_proxy.update(iterable))
        self.assertEqual(self.module_proxy.get("x"), 40)
        self.assertEqual(self.module_proxy.get("y"), 50)
        self.assertEqual(self.module.x, 40)
        self.assertEqual(self.module.y, 50)

    def test_update_with_iterable_of_non_pair_tuple_raises_value_error(self):
        class Iterable:
            def __iter__(self):
                return iter([("x", 40), ("y", 50, 60)])

        iterable = Iterable()
        with self.assertRaises(ValueError) as context:
            self.assertIsNone(self.module_proxy.update(iterable))
        self.assertIn(
            "dictionary update sequence element #1 has length 3; 2 is required",
            str(context.exception),
        )

    def test_update_with_kwargs_updates_module_proxy_and_module(self):
        self.assertNotIn("x", self.module_proxy)
        self.assertNotIn("y", self.module_proxy)
        self.assertIsNone(self.module_proxy.update(y=50, x=40))
        self.assertEqual(self.module_proxy.get("x"), 40)
        self.assertEqual(self.module_proxy.get("y"), 50)
        self.assertEqual(self.module.x, 40)
        self.assertEqual(self.module.y, 50)

    def test_update_with_dict_and_kwargs_updates_module_proxy_and_module(self):
        self.assertNotIn("x", self.module_proxy)
        self.assertNotIn("y", self.module_proxy)
        self.assertIsNone(self.module_proxy.update({"y": 50}, x=40))
        self.assertEqual(self.module_proxy.get("x"), 40)
        self.assertEqual(self.module_proxy.get("y"), 50)
        self.assertEqual(self.module.x, 40)
        self.assertEqual(self.module.y, 50)

    def test_update_with_dict_and_kwargs_gives_kwargs_precedence(self):
        self.assertNotIn("y", self.module_proxy)
        self.assertIsNone(self.module_proxy.update({"y": 50}, y=60))
        self.assertEqual(self.module_proxy.get("y"), 60)
        self.assertEqual(self.module.y, 60)

    def test_update_with_self_in_kwargs_puts_self_in_attributes(self):
        self.assertNotIn("self", self.module_proxy)
        self.assertIsNone(self.module_proxy.update(self=60))
        self.assertEqual(self.module_proxy.get("self"), 60)
        self.assertEqual(self.module.self, 60)

    def test_values_with_non_module_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.module_proxy).values(None)

    def test_values_returns_container_for_values(self):
        self.module.x = 1243314135
        self.module.y = -1243314135
        result = self.module_proxy.values()
        self.assertTrue(hasattr(result, "__iter__"))
        result_list = list(iter(result))
        self.assertIn(1243314135, result_list)
        self.assertIn(-1243314135, result_list)

    def test_values_returns_iterator_excluding_placeholder_value(self):
        result = self.module_proxy.values()
        self.assertNotIn("builtin_value", result)


class NextTests(unittest.TestCase):
    def test_next_with_raising_dunder_next_propagates_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("failed")

        class Foo:
            __next__ = Desc()

        foo = Foo()
        with self.assertRaises(AttributeError) as context:
            next(foo)
        self.assertEqual(str(context.exception), "failed")

    def test_next_with_dict_keyiterater_returns_next_item(self):
        d = {"a": 0}
        dict_keyiter = iter(d.keys())
        self.assertEqual(next(dict_keyiter), "a")

    def test_next_with_empty_dict_keyiterator_returns_default(self):
        d = {}
        dict_keyiter = iter(d.keys())
        self.assertEqual(next(dict_keyiter, None), None)

    def test_next_with_empty_dict_keyiterator_raises_stop_iteration(self):
        d = {}
        dict_keyiter = iter(d.keys())
        with self.assertRaises(StopIteration):
            next(dict_keyiter)

    def test_next_with_list_iterater_returns_next_item(self):
        li = [1]
        list_iter = iter(li)
        self.assertEqual(next(list_iter), 1)

    def test_next_with_empty_list_iterator_returns_default(self):
        li = []
        list_iter = iter(li)
        self.assertEqual(next(list_iter, None), None)

    def test_next_with_empty_list_iterator_raises_stop_iteration(self):
        li = []
        list_iter = iter(li)
        with self.assertRaises(StopIteration):
            next(list_iter)

    def test_next_with_range_iterater_returns_next_item(self):
        ran = range(1)
        range_iter = iter(ran)
        self.assertEqual(next(range_iter), 0)

    def test_next_with_empty_range_iterator_returns_default(self):
        ran = range(0)
        range_iter = iter(ran)
        self.assertEqual(next(range_iter, None), None)

    def test_next_with_empty_range_iterator_raises_stop_iteration(self):
        ran = range(0)
        range_iter = iter(ran)
        with self.assertRaises(StopIteration):
            next(range_iter)

    def test_next_with_set_iterater_returns_next_item(self):
        s = {0}
        set_iter = iter(s)
        self.assertEqual(next(set_iter), 0)

    def test_next_with_empty_set_iterator_returns_default(self):
        set_iter = iter(set())
        self.assertEqual(next(set_iter, None), None)

    def test_next_with_empty_set_iterator_raises_stop_iteration(self):
        set_iter = iter(set())
        with self.assertRaises(StopIteration):
            next(set_iter)

    def test_next_with_set_iterator_raises_stop_iteration(self):
        s = {0}
        set_iter = iter(s)
        self.assertEqual(next(set_iter), 0)
        with self.assertRaises(StopIteration):
            next(set_iter)

    def test_next_with_str_iterater_returns_next_item(self):
        str = "a"
        str_iter = iter(str)
        self.assertEqual(next(str_iter), "a")

    def test_next_with_empty_str_iterator_returns_default(self):
        str = ""
        str_iter = iter(str)
        self.assertEqual(next(str_iter, None), None)

    def test_next_with_empty_str_iterator_raises_stop_iteration(self):
        str = ""
        str_iter = iter(str)
        with self.assertRaises(StopIteration):
            next(str_iter)

    def test_next_with_str_iterator_raises_stop_iteration(self):
        str = "abc"
        str_iter = iter(str)
        self.assertEqual(next(str_iter), "a")
        self.assertEqual(next(str_iter), "b")
        self.assertEqual(next(str_iter), "c")
        with self.assertRaises(StopIteration):
            next(str_iter)

    def test_next_with_tuple_iterater_returns_next_item(self):
        t = (1,)
        tuple_iter = iter(t)
        self.assertEqual(next(tuple_iter), 1)

    def test_next_with_empty_tuple_iterator_returns_default(self):
        t = ()
        tuple_iter = iter(t)
        self.assertEqual(next(tuple_iter, None), None)

    def test_next_with_empty_tuple_iterator_raises_stop_iteration(self):
        t = ()
        tuple_iter = iter(t)
        with self.assertRaises(StopIteration):
            next(tuple_iter)


class NoneTests(unittest.TestCase):
    def test_dunder_class_returns_type_of_none(self):
        self.assertEqual(None.__class__, type(None))

    def test_dunder_class_assignment_raises_type_error(self):
        class C:
            pass

        with self.assertRaises(TypeError):
            type(None).__class__ = C


class NotImplementedTypeTests(unittest.TestCase):
    def test_repr_returns_not_implemented(self):
        self.assertEqual(NotImplemented.__repr__(), "NotImplemented")


class ObjectTests(unittest.TestCase):
    def test_reduce_with_object(self):
        result = object().__reduce__()
        self.assertEqual(len(result), 2)
        self.assertEqual(result[1], (object, object, None))

    def test_reduce_with_custom_type_without_attributes(self):
        class Foo:
            pass

        result = Foo().__reduce__()
        self.assertEqual(len(result), 2)
        self.assertEqual(result[1], (Foo, object, None))

    def test_reduce_with_custom_type(self):
        class Foo:
            def __init__(self):
                self.a = 1

        result = Foo().__reduce__()
        self.assertEqual(len(result), 3)
        self.assertEqual(result[1:], ((Foo, object, None), {"a": 1}))

    def test_reduce_with_custom_base(self):
        class Foo:
            def __init__(self):
                self.a = 1

        class Bar(Foo):
            def __init__(self):
                Foo.__init__(self)
                self.b = 2

        result = Bar().__reduce__()
        self.assertEqual(len(result), 3)
        self.assertEqual(result[1:], ((Bar, object, None), {"a": 1, "b": 2}))

    def test_reduce_with_getstate(self):
        class Foo:
            def __init__(self):
                self.a = 1

            def __getstate__(self):
                return {"b": 2}

        result = Foo().__reduce__()
        self.assertEqual(len(result), 3)
        self.assertEqual(result[1], (Foo, object, None), {"b": 2})

    def test_reduce_ex_with_object(self):
        result = object().__reduce_ex__(0)
        self.assertEqual(len(result), 2)
        self.assertEqual(result[1], (object, object, None))

    def test_reduce_ex_with_object_and_proto_three(self):
        result = object().__reduce_ex__(3)
        self.assertEqual(len(result), 5)
        self.assertEqual(result[1:], ((object,), None, None, None))

    def test_reduce_ex_with_custom_type_without_attributes(self):
        class Foo:
            pass

        result = Foo().__reduce_ex__(0)
        self.assertEqual(len(result), 2)
        self.assertEqual(result[1], (Foo, object, None))

    def test_reduce_ex_with_custom_type_without_attributes_and_proto_three(self):
        class Foo:
            pass

        result = Foo().__reduce_ex__(3)
        self.assertEqual(len(result), 5)
        self.assertEqual(result[1:], ((Foo,), None, None, None))

    def test_reduce_ex_with_custom_type(self):
        class Foo:
            def __init__(self):
                self.a = 1

        result = Foo().__reduce_ex__(0)
        self.assertEqual(len(result), 3)
        self.assertEqual(result[1], (Foo, object, None), {"a": 1})

    def test_reduce_ex_with_custom_type_and_proto_three(self):
        class Foo:
            def __init__(self):
                self.a = 1

        result = Foo().__reduce_ex__(3)
        self.assertEqual(len(result), 5)
        self.assertEqual(result[1:], ((Foo,), {"a": 1}, None, None))

    def test_reduce_ex_with_custom_base(self):
        class Foo:
            def __init__(self):
                self.a = 1

        class Bar(Foo):
            def __init__(self):
                Foo.__init__(self)
                self.b = 2

        result = Bar().__reduce_ex__(0)
        self.assertEqual(len(result), 3)
        self.assertEqual(result[1:], ((Bar, object, None), {"a": 1, "b": 2}))

    def test_reduce_ex_with_custom_base_and_proto_three(self):
        class Foo:
            def __init__(self):
                self.a = 1

        class Bar(Foo):
            def __init__(self):
                Foo.__init__(self)
                self.b = 2

        result = Bar().__reduce_ex__(3)
        self.assertEqual(len(result), 5)
        self.assertEqual(result[1:], ((Bar,), {"a": 1, "b": 2}, None, None))

    def test_reduce_ex_with_custom_reduce(self):
        class Foo:
            def __init__(self):
                self.a = 1

            def __reduce__(self):
                return (Foo, (self.a))

        result = Foo().__reduce_ex__(0)
        self.assertEqual(len(result), 2)
        self.assertEqual(result, (Foo, 1))

    def test_reduce_ex_with_custom_reduce_and_proto_three(self):
        class Foo:
            def __init__(self):
                self.a = 1

            def __reduce__(self):
                return (Foo, (self.a))

        result = Foo().__reduce_ex__(3)
        self.assertEqual(len(result), 2)
        self.assertEqual(result, (Foo, 1))

    def test_reduce_ex_with_getstate(self):
        class Foo:
            def __init__(self):
                self.a = 1

            def __getstate__(self):
                return {"b": 2}

        result = Foo().__reduce_ex__(1)
        self.assertEqual(len(result), 3)
        self.assertEqual(result[1:], ((Foo, object, None), {"b": 2}))

    def test_reduce_ex_with_getstate_and_proto_three(self):
        class Foo:
            def __init__(self):
                self.a = 1

            def __getstate__(self):
                return {"b": 2}

        result = Foo().__reduce_ex__(3)
        self.assertEqual(len(result), 5)
        self.assertEqual(result[1:], ((Foo,), {"b": 2}, None, None))

    def test_reduce_ex_with_empty_slots_and_proto_three(self):
        class Foo:
            __slots__ = ()

        result = Foo().__reduce_ex__(3)
        self.assertEqual(len(result), 5)
        self.assertEqual(result[1:], ((Foo,), None, None, None))

    def test_reduce_ex_with_slots_and_proto_three(self):
        class Foo:
            __slots__ = "a"

            def __init__(self, value):
                self.a = value

        result = Foo(1).__reduce_ex__(3)
        self.assertEqual(len(result), 5)
        self.assertEqual(result[1:], ((Foo,), (None, {"a": 1}), None, None))

    def test_reduce_ex_with_dict_and_proto_three(self):
        d = {"a": 1}
        result = d.__reduce_ex__(3)
        self.assertEqual(len(result), 5)
        self.assertEqual(result[1:4], ((dict,), None, None))
        it = result[4]
        self.assertEqual(it.__next__(), ("a", 1))
        with self.assertRaises(StopIteration):
            it.__next__()

    def test_reduce_ex_ignores_instance_reduce(self):
        class Foo:
            def __init__(self):
                self.a = 123

        def reconstruct_foo(a):
            foo = Foo()
            foo.a = a
            return foo

        def reduce_foo(self):
            return (reconstruct_foo, (self.a))

        foo = Foo()
        foo.__reduce__ = reduce_foo.__get__(foo, Foo)
        self.assertEqual(foo.__reduce__()[0], reconstruct_foo)
        import copyreg

        self.assertEqual(foo.__reduce_ex__(1)[0], copyreg._reconstructor)

    def test_reduce_ex_with_getnewargs_ex_equals_none_raises_type_error(self):
        class Foo:
            __getnewargs_ex__ = None

        with self.assertRaises(TypeError) as context:
            Foo().__reduce_ex__(3)
        self.assertEqual(str(context.exception), "'NoneType' object is not callable")

    def test_reduce_ex_with_getnewargs_equals_none_raises_type_error(self):
        class Foo:
            __getnewargs__ = None

        with self.assertRaises(TypeError) as context:
            Foo().__reduce_ex__(3)
        self.assertEqual(str(context.exception), "'NoneType' object is not callable")

    def test_reduce_ex_with_getstate_equals_none_raises_type_error(self):
        class Foo:
            __getstate__ = None

        with self.assertRaises(TypeError) as context:
            Foo().__reduce_ex__(3)
        self.assertEqual(str(context.exception), "'NoneType' object is not callable")

    def test_dunder_repr_returns_string_with_module_and_name(self):
        class C:
            pass

        self.assertTrue(
            object.__repr__(C()).startswith(
                "<__main__.ObjectTests.test_dunder_repr_returns_string_with_module_and_name.<locals>.C object at"
            )
        )

    def test_dunder_repr_returns_string_with_only_name(self):
        self.assertTrue(object.__repr__(42).startswith("<int object at "))
        self.assertTrue(object.__repr__("").startswith("<str object at "))

    def test_dir_without_dict_returns_type_attributes(self):
        o = dir(object())
        self.assertIn("__class__", o)
        self.assertIn("__repr__", o)
        self.assertIn("__doc__", o)

    def test_dir_with_builtin_returns_attributes(self):
        s = dir("foo")
        self.assertIn("__class__", s)
        self.assertIn("__repr__", s)
        self.assertIn("__doc__", s)
        self.assertIn("lower", s)
        self.assertIn("strip", s)

    def test_dir_with_builtin_subclass_returns_attributes(self):
        class Foo(str):
            def foo(self):
                pass

        s = dir(Foo("bar"))
        self.assertIn("__class__", s)
        self.assertIn("__repr__", s)
        self.assertIn("__doc__", s)
        self.assertIn("lower", s)
        self.assertIn("strip", s)
        self.assertIn("foo", s)

    def test_dir_with_custom_object_returns_attribtues(self):
        class Foo:
            def foo(self):
                pass

        f = dir(Foo())
        self.assertIn("__class__", f)
        self.assertIn("__repr__", f)
        self.assertIn("__doc__", f)
        self.assertIn("foo", f)

    def test_dir_with_inherited_object_returns_base_attribtues(self):
        class Foo:
            def foo(self):
                pass

        class Bar(Foo):
            def bar(self):
                pass

        b = dir(Bar())
        self.assertIn("__class__", b)
        self.assertIn("__repr__", b)
        self.assertIn("__doc__", b)
        self.assertIn("foo", b)
        self.assertIn("bar", b)

    def test_dir_with_custom_dunder_dict_returns_attribtues(self):
        class Foo:
            __slots__ = ["__dict__"]
            __dict__ = "hey"

            def foo(self):
                pass

        f = dir(Foo())
        self.assertIn("__class__", f)
        self.assertIn("__repr__", f)
        self.assertIn("__doc__", f)
        self.assertIn("foo", f)

    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info < (3, 7),
        "requires at least CPython 3.7",
    )
    def test_dunder_class_getitem_becomes_classmethod(self):
        class C:
            def __class_getitem__(cls, item):
                return None

            @classmethod
            def foo(cls):
                return cls

        self.assertIs(type(C.__class_getitem__), type(C.foo))

    def test_dunder_getattribute_with_raising_descr_propagates_exception(self):
        class D:
            def __get__(self, x, y):
                raise UserWarning("foo")

        class C:
            __getattribute__ = D()

        instance = C()
        with self.assertRaises(UserWarning):
            instance.foo

    def test_dunder_new_with_builtin_type_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, r"object\.__new__\(int\) is not safe.*int\.__new__\(\)"
        ):
            object.__new__(int)

    def test_dunder_subclasshook_returns_not_implemented(self):
        self.assertIs(object.__subclasshook__(), NotImplemented)
        self.assertIs(object.__subclasshook__(int), NotImplemented)

    def test_dunder_class_on_instance_returns_type(self):
        class Foo:
            pass

        class Bar(Foo):
            pass

        class Hello(Bar, list):
            pass

        self.assertIs([].__class__, list)
        self.assertIs(Foo().__class__, Foo)
        self.assertIs(Bar().__class__, Bar)
        self.assertIs(Hello().__class__, Hello)
        self.assertIs(Foo.__class__, type)
        self.assertIs(super(Bar, Bar()).__class__, super)

    def test_dunder_init_subclass_with_args_raises_type_error(self):
        with self.assertRaises(TypeError):
            object.__init_subclass__(str)

    def test_dunder_init_subclass_with_kwargs_raises_type_error(self):
        with self.assertRaises(TypeError):
            object.__init_subclass__(name="foo")

    def test_dunder_init_subclass_returns_none(self):
        self.assertIs(object.__init_subclass__(), None)

    def test_dunder_setattr_raises_attribute_error(self):
        result = object()
        with self.assertRaisesRegex(AttributeError, ".*attribute 'foo'"):
            result.foo = "bar"

    def test_dunder_setattr_on_exception_does_not_raise(self):
        result = Exception()
        result.foo = "bar"
        self.assertEqual(result.foo, "bar")

    def test_dunder_dict_dunder_getitem_with_non_existent_key_raises_key_error(self):
        class C:
            pass

        instance = C()
        with self.assertRaises(KeyError):
            instance.__dict__["non_key"]

    def test_subclass_does_not_override_user_dunder_dict(self):
        class C(object):
            __dict__ = 5

        class D(C):
            pass

        self.assertEqual(C.__dict__["__dict__"], 5)
        self.assertNotIn("__dict__", D.__dict__)

    def test_dunder_dict_dunder_setitem_sets_attribute(self):
        class C:
            pass

        instance = C()
        with self.assertRaises(AttributeError):
            instance.foo
        instance.__dict__["foo"] = "bar"
        self.assertEqual(instance.foo, "bar")

    def test_dunder_dict_items_returns_items_iterable(self):
        class C:
            pass

        instance = C()
        d = instance.__dict__
        d["foo"] = "bar"
        self.assertEqual(list(d.items()), [("foo", "bar")])

    def test_dunder_dict_keys_returns_keys_iterable(self):
        class C:
            pass

        instance = C()
        d = instance.__dict__
        d["foo"] = "bar"
        d["baz"] = "quux"
        keys = d.keys()
        self.assertEqual(len(keys), 2)
        self.assertIn("foo", keys)
        self.assertIn("baz", keys)

    def test_dunder_dict_update_sets_attributes(self):
        class C:
            pass

        instance = C()
        d = instance.__dict__
        self.assertEqual(len(d), 0)
        d.update({"hello": "world", "foo": "bar"})
        self.assertEqual(len(d), 2)
        self.assertEqual(d["hello"], "world")
        self.assertEqual(d["foo"], "bar")

    def test_int_subclass_has_dunder_dict(self):
        class C(int):
            pass

        sub = C(5)
        self.assertTrue(hasattr(sub, "__dict__"))

    def test_str_subclass_has_dunder_dict(self):
        class C(str):
            pass

        sub = C("foo")
        self.assertTrue(hasattr(sub, "__dict__"))

    def test_instance_dict_stays_synced_with_attribute_values(self):
        class C:
            pass

        instance = C()
        d = instance.__dict__
        self.assertEqual(len(d), 0)
        instance.foo = 42
        self.assertEqual(len(d), 1)
        self.assertEqual(d["foo"], 42)
        d["bar"] = 7
        self.assertEqual(instance.bar, 7)

    def test_dunder_dict_dunder_contains_returns_true_if_attr_exists(self):
        class C:
            pass

        instance = C()
        d = instance.__dict__
        self.assertFalse(d.__contains__("foo"))
        instance.foo = "bar"
        self.assertTrue(d.__contains__("foo"))
        del instance.foo
        self.assertFalse(d.__contains__("foo"))

    def test_dunder_dict_dunder_delitem_deletes_attribute(self):
        class C:
            pass

        instance = C()
        d = instance.__dict__
        self.assertNotIn("foo", d)
        with self.assertRaisesRegex(AttributeError, "foo"):
            instance.foo
        instance.foo = "bar"
        self.assertIn("foo", d)
        d.__delitem__("foo")
        self.assertNotIn("foo", d)
        with self.assertRaisesRegex(AttributeError, "foo"):
            instance.foo

    def test_dunder_dict_dunder_iter_iterates_over_keys(self):
        class C:
            pass

        instance = C()
        d = instance.__dict__
        instance.foo = "bar"
        instance.bar = "baz"
        keys = (*d.__iter__(),)
        self.assertEqual(len(keys), 2)
        self.assertIn("foo", keys)
        self.assertIn("bar", keys)

    def test_dunder_dict_pop_removes_and_returns_attribute(self):
        class C:
            pass

        instance = C()
        d = instance.__dict__
        instance.foo = "bar"
        self.assertEqual(d.pop("foo"), "bar")
        self.assertNotIn("foo", d)
        with self.assertRaisesRegex(AttributeError, "foo"):
            instance.foo

    def test_dunder_dict_pop_with_nonexistent_attr_returns_default(self):
        class C:
            pass

        instance = C()
        d = instance.__dict__
        value = "bar"
        self.assertIs(d.pop("foo", value), value)
        self.assertNotIn("foo", d)
        with self.assertRaisesRegex(AttributeError, "foo"):
            instance.foo

    def test_dunder_dict_pop_with_nonexistent_attr_raises_key_error(self):
        class C:
            pass

        instance = C()
        d = instance.__dict__
        self.assertRaisesRegex(KeyError, "foo", d.pop, "foo")
        self.assertNotIn("foo", d)
        with self.assertRaisesRegex(AttributeError, "foo"):
            instance.foo

    def test_dunder_dict_popitem_with_empty_dict_raises_key_error(self):
        class C:
            pass

        instance = C()
        self.assertRaisesRegex(KeyError, "empty", instance.__dict__.popitem)

    def test_dunder_dict_popitem_removes_attribute(self):
        class C:
            pass

        instance = C()
        orig_value = "bar"
        instance.foo = orig_value
        d = instance.__dict__
        (key, value) = d.popitem()
        self.assertEqual(key, "foo")
        self.assertIs(value, orig_value)
        self.assertNotIn("foo", d)

    def test_dunder_dict_removes_only_one_attribute(self):
        class C:
            pass

        instance = C()
        instance.foo = "bar"
        instance.bar = "baz"
        d = instance.__dict__
        self.assertEqual(len(d), 2)
        (key, value) = d.popitem()
        self.assertEqual(len(d), 1)
        self.assertNotIn(key, d)
        self.assertIn((key, value), (("foo", "bar"), ("bar", "baz")))

    def test_dunder_dict_clear_removes_attributes(self):
        class C:
            pass

        instance = C()
        d = instance.__dict__
        self.assertEqual(len(d), 0)
        instance.foo = "bar"
        self.assertIn("foo", d)
        instance.bar = "foo"
        self.assertIn("bar", d)
        self.assertEqual(len(d), 2)
        d.clear()
        self.assertEqual(len(d), 0)
        self.assertNotIn("foo", d)
        with self.assertRaisesRegex(AttributeError, "foo"):
            instance.foo
        self.assertNotIn("bar", d)
        with self.assertRaisesRegex(AttributeError, "bar"):
            instance.bar

    def test_dunder_dict_setdefault_with_nonexistent_attr_sets_none(self):
        class C:
            pass

        instance = C()
        d = instance.__dict__
        self.assertIsNone(d.setdefault("foo"))
        self.assertIsNone(d["foo"])
        self.assertIsNone(instance.foo)

    def test_dunder_dict_setdefault_with_nonexistent_attr_sets_default(self):
        class C:
            pass

        instance = C()
        d = instance.__dict__
        value = "bar"
        self.assertIs(d.setdefault("foo", value), value)
        self.assertIs(d["foo"], value)
        self.assertIs(instance.foo, value)

    def test_dunder_dict_setdefault_with_extant_attr_returns_value(self):
        class C:
            pass

        instance = C()
        value = "bar"
        instance.foo = value
        d = instance.__dict__
        self.assertIs(d.setdefault("foo"), value)
        self.assertIs(d["foo"], value)
        self.assertIs(instance.foo, value)

    def test_dunder_str_with_none_repr_raises_type_error(self):
        class C:
            __repr__ = None

        c = C()
        with self.assertRaises(TypeError) as context:
            str(c)
        self.assertEqual(str(context.exception), "'NoneType' object is not callable")

    def test_ge_returns_not_implemented(self):
        self.assertIs(object().__ge__(object()), NotImplemented)

    def test_gt_returns_not_implemented(self):
        self.assertIs(object().__gt__(object()), NotImplemented)

    def test_le_returns_not_implemented(self):
        self.assertIs(object().__le__(object()), NotImplemented)

    def test_lt_returns_not_implemented(self):
        self.assertIs(object().__lt__(object()), NotImplemented)


class OctTests(unittest.TestCase):
    def test_returns_string(self):
        self.assertEqual(oct(0), "0o0")
        self.assertEqual(oct(-1), "-0o1")
        self.assertEqual(oct(1), "0o1")
        self.assertEqual(oct(54321), "0o152061")
        self.assertEqual(oct(34466324363639), "0o765432101234567")

    def test_with_large_int_returns_string(self):
        # Test carry-over behavior at the border between digit 0 and 1:
        self.assertEqual(oct(1 << 60), "0o100000000000000000000")
        self.assertEqual(oct(1 << 61), "0o200000000000000000000")
        self.assertEqual(oct(1 << 62), "0o400000000000000000000")
        self.assertEqual(oct(1 << 63), "0o1000000000000000000000")
        self.assertEqual(oct(1 << 64), "0o2000000000000000000000")
        self.assertEqual(oct(1 << 65), "0o4000000000000000000000")
        # Test carry over behavior between later digits (there's 3 different
        # carry sizes, between 0-1, 1-2, 2-3).
        self.assertEqual(
            oct(0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF),
            "0o77777777777777777777777777777777777777777777777777777777777777"
            "7777777777",
        )
        self.assertEqual(
            oct(0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF),
            "0o17777777777777777777777777777777777777777777777777777777777777"
            "777777777777",
        )
        self.assertEqual(
            oct(0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF),
            "0o37777777777777777777777777777777777777777777777777777777777777"
            "7777777777777",
        )
        # Some random numbers:
        self.assertEqual(
            oct(0xDEE182DE2EC55F61B22A509ED1DC3EB),
            "0o157341405570566125754154425120475507341753",
        )
        self.assertEqual(
            oct(-0x53ADC651E593B1323158BFA776E8173F60C76519277B2BD6),
            "-0o2472670624362623542310612613764735564027176603073121444736625726",
        )

    def test_calls_dunder_index(self):
        class C:
            def __int__(self):
                return 42

            def __index__(self):
                return -9

        self.assertEqual(oct(C()), "-0o11")

    def test_with_int_subclass(self):
        class C(int):
            pass

        self.assertEqual(oct(C(51)), "0o63")

    def test_with_non_int_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            oct("not an int")
        self.assertEqual(
            str(context.exception), "'str' object cannot be interpreted as an integer"
        )


class OpenTests(unittest.TestCase):
    def test_function_exists(self):
        import builtins

        self.assertTrue(hasattr(builtins, "open"))


class PowTests(unittest.TestCase):
    def test_binary_first_arg_pow_returns_result(self):
        dunder_pow_args = None

        class A:
            def __pow__(self, other, mod=None):
                nonlocal dunder_pow_args
                dunder_pow_args = (self, other, mod)
                return -123

        a = A()
        self.assertEqual(pow(a, "str0"), -123)
        self.assertEqual(dunder_pow_args, (a, "str0", None))

    def test_binary_second_arg_rpow_returns_result(self):
        dunder_pow_args = None

        class A:
            def __rpow__(self, other, mod=None):
                nonlocal dunder_pow_args
                dunder_pow_args = (self, other, mod)
                return -123

        a = A()
        self.assertEqual(pow("str0", a), -123)
        self.assertEqual(dunder_pow_args, (a, "str0", None))

    def test_binary_unsupported_dunder_functions_raises_type_error(self):
        # TODO(T53066604): Check error message.
        with self.assertRaises(TypeError):
            pow("", "")

    def test_ternary_first_arg_pow_returns_result(self):
        dunder_pow_args = None

        class A:
            def __pow__(self, other, mod=None):
                nonlocal dunder_pow_args
                dunder_pow_args = (self, other, mod)
                return -123

        a = A()
        self.assertEqual(pow(a, "str0", "str1"), -123)
        self.assertEqual(dunder_pow_args, (a, "str0", "str1"))

    def test_ternary_first_arg_pow_from_descriptor_returns_result(self):
        class Desc:
            def __get__(self, obj, type):
                return lambda a, b, c=None: -5678

        class A:
            __pow__ = Desc()

        a = A()
        self.assertEqual(pow(a, "str0", "str1"), -5678)

    def test_ternary_ignore_non_first_args_dunder_functions(self):
        class A:
            def __pow__(self, other, mod=None):
                raise UserWarning("unreachable")

            def __rpow__(self, other):
                # __rpow__ doesn't accept the third arg.
                raise UserWarning("unreachable")

        with self.assertRaises(TypeError) as context:
            pow("", A(), A())
        self.assertIn(
            "unsupported operand type(s) for pow(): 'str', 'A', 'A'",
            str(context.exception),
        )

    def test_ternary_unsupported_dunder_functions_raises_type_error(self):
        class A:
            pass

        class B:
            pass

        class C:
            pass

        with self.assertRaises(TypeError) as context:
            pow(A(), B(), C())
        self.assertIn(
            "unsupported operand type(s) for pow(): 'A', 'B', 'C'",
            str(context.exception),
        )


class PrintTests(unittest.TestCase):
    class MyStream:
        def __init__(self):
            self.buf = ""

        def write(self, text):
            self.buf += text
            return len(text)

        def flush(self):
            raise UserWarning("foo")

    def test_print_writes_to_stream(self):
        stream = PrintTests.MyStream()
        print("hello", file=stream)
        self.assertEqual(stream.buf, "hello\n")

    def test_print_returns_none(self):
        stream = PrintTests.MyStream()
        self.assertIs(print("hello", file=stream), None)

    def test_print_writes_end(self):
        stream = PrintTests.MyStream()
        print("hi", end="ho", file=stream)
        self.assertEqual(stream.buf, "hiho")

    def test_print_with_no_sep_defaults_to_space(self):
        stream = PrintTests.MyStream()
        print("hello", "world", file=stream)
        self.assertEqual(stream.buf, "hello world\n")

    def test_print_writes_none(self):
        stream = PrintTests.MyStream()
        print(None, file=stream)
        self.assertEqual(stream.buf, "None\n")

    def test_print_with_none_file_prints_to_sys_stdout(self):
        stream = PrintTests.MyStream()
        import sys

        orig_stdout = sys.stdout
        sys.stdout = stream
        print("hello", file=None)
        self.assertEqual(stream.buf, "hello\n")
        sys.stdout = orig_stdout

    def test_print_with_none_stdout_does_nothing(self):
        import sys

        orig_stdout = sys.stdout
        sys.stdout = None
        print("hello", file=None)
        sys.stdout = orig_stdout

    def test_print_with_deleted_stdout_raises_runtime_error(self):
        import sys

        orig_stdout = sys.stdout
        del sys.stdout
        with self.assertRaises(RuntimeError):
            print("hello", file=None)
        sys.stdout = orig_stdout

    def test_print_with_flush_calls_file_flush(self):
        stream = PrintTests.MyStream()
        with self.assertRaises(UserWarning):
            print("hello", file=stream, flush=True)
        self.assertEqual(stream.buf, "hello\n")

    def test_print_calls_dunder_str(self):
        class C:
            def __str__(self):
                raise UserWarning("foo")

        stream = PrintTests.MyStream()
        c = C()
        with self.assertRaises(UserWarning):
            print(c, file=stream)


class PropertyTests(unittest.TestCase):
    def test_dunder_abstractmethod_with_missing_attr_returns_false(self):
        def foo():
            pass

        prop = property(foo)
        self.assertIs(prop.__isabstractmethod__, False)

    def test_dunder_abstractmethod_with_false_attr_returns_false(self):
        def foo():
            pass

        foo.__isabstractmethod__ = False
        with self.assertRaises(AttributeError):
            type(foo).__isabstractmethod__
        prop = property(foo)
        self.assertIs(prop.__isabstractmethod__, False)

    def test_dunder_abstractmethod_with_abstract_getter_returns_true(self):
        def foo():
            pass

        foo.__isabstractmethod__ = b"random non-empty value"
        with self.assertRaises(AttributeError):
            type(foo).__isabstractmethod__
        prop = property(foo)
        self.assertIs(prop.__isabstractmethod__, True)

    def test_dunder_abstractmethod_with_abstract_setter_returns_true(self):
        def foo():
            pass

        foo.__isabstractmethod__ = True
        with self.assertRaises(AttributeError):
            type(foo).__isabstractmethod__
        prop = property(fset=foo)
        self.assertIs(prop.__isabstractmethod__, True)

    def test_dunder_abstractmethod_with_abstract_deleter_returns_true(self):
        def foo():
            pass

        foo.__isabstractmethod__ = (42, "non-empty tuple")
        with self.assertRaises(AttributeError):
            type(foo).__isabstractmethod__
        prop = property(fdel=foo)
        self.assertIs(prop.__isabstractmethod__, True)

    def test_dunder_abstractmethod_with_non_abstractmethod_raise_type_error(self):
        with self.assertRaises(TypeError):
            property.__dict__["__isabstractmethod__"].__get__(42)

    def test_dunder_doc_returns_updated_value(self):
        p = property()
        self.assertIsNone(p.__doc__)

        document_message = "this is for testing"
        p.__doc__ = document_message
        self.assertIs(p.__doc__, document_message)

    def test_dunder_get_returns_value(self):
        class C:
            @property
            def bar(self):
                return 5

        self.assertEqual(C().bar, 5)

    def test_dunder_get_with_subclassed_property_returns_value(self):
        class foo(property):
            pass

        class C:
            @foo
            def bar(self):
                return 5

        self.assertEqual(C().bar, 5)

    def test_dunder_get_called_with_non_property_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            def bar():
                pass

            property.__get__(42.3, None, bar)
        self.assertIn(
            "'__get__' requires a 'property' object but received a 'float'",
            str(context.exception),
        )

    def test_dunder_new_called_with_non_type_object_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            def bar():
                pass

            property.__new__(42.3, bar)
        self.assertIn("not a type object", str(context.exception))

    def test_dunder_new_called_with_non_subtype_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            def bar():
                pass

            property.__new__(float, bar)
        self.assertIn("not a subtype of property", str(context.exception))

    def test_dunder_new_with_subclassed_property_returns_instance_of_superclass(self):
        class foo(property):
            pass

        def bar():
            pass

        self.assertIsInstance(property.__new__(foo, bar), foo)

    def test_getter_and_setter_and_deleter_default_to_none(self):
        p = property()
        self.assertIsNone(p.fget)
        self.assertIsNone(p.fset)
        self.assertIsNone(p.fdel)

    def test_fget_returns_getter(self):
        p = property(fget=len)
        self.assertIs(p.fget, len)
        with self.assertRaises(AttributeError):
            p.fget = len

    def test_fset_returns_setter(self):
        p = property(fset=len)
        self.assertIs(p.fset, len)
        with self.assertRaises(AttributeError):
            p.fset = len

    def test_fdel_returns_deleter(self):
        p = property(fdel=len)
        self.assertIs(p.fdel, len)
        with self.assertRaises(AttributeError):
            p.fdel = len


class RangeTests(unittest.TestCase):
    def test_dunder_bool_with_empty_range_returns_true(self):
        self.assertFalse(range(0).__bool__())

    def test_dunder_bool_with_non_empty_range_returns_true(self):
        self.assertTrue(range(1).__bool__())

    def test_dunder_contains_with_int_less_than_start_returns_false(self):
        self.assertFalse(range(1, 5).__contains__(0))

    def test_dunder_contains_with_int_equals_start_returns_true(self):
        self.assertTrue(range(1, 5).__contains__(1))

    def test_dunder_contains_with_int_in_range_returns_true(self):
        self.assertTrue(range(1, 5).__contains__(3))

    def test_dunder_contains_with_int_equals_stop_returns_false(self):
        self.assertFalse(range(1, 5).__contains__(5))

    def test_dunder_contains_with_int_in_stride_returns_true(self):
        self.assertTrue(range(1, 5, 2).__contains__(3))

    def test_dunder_contains_with_int_not_in_stride_returns_false(self):
        self.assertFalse(range(1, 5, 2).__contains__(4))

    def test_dunder_contains_with_negative_step_and_int_greater_than_start_returns_false(  # noqa: B950
        self,
    ):
        self.assertFalse(range(5, 1, -1).__contains__(6))

    def test_dunder_contains_with_negative_step_and_int_equals_start_returns_true(self):
        self.assertTrue(range(5, 1, -1).__contains__(5))

    def test_dunder_contains_with_negative_step_and_int_less_than_start_returns_true(  # noqa: B950
        self,
    ):
        self.assertTrue(range(5, 1, -1).__contains__(4))

    def test_dunder_contains_with_negative_step_and_int_equals_stop_returns_false(self):
        self.assertFalse(range(5, 1, -1).__contains__(1))

    def test_dunder_contains_with_negative_step_and_int_greater_than_stop_returns_true(  # noqa: B950
        self,
    ):
        self.assertTrue(range(5, 1, -1).__contains__(2))

    def test_dunder_contains_with_non_int_falls_back_to_iter_search(self):
        class C:
            __eq__ = Mock(name="__eq__", return_value=False)

        range(1, 10, 3).__contains__(C())
        self.assertEqual(C.__eq__.call_count, 3)

    def test_dunder_eq_with_non_range_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            range.__eq__(1, 2)

    def test_dunder_eq_with_non_range_other_returns_not_implemented(self):
        r = range(100)
        self.assertIs(r.__eq__(1), NotImplemented)

    def test_dunder_eq_same_returns_true(self):
        r = range(10)
        self.assertTrue(r == r)

    def test_dunder_eq_both_empty_returns_true(self):
        r1 = range(0)
        r2 = range(4, 3, 2)
        r3 = range(2, 5, -1)
        self.assertTrue(r1 == r2)
        self.assertTrue(r1 == r3)
        self.assertTrue(r2 == r3)

    def test_dunder_eq_different_start_returns_false(self):
        r1 = range(1, 10, 3)
        r2 = range(2, 10, 3)
        self.assertFalse(r1 == r2)

    def test_dunder_eq_different_stop_returns_true(self):
        r1 = range(0, 10, 3)
        r2 = range(0, 11, 3)
        self.assertTrue(r1 == r2)

    def test_dunder_eq_different_step_length_one_returns_true(self):
        r1 = range(0, 4, 10)
        r2 = range(0, 4, 11)
        self.assertTrue(r1 == r2)

    def test_dunder_eq_different_step_returns_false(self):
        r1 = range(0, 14, 10)
        r2 = range(0, 14, 11)
        self.assertFalse(r1 == r2)

    def test_dunder_ge_with_non_range_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            range.__ge__(1, 2)

    def test_dunder_ge_returns_not_implemented(self):
        r = range(100)
        self.assertIs(r.__ge__(r), NotImplemented)

    def test_dunder_getitem_with_non_range_raises_type_error(self):
        with self.assertRaises(TypeError):
            range.__getitem__(1, 2)

    def test_dunder_getitem_with_int_subclass_does_not_call_dunder_index(self):
        class C(int):
            def __index__(self):
                raise ValueError("foo")

        r = range(5)
        self.assertEqual(r[C(3)], 3)

    def test_dunder_getitem_with_raising_descriptor_propagates_exception(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("foo")

        class C:
            __index__ = Desc()

        r = range(5)
        with self.assertRaises(AttributeError) as context:
            r[C()]
        self.assertEqual(str(context.exception), "foo")

    def test_dunder_getitem_with_string_raises_type_error(self):
        r = range(5)
        with self.assertRaises(TypeError) as context:
            r["3"]
        self.assertEqual(
            str(context.exception), "range indices must be integers or slices, not str"
        )

    def test_dunder_getitem_with_dunder_index_calls_dunder_index(self):
        class C:
            def __index__(self):
                return 2

        r = range(5)
        self.assertEqual(r[C()], 2)

    def test_dunder_getitem_index_too_small_raises_index_error(self):
        r = range(5)
        with self.assertRaises(IndexError) as context:
            r[-6]
        self.assertEqual(str(context.exception), "range object index out of range")

    def test_dunder_getitem_index_too_large_raises_index_error(self):
        r = range(5)
        with self.assertRaises(IndexError) as context:
            r[5]
        self.assertEqual(str(context.exception), "range object index out of range")

    def test_dunder_getitem_negative_index_relative_to_end_value_error(self):
        r = range(5)
        self.assertEqual(r[-4], 1)

    def test_dunder_getitem_with_valid_indices_returns_sublist(self):
        r = range(5)
        self.assertEqual(r[2:-1:1], range(2, 4))

    def test_dunder_getitem_with_negative_start_returns_trailing(self):
        r = range(5)
        self.assertEqual(r[-2:], range(3, 5))

    def test_dunder_getitem_with_positive_stop_returns_leading(self):
        r = range(5)
        self.assertEqual(r[:2], range(2))

    def test_dunder_getitem_with_negative_stop_returns_all_but_trailing(self):
        r = range(5)
        self.assertEqual(r[:-2], range(3))

    def test_dunder_getitem_with_positive_step_returns_forwards_list(self):
        r = range(5)
        self.assertEqual(r[::2], range(0, 5, 2))

    def test_dunder_getitem_with_negative_step_returns_backwards_list(self):
        r = range(5)
        self.assertEqual(r[::-2], range(4, -1, -2))

    def test_dunder_getitem_with_large_negative_start_returns_copy(self):
        r = range(5)
        copy = r[-10:]
        self.assertEqual(copy, r)
        self.assertIsNot(copy, r)

    def test_dunder_getitem_with_large_positive_start_returns_empty(self):
        r = range(5)
        self.assertEqual(r[10:], range(0))

    def test_dunder_getitem_with_large_negative_start_returns_empty(self):
        r = range(5)
        self.assertEqual(r[:-10], range(0))

    def test_dunder_getitem_with_large_positive_start_returns_copy(self):
        r = range(5)
        copy = r[:10]
        self.assertEqual(copy, r)
        self.assertIsNot(copy, r)

    def test_dunder_getitem_with_identity_slice_returns_copy(self):
        r = range(5)
        copy = r[::]
        self.assertEqual(copy, r)
        self.assertIsNot(copy, r)

    def test_dunder_gt_with_non_range_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            range.__gt__(1, 2)

    def test_dunder_gt_returns_not_implemented(self):
        r = range(100)
        self.assertIs(r.__gt__(r), NotImplemented)

    def test_dunder_iter_returns_range_iterator(self):
        it = iter(range(100))
        self.assertEqual(type(it).__name__, "range_iterator")

    def test_dunder_iter_returns_longrange_iterator(self):
        it = iter(range(2 ** 63))
        self.assertEqual(type(it).__name__, "longrange_iterator")

    def test_dunder_le_with_non_range_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            range.__le__(1, 2)

    def test_dunder_le_returns_not_implemented(self):
        r = range(100)
        self.assertIs(r.__le__(r), NotImplemented)

    def test_dunder_lt_with_non_range_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            range.__lt__(1, 2)

    def test_dunder_lt_returns_not_implemented(self):
        r = range(100)
        self.assertIs(r.__lt__(r), NotImplemented)

    def test_dunder_ne_with_non_range_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            range.__ne__(1, 2)

    def test_dunder_ne_with_non_range_other_returns_not_implemented(self):
        r = range(100)
        self.assertIs(r.__ne__(1), NotImplemented)

    def test_dunder_ne_same_returns_false(self):
        r = range(10)
        self.assertFalse(r != r)

    def test_dunder_ne_both_empty_returns_false(self):
        r1 = range(0)
        r2 = range(4, 3, 2)
        r3 = range(2, 5, -1)
        self.assertFalse(r1 != r2)
        self.assertFalse(r1 != r3)
        self.assertFalse(r2 != r3)

    def test_dunder_ne_different_start_returns_true(self):
        r1 = range(1, 10, 3)
        r2 = range(2, 10, 3)
        self.assertTrue(r1 != r2)

    def test_dunder_ne_different_stop_returns_false(self):
        r1 = range(0, 10, 3)
        r2 = range(0, 11, 3)
        self.assertFalse(r1 != r2)

    def test_dunder_ne_different_step_length_one_returns_false(self):
        r1 = range(0, 4, 10)
        r2 = range(0, 4, 11)
        self.assertFalse(r1 != r2)

    def test_dunder_ne_different_step_returns_true(self):
        r1 = range(0, 14, 10)
        r2 = range(0, 14, 11)
        self.assertTrue(r1 != r2)

    def test_dunder_new_with_non_type_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            range.__new__(2, 1)
        self.assertEqual(
            str(context.exception), "range.__new__(X): X is not a type object (int)"
        )

    def test_dunder_new_with_str_type_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            range.__new__(str, 5)
        self.assertEqual(
            str(context.exception), "range.__new__(str): str is not a subtype of range"
        )

    def test_dunder_new_with_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            range("2")
        self.assertEqual(
            str(context.exception), "'str' object cannot be interpreted as an integer"
        )

    def test_dunder_new_with_zero_step_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            range(1, 2, 0)
        self.assertEqual(str(context.exception), "range() arg 3 must not be zero")

    def test_dunder_new_calls_dunder_index(self):
        class Foo:
            def __index__(self):
                return 10

        obj = range(10)
        self.assertEqual(obj.start, 0)
        self.assertEqual(obj.stop, 10)
        self.assertEqual(obj.step, 1)

    def test_dunder_new_stores_int_subclasses(self):
        class Foo(int):
            pass

        class Bar:
            def __index__(self):
                return Foo(10)

        warnings.filterwarnings(
            action="ignore",
            category=DeprecationWarning,
            message="__index__ returned non-int.*",
            module=__name__,
        )
        obj = range(Foo(2), Bar())
        self.assertEqual(type(obj.start), Foo)
        self.assertEqual(type(obj.stop), Foo)
        self.assertEqual(obj.start, 2)
        self.assertEqual(obj.stop, 10)
        self.assertEqual(obj.step, 1)

    def test_dunder_new_with_one_arg_sets_stop(self):
        obj = range(10)
        self.assertEqual(obj.start, 0)
        self.assertEqual(obj.stop, 10)
        self.assertEqual(obj.step, 1)

    def test_dunder_new_with_two_args_sets_start_and_stop(self):
        obj = range(10, 11)
        self.assertEqual(obj.start, 10)
        self.assertEqual(obj.stop, 11)
        self.assertEqual(obj.step, 1)

    def test_dunder_new_with_three_args_sets_all(self):
        obj = range(10, 11, 12)
        self.assertEqual(obj.start, 10)
        self.assertEqual(obj.stop, 11)
        self.assertEqual(obj.step, 12)

    def test_dunder_reduce_returns_tuple(self):
        r = range(10)
        self.assertTupleEqual(r.__reduce__(), (range, (0, 10, 1)))
        r = range(21, 10, 2)
        self.assertTupleEqual(r.__reduce__(), (range, (21, 10, 2)))

    def test_dunder_repr_with_no_step_calls_repr(self):
        class Repr(int):
            def __repr__(self):
                return "repr"

            def __str__(self):
                return "1"

        r = range(Repr(2), Repr(4))
        self.assertEqual(repr(r), "range(repr, repr)")

    def test_dunder_repr_with_step_calls_repr(self):
        class Repr(int):
            def __repr__(self):
                return "repr"

            def __str__(self):
                return "1"

        r = range(Repr(2), Repr(4), Repr(2))
        self.assertEqual(repr(r), "range(repr, repr, repr)")

    def test_dunder_reversed_with_returns_reversed_iterator(self):
        rev_iter = range(1, 6, 2).__reversed__()
        self.assertEqual(rev_iter.__next__(), 5)
        self.assertEqual(rev_iter.__next__(), 3)
        self.assertEqual(rev_iter.__next__(), 1)

    def test_start_is_readonly(self):
        with self.assertRaises(AttributeError):
            range(0, 1, 2).start = 2

    def test_step_is_readonly(self):
        with self.assertRaises(AttributeError):
            range(0, 1, 2).step = 2

    def test_stop_is_readonly(self):
        with self.assertRaises(AttributeError):
            range(0, 1, 2).stop = 2

    def test_count_with_int_in_range_returns_one(self):
        self.assertEqual(range(3).count(1), 1)

    def test_count_with_int_not_in_range_returns_zero(self):
        self.assertEqual(range(3).count(4), 0)

    def test_count_with_overloaded_eq_returns_all_counts(self):
        class A:
            def __eq__(self, other):
                return other == 1 or other == 2

        self.assertEqual(range(3).count(A()), 2)

    def test_index_with_int_in_range_returns_index(self):
        self.assertEqual(range(3).index(1), 1)

    def test_index_with_int_not_in_range_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            range(3).index(4)
        self.assertEqual(str(context.exception), "4 is not in range")

    def test_index_with_overloaded_eq_returns_all_indexs(self):
        class A:
            def __eq__(self, other):
                return other == 1 or other == 2

        self.assertEqual(range(3).index(A()), 1)

    def test_hash_equals_equivalent_tuple_hashes(self):
        self.assertEqual(hash(range(0)), hash((0, None, None)))
        self.assertEqual(hash(range(1)), hash((1, 0, None)))
        self.assertEqual(hash(range(1, 4)), hash((3, 1, 1)))
        self.assertEqual(hash(range(0, 4, 2)), hash((2, 0, 2)))


class RangeIteratorTests(unittest.TestCase):
    def test_dunder_iter_returns_self(self):
        it = iter(range(100))
        self.assertEqual(iter(it), it)

    def test_dunder_length_hint_returns_pending_length(self):
        len = 100
        it = iter(range(len))
        self.assertEqual(it.__length_hint__(), len)
        it.__next__()
        self.assertEqual(it.__length_hint__(), len - 1)

    def test_dunder_next_returns_ints(self):
        it = iter(range(10, 5, -2))
        self.assertEqual(it.__next__(), 10)
        self.assertEqual(it.__next__(), 8)
        self.assertEqual(it.__next__(), 6)
        with self.assertRaises(StopIteration):
            it.__next__()


class ReversedTests(unittest.TestCase):
    def test_dunder_new_with_no_dunder_getitem_raises_type_error(self):
        class C:
            pass

        with self.assertRaises(TypeError) as context:
            reversed(C())
        self.assertEqual(str(context.exception), "'C' object is not reversible")

    def test_reversed_iterates_backwards_over_iterable(self):
        it = reversed([1, 2, 3])
        self.assertEqual(it.__next__(), 3)
        self.assertEqual(it.__next__(), 2)
        self.assertEqual(it.__next__(), 1)
        with self.assertRaises(StopIteration):
            it.__next__()

    def test_reversed_calls_dunder_reverse(self):
        class C:
            def __reversed__(self):
                return "foo"

        self.assertEqual(reversed(C()), "foo")

    def test_reversed_with_none_dunder_reverse_raises_type_error(self):
        class C:
            __reversed__ = None

        with self.assertRaises(TypeError) as context:
            reversed(C())
        self.assertEqual(str(context.exception), "'C' object is not reversible")

    def test_reversed_with_non_callable_dunder_reverse_raises_type_error(self):
        class C:
            __reversed__ = 1

        with self.assertRaises(TypeError) as context:
            reversed(C())
        self.assertEqual(str(context.exception), "'int' object is not callable")

    def test_reversed_length_hint(self):
        it = reversed([1, 2, 3])
        self.assertEqual(it.__length_hint__(), 3)
        it.__next__()
        self.assertEqual(it.__length_hint__(), 2)
        it.__next__()
        self.assertEqual(it.__length_hint__(), 1)
        it.__next__()
        self.assertEqual(it.__length_hint__(), 0)


class RoundTests(unittest.TestCase):
    def test_round_calls_dunder_round(self):
        class Roundable:
            def __init__(self, value):
                self.value = value

            def __round__(self, ndigits="a default value"):
                return (self.value, ndigits)

        self.assertEqual(round(Roundable(12), 34), (12, 34))
        self.assertEqual(round(Roundable(56)), (56, "a default value"))

    def test_round_raises_type_error(self):
        class ClassWithoutDunderRound:
            pass

        with self.assertRaises(TypeError):
            round(ClassWithoutDunderRound())


class SeqTests(unittest.TestCase):
    def test_sequence_is_iterable(self):
        class A:
            def __getitem__(self, index):
                return [1, 2, 3][index]

        self.assertEqual([x for x in A()], [1, 2, 3])

    def test_sequence_iter_is_itself(self):
        class A:
            def __getitem__(self, index):
                return [1, 2, 3][index]

        a = iter(A())
        self.assertEqual(a, a.__iter__())

    def test_non_iter_non_sequence_with_iter_raises_type_error(self):
        class NonIter:
            pass

        with self.assertRaises(TypeError) as context:
            iter(NonIter())

        self.assertEqual(str(context.exception), "'NonIter' object is not iterable")

    def test_non_iter_non_sequence_with_for_raises_type_error(self):
        class NonIter:
            pass

        with self.assertRaises(TypeError) as context:
            [x for x in NonIter()]

        self.assertEqual(str(context.exception), "'NonIter' object is not iterable")

    def test_sequence_with_error_raising_iter_descriptor_raises_type_error(self):
        dunder_get_called = False

        class Desc:
            def __get__(self, obj, type):
                nonlocal dunder_get_called
                dunder_get_called = True
                raise UserWarning("Nope")

        class C:
            __iter__ = Desc()

        with self.assertRaises(TypeError) as context:
            [x for x in C()]

        self.assertEqual(str(context.exception), "'C' object is not iterable")
        self.assertTrue(dunder_get_called)

    def test_sequence_with_error_raising_getitem_descriptor_returns_iter(self):
        dunder_get_called = False

        class Desc:
            def __get__(self, obj, type):
                nonlocal dunder_get_called
                dunder_get_called = True
                raise UserWarning("Nope")

        class C:
            __getitem__ = Desc()

        i = iter(C())
        self.assertTrue(hasattr(i, "__next__"))
        self.assertFalse(dunder_get_called)


class SetTests(unittest.TestCase):
    def test_deepcopy_with_set_returns_set(self):
        s = {1, 2, 3}
        self.assertEqual(s, copy.deepcopy(s))

    def test_deepcopy_with_set_subclass_returns_subclass(self):
        class SubSet(set):
            pass

        s = SubSet({1, 2, 3})
        self.assertEqual(s, copy.deepcopy(s))

    def test_dunder_reduce_with_set_returns_tuple(self):
        s = set({1, 2, 3})
        result = s.__reduce__()
        cls, value, state = result
        self.assertEqual(type(result), tuple)
        self.assertIsInstance(cls, type)
        self.assertEqual(cls, set)
        self.assertEqual(type(value), tuple)
        self.assertEqual(len(value), 1)
        self.assertEqual(value[0], [1, 2, 3])
        self.assertEqual(state, None)

    def test_dunder_reduce_with_set_subclass_returns_tuple(self):
        class SubSet(set):
            pass

        s = SubSet({1, 2, 3})
        s.test_value = "test_value"
        result = s.__reduce__()
        cls, value, state = result
        self.assertEqual(type(result), tuple)
        self.assertIsInstance(cls, type)
        self.assertEqual(cls, SubSet)
        self.assertEqual(type(value), tuple)
        self.assertEqual(len(value), 1)
        self.assertEqual(type(value[0]), list)
        self.assertEqual(value[0], [1, 2, 3])
        self.assertEqual(len(state), 1)
        self.assertEqual(state["test_value"], "test_value")

    def test_set_add_then_remove_adds_then_add_adds_elements(self):
        s = set()
        for x in range(100):
            s.add(x)
        for x in range(100):
            s.remove(x)
        for x in range(100):
            s.add(x)
        self.assertEqual(s, set(range(100)))

    def test_set_difference_update_with_non_iterable_raises_type_error(self):
        a = {1, 2, 3}
        with self.assertRaises(TypeError):
            a.difference_update(1)

    def test_set_difference_update_no_match(self):
        a = {1, 2, 3}
        b = {4, 5, 6}
        a.difference_update(b)
        self.assertEqual(len(a), 3)
        self.assertIn(1, a)
        self.assertIn(2, a)
        self.assertIn(3, a)

    def test_set_difference_update_removes_elements(self):
        a = {1, 2, 3}
        b = {2, 3}
        a.difference_update(b)
        self.assertEqual(len(a), 1)
        self.assertIn(1, a)

    def test_set_difference_update_with_iterable_removes_elements(self):
        a = {1, 2, 3}
        b = [2]
        a.difference_update(b)
        self.assertEqual(len(a), 2)
        self.assertIn(1, a)
        self.assertIn(3, a)

    def test_set_difference_update_with_multiple_args_removes_elements(self):
        a = {1, 2, 3}
        b = [2]
        c = {1, 3}
        a.difference_update(b, c)
        self.assertEqual(len(a), 0)

    def test_dunder_and_with_non_set_raises_type_error(self):
        with self.assertRaises(TypeError):
            set.__and__(frozenset(), set())

    def test_dunder_ixor_with_non_set_self_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, "'__ixor__' requires a 'set' object but received a 'frozenset'"
        ):
            set.__ixor__(frozenset(), set())

    def test_dunder_ixor_with_non_anyset_other_returns_not_implemented(self):
        result = set.__ixor__(set(), ())
        self.assertIs(result, NotImplemented)

    def test_dunder_ixor_does_inplace_symmetric_difference_update(self):
        left = {1, 2, 3}
        right = {3, 4, 5}
        result = set.__ixor__(left, right)
        self.assertIs(result, left)
        self.assertSetEqual(left, {1, 2, 4, 5})
        self.assertSetEqual(right, {3, 4, 5})

    def test_dunder_ixor_with_frozenset_does_inplace_symmetric_difference_update(self):
        left = {1, 2, 3}
        right = frozenset({3, 4, 5})
        result = set.__ixor__(left, right)
        self.assertIs(result, left)
        self.assertSetEqual(left, {1, 2, 4, 5})
        self.assertEqual(right, {3, 4, 5})

    def test_set_subclass_difference_removes_elements(self):
        class SubSet(set):
            pass

        a = SubSet([1, 2, 3])
        b = {2, 3}
        a.difference_update(b)
        self.assertEqual(len(a), 1)
        self.assertIn(1, a)

    def test_set_subclass_difference_update_with_custom_discard(self):
        class SubSet(set):
            def discard(self, item):
                pass

        a = SubSet([1, 2, 3])
        b = {2, 3}
        a.difference_update(b)
        self.assertEqual(len(a), 1)
        self.assertIn(1, a)

    def test_set_subclass_difference_ignores_subclass_difference_update(self):
        class SubSet(set):
            def difference_update(self, *others):
                pass

        a = SubSet([1, 2, 3])
        b = {2, 3}
        set.difference_update(a, b)
        self.assertEqual(len(a), 1)
        self.assertIn(1, a)

    def test_set_clear_with_non_set_raises_type_error(self):
        with self.assertRaises(TypeError):
            set.clear(1)

    def test_set_clear_removes_elements(self):
        a = {1, 2, 3}
        a.clear()
        self.assertEqual(len(a), 0)

    def test_set_subclass_clear_removes_elements(self):
        class SubSet(set):
            pass

        a = SubSet([1, 2, 3])
        a.clear()
        self.assertEqual(len(a), 0)

    def test_dunder_or_with_non_set_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            set.__or__(frozenset(), set())
        self.assertIn(
            "'__or__' requires a 'set' object but received a 'frozenset'",
            str(context.exception),
        )

    def test_dunder_xor_with_non_set_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            set.__xor__(frozenset(), set())
        self.assertIn(
            "'__xor__' requires a 'set' object but received a 'frozenset'",
            str(context.exception),
        )

    def test_dunder_xor_with_non_anyset_other_returns_notimplemented(self):
        result = set.__xor__(set(), ())
        self.assertIs(result, NotImplemented)

    def test_dunder_xor_removes_common_elements(self):
        left = {1, 2, 3}
        right = {3, 4, 5}
        result = set.__xor__(left, right)
        self.assertIs(type(result), set)
        self.assertSetEqual(result, {1, 2, 4, 5})

    def test_dunder_xor_with_frozenset_removes_common_elements(self):
        left = {1, 2, 3}
        right = frozenset({3, 4, 5})
        result = set.__xor__(left, right)
        self.assertIs(type(result), set)
        self.assertSetEqual(result, {1, 2, 4, 5})

    def test_dunder_rxor_with_non_set_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, "'__rxor__' requires a 'set' object but received a 'frozenset'"
        ):
            set.__rxor__(frozenset(), set())

    def test_dunder_rxor_with_non_anyset_other_returns_notimplemented(self):
        result = set.__rxor__(set(), ())
        self.assertIs(result, NotImplemented)

    def test_dunder_rxor_removes_common_elements(self):
        left = {1, 2, 3}
        right = {3, 4, 5}
        result = set.__rxor__(left, right)
        self.assertSetEqual(result, {1, 2, 4, 5})

    def test_dunder_rxor_with_frozenset_removes_common_elements(self):
        left = {1, 2, 3}
        right = frozenset({3, 4, 5})
        result = set.__rxor__(left, right)
        self.assertSetEqual(result, {1, 2, 4, 5})

    def test_difference_no_others_copies_self(self):
        a_set = {1, 2, 3}
        self.assertIsNot(set.difference(a_set), a_set)

    def test_difference_same_sets_returns_empty_set(self):
        a_set = {1, 2, 3}
        self.assertFalse(set.difference(a_set, a_set))

    def test_difference_two_sets_returns_difference(self):
        set1 = {1, 2, 3, 4, 5, 6, 7}
        set2 = {1, 3, 5, 7}
        self.assertEqual(set.difference(set1, set2), {2, 4, 6})

    def test_difference_many_sets_returns_difference(self):
        a_set = {1, 10, 100, 1000}
        self.assertEqual(set.difference(a_set, {10}, {100}, {1000}), {1})

    def test_discard_with_non_set_raises_type_error(self):
        with self.assertRaises(TypeError):
            set.discard(None, 1)

    def test_discard_with_non_member_returns_none(self):
        self.assertIs(set.discard(set(), 1), None)

    def test_discard_with_member_removes_element(self):
        s = {1, 2, 3}
        self.assertIn(1, s)
        self.assertIs(set.discard(s, 1), None)
        self.assertNotIn(1, s)

    def test_mix_bool_and_int(self):
        s = set()
        s.add(1)
        self.assertIn(1, s)
        self.assertNotIn(0, s)
        self.assertIn(True, s)
        self.assertNotIn(False, s)
        self.assertEqual(len(s), 1)
        s.add(True)
        self.assertEqual(len(s), 1)

        s = set()
        s.add(False)
        self.assertIn(0, s)
        self.assertEqual(len(s), 1)
        s.add(0)
        self.assertEqual(len(s), 1)

    def test_repr_returns_str(self):
        self.assertEqual(set.__repr__(set()), "set()")
        self.assertEqual(set.__repr__({1}), "{1}")
        result = set.__repr__({1, "foo"})
        self.assertTrue(result == "{1, 'foo'}" or result == "{'foo', 1}")

        class M(type):
            def __repr__(cls):
                return "<M instance>"

        class C(metaclass=M):
            def __repr__(self):
                return "<C instance>"

        self.assertEqual(set.__repr__({C}), "{<M instance>}")
        self.assertEqual(set.__repr__({C()}), "{<C instance>}")

    def test_repr_with_subclass_returns_str(self):
        class C(set):
            pass

        self.assertEqual(set.__repr__(C()), "C()")

    def test_remove_with_non_set_raises_type_error(self):
        with self.assertRaises(TypeError):
            set.remove(None, 1)

    def test_remove_with_non_member_raises_key_error(self):
        with self.assertRaises(KeyError):
            set.remove(set(), 1)

    def test_remove_with_member_removes_element(self):
        s = {1, 2, 3}
        self.assertIn(1, s)
        set.remove(s, 1)
        self.assertNotIn(1, s)

    def test_inplace_with_non_set_raises_type_error(self):
        a = frozenset()
        self.assertRaises(TypeError, set.__ior__, a, set())

    def test_inplace_with_non_set_as_other_returns_unimplemented(self):
        a = set()
        result = set.__ior__(a, 1)
        self.assertEqual(len(a), 0)
        self.assertIs(result, NotImplemented)

    def test_inplace_or_modifies_self(self):
        a = set()
        b = {"foo"}
        result = set.__ior__(a, b)
        self.assertIs(result, a)
        self.assertEqual(len(a), 1)
        self.assertIn("foo", a)

    def test_intersection_update_with_non_set_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            set.intersection_update(frozenset(), set())
        self.assertIn(
            "'intersection_update' requires a 'set' object but received a 'frozenset'",
            str(context.exception),
        )

    def test_intersection_update_with_non_iterable_other_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            set.intersection_update(set(), 5)
        self.assertEqual(str(context.exception), "'int' object is not iterable")

    def test_intersection_update_updates_self_to_be_intersection(self):
        a = {1, 2, 3}
        a.intersection_update((2, 3, 4))
        self.assertEqual(a, {2, 3})

    def test_intersection_update_updates_self_to_be_intersection_with_multiple(self):
        a = {1, 2, 3}
        a.intersection_update((2, 3, 4), (3, 4, 5))
        self.assertEqual(a, {3})

    def test_intersection_update_with_empty_lhs_and_no_rhs_stays_empty(self):
        a = set()
        a.intersection_update()
        self.assertEqual(a, set())

    def test_intersection_update_with_empty_lhs_and_empty_rhs(self):
        a = set()
        a.intersection_update(())
        self.assertEqual(a, set())

    def test_intersection_update_with_non_empty_lhs_and_empty_rhs(self):
        a = {1, 2, 3}
        a.intersection_update(())
        self.assertEqual(a, set())

    def test_intersection_update_with_empty_lhs_and_non_empty_rhs(self):
        a = set()
        a.intersection_update({1, 2, 3})
        self.assertEqual(a, set())

    def test_sub_returns_difference(self):
        self.assertEqual(set.__sub__({1, 2}, set()), {1, 2})
        self.assertEqual(set.__sub__({1, 2}, {1}), {2})
        self.assertEqual(set.__sub__({1, 2}, {1, 2}), set())

    def test_sub_with_frozenset_returns_difference(self):
        self.assertEqual(set.__sub__({1, 2}, frozenset({1})), {2})

    def test_sub_with_non_set_raises_type_error(self):
        with self.assertRaises(TypeError):
            set.__sub__("not a set", set())
        with self.assertRaises(TypeError):
            set.__sub__("not a set", "also not a set")
        with self.assertRaises(TypeError):
            set.__sub__(frozenset(), set())

    def test_sub_with_non_set_other_returns_not_implemented(self):
        self.assertEqual(set.__sub__(set(), "not a set"), NotImplemented)

    def test_isub_returns_difference(self):
        x1 = {1, 2}
        y1 = set()
        self.assertIs(set.__isub__(x1, y1), x1)
        self.assertEqual(x1, {1, 2})

        x2 = {1, 2}
        y2 = {1}
        self.assertIs(set.__isub__(x2, y2), x2)
        self.assertEqual(x2, {2})

        x3 = {1, 2}
        y3 = {1, 2}
        self.assertIs(set.__isub__(x3, y3), x3)
        self.assertEqual(x3, set())

    def test_isub_with_frozenset_returns_difference(self):
        x2 = {1, 2}
        y2 = frozenset({1})
        self.assertIs(set.__isub__(x2, y2), x2)
        self.assertEqual(x2, {2})

    def test_isub_operator_returns_difference(self):
        x1 = {1, 2}
        y1 = set()
        x1 -= y1
        self.assertEqual(x1, {1, 2})

        x2 = {1, 2}
        y2 = {1}
        x2 -= y2
        self.assertEqual(x2, {2})

        x3 = {1, 2}
        y3 = {1, 2}
        x3 -= y3
        self.assertEqual(x3, set())

    def test_isub_with_non_set_raises_type_error(self):
        with self.assertRaises(TypeError):
            set.__isub__("not a set", set())
        with self.assertRaises(TypeError):
            set.__isub__("not a set", "also not a set")
        with self.assertRaises(TypeError):
            set.__isub__(frozenset(), set())

    def test_isub_with_non_set_other_returns_not_implemented(self):
        self.assertEqual(set.__isub__(set(), "not a set"), NotImplemented)

    def test_symmetric_difference_with_non_set_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            set.symmetric_difference(frozenset(), set())
        self.assertIn(
            "'symmetric_difference' requires a 'set' object but received a 'frozenset'",
            str(context.exception),
        )

    def test_symmetric_difference_with_non_anyset_other_returns_notimplemented(self):
        result = set.symmetric_difference(set(), ())
        self.assertIs(type(result), set)
        self.assertSetEqual(result, set())

    def test_symmetric_difference_removes_common_elements(self):
        left = {1, 2, 3}
        right = {3, 4, 5}
        result = set.symmetric_difference(left, right)
        self.assertIs(type(result), set)
        self.assertSetEqual(result, {1, 2, 4, 5})

    def test_symmetric_difference_with_iterable_removes_common_elements(self):
        left = {1, 2, 3}
        right = (3, 4, 5)
        result = set.symmetric_difference(left, right)
        self.assertIs(type(result), set)
        self.assertSetEqual(result, {1, 2, 4, 5})

    def test_symmetric_difference_update_with_non_set_self_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, "requires a 'set' object but received .+ 'int'"
        ):
            set.symmetric_difference_update(1, set())

    def test_symmetric_difference_update_with_non_iterable_other_raises_type_error(
        self,
    ):
        with self.assertRaisesRegex(TypeError, "object is not iterable"):
            set.symmetric_difference_update(set(), 1)

    def test_symmetric_difference_update_with_identity_equal_params_clears_self(self):
        a = {1, 2, 3}
        self.assertSetEqual(a, {1, 2, 3})
        set.symmetric_difference_update(a, a)
        self.assertSetEqual(a, set())

    def test_symmetric_difference_update_removes_self_items_found_in_other(self):
        left = {1, 2, 3}
        right = {3}
        set.symmetric_difference_update(left, right)
        self.assertSetEqual(left, {1, 2})
        self.assertSetEqual(right, {3})

    def test_symmetric_difference_update_adds_items_found_in_other(self):
        left = {1, 2}
        right = {3, 4, 5}
        set.symmetric_difference_update(left, right)
        self.assertSetEqual(left, {1, 2, 3, 4, 5})
        self.assertSetEqual(right, {3, 4, 5})

    def test_symmetric_difference_update_with_raising_iterable_does_not_remove(self):
        class C:
            def __init__(self):
                self.value = 0

            def __iter__(self):
                return self

            def __next__(self):
                if self.value > 3:
                    raise UserWarning("foo")
                self.value += 1
                return self.value

        left = {1, 2, 3}
        right = C()
        with self.assertRaisesRegex(UserWarning, "foo"):
            set.symmetric_difference_update(left, right)
        self.assertEqual(left, {1, 2, 3})

    def test_set_ignores_subclass_isub_update(self):
        class SubSet(set):
            def __isub__(self, other):
                pass

        a = SubSet([1, 2, 3])
        b = {2, 3}
        self.assertIs(set.__isub__(a, b), a)
        self.assertEqual(a, {1})

    def test_union_with_non_set_as_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            set.union(frozenset(), set())

    def test_union_with_set_returns_union(self):
        set1 = {1, 2}
        set2 = {2, 3}
        set3 = {4, 5}
        self.assertEqual(set.union(set1, set2, set3), {1, 2, 3, 4, 5})

    def test_union_with_self_returns_copy(self):
        a_set = {1, 2, 3}
        self.assertIsNot(set.union(a_set), a_set)
        self.assertIsNot(set.union(a_set, a_set), a_set)

    def test_union_with_iterable_contains_iterable_items(self):
        a_set = {1, 2}
        a_dict = {2: True, 3: True}
        self.assertEqual(set.union(a_set, a_dict), {1, 2, 3})

    def test_union_with_custom_iterable(self):
        class C:
            def __init__(self, start, end):
                self.pos = start
                self.end = end

            def __iter__(self):
                return self

            def __next__(self):
                if self.pos == self.end:
                    raise StopIteration
                result = self.pos
                self.pos += 1
                return result

        self.assertEqual(set.union(set(), C(1, 3), C(6, 9)), {1, 2, 6, 7, 8})

    def test_issuperset_with_non_set_raises_type_error(self):
        with self.assertRaises(TypeError):
            set.issuperset(None, set())

    def test_issuperset_with_frozenset_raises_type_error(self):
        with self.assertRaises(TypeError):
            set.issuperset(frozenset(), set())

    def test_issuperset_with_empty_sets_returns_true(self):
        self.assertTrue(set.issuperset(set(), frozenset()))
        self.assertTrue(set.issuperset(set(), set()))

    def test_issuperset_with_anyset_subset_returns_true(self):
        self.assertTrue({1}.issuperset(set()))
        self.assertTrue({1}.issuperset(frozenset()))

        self.assertTrue({1}.issuperset({1}))
        self.assertTrue({1}.issuperset(frozenset({1})))

        self.assertTrue({1, 2}.issuperset({1}))
        self.assertTrue({1, 2}.issuperset(frozenset({1})))

        self.assertTrue({1, 2}.issuperset({1, 2}))
        self.assertTrue({1, 2}.issuperset(frozenset({1, 2})))

        self.assertTrue({1, 2, 3}.issuperset({1, 2}))
        self.assertTrue({1, 2, 3}.issuperset(frozenset({1, 2})))

    def test_issuperset_with_iterable_subset_returns_true(self):
        self.assertTrue({1}.issuperset([]))
        self.assertTrue({1}.issuperset(range(1, 1)))

        self.assertTrue({1}.issuperset([1]))
        self.assertTrue({1}.issuperset(range(1, 2)))

        self.assertTrue({1, 2}.issuperset([1]))
        self.assertTrue({1, 2}.issuperset(range(1, 2)))

        self.assertTrue({1, 2}.issuperset([1, 2]))
        self.assertTrue({1, 2}.issuperset(range(1, 3)))

        self.assertTrue({1, 2, 3}.issuperset([1, 2]))
        self.assertTrue({1, 2, 3}.issuperset(range(1, 3)))

    def test_issuperset_with_superset_returns_false(self):
        self.assertFalse(set().issuperset({1}))
        self.assertFalse(set().issuperset(frozenset({1})))
        self.assertFalse(set().issuperset([1]))
        self.assertFalse(set().issuperset(range(1, 2)))

        self.assertFalse({1}.issuperset({1, 2}))
        self.assertFalse({1}.issuperset(frozenset({1, 2})))
        self.assertFalse({1}.issuperset([1, 2]))
        self.assertFalse({1}.issuperset(range(1, 3)))

        self.assertFalse({1, 2}.issuperset({1, 2, 3}))
        self.assertFalse({1, 2}.issuperset(frozenset({1, 2, 3})))
        self.assertFalse({1, 2}.issuperset([1, 2, 3]))
        self.assertFalse({1, 2}.issuperset(range(1, 4)))


class SimpleNamespaceTests(unittest.TestCase):
    def test_dunder_eq_returns_false(self):
        sn0 = SimpleNamespace(foo=42, bar="baz")
        self.assertIs(SimpleNamespace.__eq__(sn0, SimpleNamespace()), False)
        self.assertIs(SimpleNamespace.__eq__(SimpleNamespace(), sn0), False)
        sn1 = SimpleNamespace(foo=42)
        self.assertIs(SimpleNamespace.__eq__(sn0, sn1), False)
        self.assertIs(SimpleNamespace.__eq__(sn1, sn0), False)
        sn2 = SimpleNamespace(foo=42, bar="baz", bam=None)
        self.assertIs(SimpleNamespace.__eq__(sn0, sn2), False)
        self.assertIs(SimpleNamespace.__eq__(sn2, sn0), False)

    def test_dunder_eq_returns_true(self):
        self.assertIs(
            SimpleNamespace.__eq__(SimpleNamespace(), SimpleNamespace()), True
        )
        sn0 = SimpleNamespace(foo=42, bar="baz")
        sn1 = SimpleNamespace(bar="baz", foo=42)
        self.assertIs(SimpleNamespace.__eq__(sn0, sn1), True)
        self.assertIs(SimpleNamespace.__eq__(sn1, sn0), True)

    def test_dunder_eq_returns_notimplemented(self):
        self.assertIs(SimpleNamespace.__eq__(SimpleNamespace(), {}), NotImplemented)

    def test_dunder_init_returns_simple_namespace(self):

        s = SimpleNamespace(foo=42, bar="baz")
        self.assertEqual(s.foo, 42)
        self.assertEqual(s.bar, "baz")

    def test_dunder_ne_returns_false(self):
        self.assertIs(
            SimpleNamespace.__ne__(SimpleNamespace(), SimpleNamespace()), False
        )
        sn0 = SimpleNamespace(foo=42, bar="baz")
        sn1 = SimpleNamespace(bar="baz", foo=42)
        self.assertIs(SimpleNamespace.__ne__(sn0, sn1), False)
        self.assertIs(SimpleNamespace.__ne__(sn1, sn0), False)

    def test_dunder_ne_returns_true(self):
        sn0 = SimpleNamespace(foo=42, bar="baz")
        self.assertIs(SimpleNamespace.__ne__(sn0, SimpleNamespace()), True)
        self.assertIs(SimpleNamespace.__ne__(SimpleNamespace(), sn0), True)
        sn1 = SimpleNamespace(foo=42)
        self.assertIs(SimpleNamespace.__ne__(sn0, sn1), True)
        self.assertIs(SimpleNamespace.__ne__(sn1, sn0), True)
        sn2 = SimpleNamespace(foo=42, bar="baz", bam=None)
        self.assertIs(SimpleNamespace.__ne__(sn0, sn2), True)
        self.assertIs(SimpleNamespace.__ne__(sn2, sn0), True)

    def test_dunder_ne_returns_notimplemented(self):
        self.assertIs(SimpleNamespace.__ne__(SimpleNamespace(), {}), NotImplemented)

    def test_dunder_repr_returns_str(self):
        s = SimpleNamespace(foo=42, bar="baz")
        self.assertEqual(SimpleNamespace.__repr__(s), "namespace(bar='baz', foo=42)")

    def test_dunder_repr_sorts_elements_and_returns_str(self):
        s = SimpleNamespace(x=1, y=2, w=3)
        self.assertEqual(list(s.__dict__.items()), [("x", 1), ("y", 2), ("w", 3)])
        self.assertEqual(SimpleNamespace.__repr__(s), "namespace(w=3, x=1, y=2)")
        s = SimpleNamespace(w=3, y=2, x=1)
        self.assertEqual(list(s.__dict__.items()), [("w", 3), ("y", 2), ("x", 1)])
        self.assertEqual(SimpleNamespace.__repr__(s), "namespace(w=3, x=1, y=2)")

    def test_dunder_repr_recursive_returns_str(self):
        ns0 = SimpleNamespace()
        ns0.n = ns0
        self.assertEqual(SimpleNamespace.__repr__(ns0), "namespace(n=namespace(...))")

        ns1 = SimpleNamespace()
        ns2 = SimpleNamespace()
        ns1.foo = ns2
        ns2.bar = ns1
        self.assertEqual(
            SimpleNamespace.__repr__(ns1),
            "namespace(foo=namespace(bar=namespace(...)))",
        )
        self.assertEqual(
            SimpleNamespace.__repr__(ns2),
            "namespace(bar=namespace(foo=namespace(...)))",
        )

    def test_simple_namespace_is_non_heaptype(self):
        self.assertFalse(SimpleNamespace.__flags__ & Py_TPFLAGS_HEAPTYPE)


class SliceTest(unittest.TestCase):
    def test_dunder_repr_with_nones_returns_str(self):
        s = slice(None)
        self.assertEqual(repr(s), "slice(None, None, None)")

    def test_dunder_repr_with_stop_returns_str(self):
        s = slice(5)
        self.assertEqual(repr(s), "slice(None, 5, None)")

    def test_dunder_repr_with_values_returns_str(self):
        s = slice(1, 5, 2)
        self.assertEqual(repr(s), "slice(1, 5, 2)")

    def test_dunder_repr_calls_dunder_repr(self):
        class C:
            def __repr__(self):
                return "repr"

            def __str__(self):
                return "str"

        c = C()
        s = slice(c, c, c)
        self.assertEqual(repr(s), "slice(repr, repr, repr)")


class StaticMethodTests(unittest.TestCase):
    def test_dunder_abstractmethod_with_missing_attr_returns_false(self):
        def foo():
            pass

        method = staticmethod(foo)
        self.assertIs(method.__isabstractmethod__, False)

    def test_dunder_abstractmethod_with_false_attr_returns_false(self):
        def foo():
            pass

        foo.__isabstractmethod__ = False
        with self.assertRaises(AttributeError):
            type(foo).__isabstractmethod__
        method = staticmethod(foo)
        self.assertIs(method.__isabstractmethod__, False)

    def test_dunder_abstractmethod_with_abstract_returns_true(self):
        def foo():
            pass

        foo.__isabstractmethod__ = 10  # non-zero is True
        with self.assertRaises(AttributeError):
            type(foo).__isabstractmethod__
        method = staticmethod(foo)
        self.assertIs(method.__isabstractmethod__, True)

    def test_dunder_abstractmethod_with_non_staticmethod_raises_type_error(self):
        with self.assertRaises(TypeError):
            staticmethod.__dict__["__isabstractmethod__"].__get__(42)

    def test_dunder_func_returns_function(self):
        def foo():
            pass

        method = staticmethod(foo)
        self.assertIs(method.__func__, foo)

    def test_dunder_func_assign_raises_attribute_error(self):
        def foo():
            pass

        def bar():
            pass

        method = staticmethod(foo)
        with self.assertRaises(AttributeError):
            method.__func__ = bar

    def test_has_dunder_call(self):
        class C:
            @staticmethod
            def bar(self):
                pass

        C.bar.__getattribute__("__call__")

    def test_dunder_get_returns_value(self):
        class C:
            @staticmethod
            def bar(self):
                return 5

        self.assertEqual(C.bar(C()), 5)

    def test_dunder_get_with_subclassed_staticmethod_returns_value(self):
        class foo(staticmethod):
            pass

        class C:
            @foo
            def bar(self):
                return 5

        self.assertEqual(C.bar(C()), 5)

    def test_dunder_get_called_with_non_staticmethod_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            def bar():
                pass

            staticmethod.__get__(42.3, None, bar)
        self.assertIn(
            "'__get__' requires a 'staticmethod' object but received a 'float'",
            str(context.exception),
        )

    def test_dunder_new_called_with_non_type_object_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            def bar():
                pass

            staticmethod.__new__(42.3, bar)
        self.assertIn("not a type object", str(context.exception))

    def test_dunder_new_called_with_non_subtype_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            def bar():
                pass

            staticmethod.__new__(float, bar)
        self.assertIn("not a subtype of staticmethod", str(context.exception))

    def test_dunder_new_with_subclassed_staticmethod_returns_instance_of_superclass(
        self,
    ):
        class foo(staticmethod):
            pass

        def bar():
            pass

        self.assertIsInstance(staticmethod.__new__(foo, bar), foo)


class SumTests(unittest.TestCase):
    def test_str_as_start_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            sum([], "")
        self.assertEqual(
            str(context.exception), "sum() can't sum strings [use ''.join(seq) instead]"
        )

    def test_bytes_as_start_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            sum([], b"")
        self.assertEqual(
            str(context.exception), "sum() can't sum bytes [use b''.join(seq) instead]"
        )

    def test_bytearray_as_start_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            sum([], bytearray())
        self.assertEqual(
            str(context.exception),
            "sum() can't sum bytearray [use b''.join(seq) instead]",
        )

    def test_non_iterable_raises_type_error(self):
        with self.assertRaises(TypeError):
            sum(None)
        # TODO(T43043290): Check for the exception message.

    def test_int_list_without_start_returns_sum(self):
        result = sum([1, 2, 3])
        self.assertEqual(result, 6)

    def test_int_list_with_start_returns_sum(self):
        result = sum([1, 2, 3], -6)
        self.assertEqual(result, 0)


class SuperTests(unittest.TestCase):
    def test_dunder_thisclass_returns_this_class(self):
        class C:
            def get_super(self):
                return super()

        class D(C):
            pass

        self.assertIs(D().get_super().__thisclass__, C)

    def test_dunder_self_returns_self(self):
        class C:
            def get_super(self):
                return super()

        class D(C):
            pass

        i = D()
        self.assertIs(i.get_super().__self__, i)

    def test_dunder_self_class_returns_self_class(self):
        class C:
            def get_super(self):
                return super()

        class D(C):
            pass

        self.assertIs(D().get_super().__self_class__, D)

    def test_dunder_init_resolves_cell_vars(self):
        class C:
            def foo(self, a1):
                def compute():
                    # This turns self, a1 into freevars in `compute` and
                    # cellvars in `foo`. I am also using a1 here to have
                    # `self` move at the end of the freevar/cellvar list to
                    # make the test more interesting.
                    self.dummy = a1

                compute()
                s = super()
                return s

        self.assertIn("self", C.foo.__code__.co_cellvars)

        instance = C()
        s = instance.foo(None)
        self.assertEqual(s.__self__, instance)
        self.assertEqual(s.__self_class__, C)
        self.assertEqual(s.__thisclass__, C)


class SyntaxErrorTests(unittest.TestCase):
    def test_dunder_init_with_no_args_sets_msg_to_none(self):
        obj = SyntaxError()
        self.assertIsNone(obj.msg)

    def test_dunder_init_with_arg_sets_msg_to_first_arg(self):
        obj = SyntaxError("hello")
        self.assertEqual(obj.msg, "hello")

    def test_dunder_init_with_tuple_of_wrong_length_raises_index_error(self):
        with self.assertRaises(IndexError) as context:
            SyntaxError("hello", ())
        self.assertEqual(str(context.exception), "tuple index out of range")

    def test_dunder_init_sets_attributes(self):
        obj = SyntaxError("msg", ("filename", "lineno", "offset", "text"))
        self.assertEqual(obj.msg, "msg")
        self.assertEqual(obj.filename, "filename")
        self.assertEqual(obj.lineno, "lineno")
        self.assertEqual(obj.offset, "offset")
        self.assertEqual(obj.text, "text")

    def test_dunder_str_with_no_filename_and_no_lineno_returns_msg(self):
        obj = SyntaxError("hello")
        self.assertEqual(obj.__str__(), "hello")

    def test_dunder_str_calls_dunder_str_on_message(self):
        class C:
            def __str__(self):
                return "foo"

        obj = SyntaxError(C())
        result = obj.__str__()
        self.assertEqual(result, "foo")

    def test_dunder_str_with_no_message_returns_none_string(self):
        obj = SyntaxError()
        result = obj.__str__()
        self.assertEqual(result, "None")

    def test_dunder_str_with_filename_and_lineno(self):
        obj = SyntaxError("msg", ("filename", 5, "offset", "text"))
        result = obj.__str__()
        self.assertEqual(result, "msg (filename, line 5)")

    def test_dunder_str_with_non_str_filename_and_lineno(self):
        obj = SyntaxError("msg", (10, 5, "offset", "text"))
        result = obj.__str__()
        self.assertEqual(result, "msg (line 5)")

    def test_dunder_str_with_non_int_lineno_and_filename(self):
        obj = SyntaxError("msg", ("filename", "lineno", "offset", "text"))
        result = obj.__str__()
        self.assertEqual(result, "msg (filename)")

    def test_dunder_str_with_str_filename_and_int_lineno(self):
        obj = SyntaxError("msg", ("path/to/foo.py", 5, "offset", "text"))
        result = obj.__str__()
        self.assertEqual(result, "msg (foo.py, line 5)")

    def test_dunder_str_with_str_filename_and_non_int_lineno(self):
        obj = SyntaxError("msg", ("path/to/foo.py", "lineno", "offset", "test"))
        result = obj.__str__()
        self.assertEqual(result, "msg (foo.py)")


class TupleTests(unittest.TestCase):
    def test_dunder_repr_with_single_element(self):
        self.assertEqual(repr((1,)), "(1,)")
        self.assertEqual(repr(([1],)), "([1],)")

    def test_dunder_repr_with_single_element_recursive(self):
        x = []
        y = (x,)
        x.append(y)
        self.assertEqual(repr(y), "([(...)],)")

    def test_dunder_repr_with_multi_element(self):
        self.assertEqual(repr((1, [])), "(1, [])")

    def test_dunder_repr_with_multi_element_recursive(self):
        x = []
        y = (x, [], 1)
        x.append(y)
        self.assertEqual(repr(y), "([(...)], [], 1)")

    def test_dunder_eq_with_non_tuple_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            tuple.__eq__(None, ())
        self.assertIn(
            "'__eq__' requires a 'tuple' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_dunder_eq_with_non_tuple_other_return_not_implemented(self):
        self.assertIs(tuple.__eq__((), None), NotImplemented)

    def test_dunder_eq_with_equal_tuples_returns_true(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3, 4)
        self.assertTrue(tuple.__eq__(lhs, rhs))

    def test_dunder_eq_with_longer_rhs_returns_false(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3, 4, 5)
        self.assertFalse(tuple.__eq__(lhs, rhs))

    def test_dunder_eq_with_shorter_rhs_returns_false(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3)
        self.assertFalse(tuple.__eq__(lhs, rhs))

    def test_dunder_eq_with_smaller_element_returns_false(self):
        lhs = (1, 2, 3)
        rhs = (1, 2, 2, 4)
        self.assertFalse(tuple.__eq__(lhs, rhs))

    def test_dunder_eq_with_larger_element_returns_false(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 4)
        self.assertFalse(tuple.__eq__(lhs, rhs))

    def test_dunder_ge_with_non_tuple_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            tuple.__ge__(None, ())
        self.assertIn(
            "'__ge__' requires a 'tuple' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_dunder_ge_with_non_tuple_other_return_not_implemented(self):
        self.assertIs(tuple.__ge__((), None), NotImplemented)

    def test_dunder_ge_with_equal_tuples_returns_true(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3, 4)
        self.assertTrue(tuple.__ge__(lhs, rhs))

    def test_dunder_ge_with_longer_rhs_returns_false(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3, 4, 5)
        self.assertFalse(tuple.__ge__(lhs, rhs))

    def test_dunder_ge_with_shorter_rhs_returns_true(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3)
        self.assertTrue(tuple.__ge__(lhs, rhs))

    def test_dunder_ge_with_smaller_element_returns_true(self):
        lhs = (1, 2, 3)
        rhs = (1, 2, 2, 4)
        self.assertTrue(tuple.__ge__(lhs, rhs))

    def test_dunder_ge_with_larger_element_returns_false(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 4)
        self.assertFalse(tuple.__ge__(lhs, rhs))

    def test_dunder_getitem_with_non_tuple_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            tuple.__getitem__(None, 1)
        self.assertIn(
            "'__getitem__' requires a 'tuple' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_dunder_getitem_with_int_subclass_does_not_call_dunder_index(self):
        class C(int):
            def __index__(self):
                raise ValueError("foo")

        t = (1, 2, 3)
        self.assertEqual(tuple.__getitem__(t, C(0)), 1)

    def test_dunder_getitem_with_raising_descriptor_propagates_exception(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("foo")

        class C:
            __index__ = Desc()

        t = (1, 2, 3)
        with self.assertRaises(AttributeError) as context:
            tuple.__getitem__(t, C())
        self.assertEqual(str(context.exception), "foo")

    def test_dunder_getitem_with_non_int_raises_type_error(self):
        t = (1, 2, 3)
        with self.assertRaises(TypeError) as context:
            tuple.__getitem__(t, "3")
        self.assertEqual(
            str(context.exception), "tuple indices must be integers or slices, not str"
        )

    def test_dunder_getitem_with_tuple_subclass_returns_value(self):
        class Foo(tuple):
            pass

        t = Foo((0, 1))
        self.assertEqual(tuple.__getitem__(t, 0), 0)

    def test_dunder_getitem_slice_with_tuple_subclass_returns_tuple(self):
        class Foo(tuple):
            pass

        t = Foo((0, 1))
        self.assertEqual(tuple.__getitem__(t, slice(2)), (0, 1))

    def test_dunder_getitem_with_dunder_index_calls_dunder_index(self):
        class C:
            def __index__(self):
                return 2

        t = (1, 2, 3)
        self.assertEqual(tuple.__getitem__(t, C()), 3)

    def test_dunder_getitem_index_too_small_raises_index_error(self):
        t = ()
        with self.assertRaises(IndexError) as context:
            tuple.__getitem__(t, -1)
        self.assertEqual(str(context.exception), "tuple index out of range")

    def test_dunder_getitem_index_too_large_raises_index_error(self):
        t = (1, 2, 3)
        with self.assertRaises(IndexError) as context:
            tuple.__getitem__(t, 3)
        self.assertEqual(str(context.exception), "tuple index out of range")

    def test_dunder_getitem_negative_index_relative_to_end_value_error(self):
        t = (1, 2, 3)
        self.assertEqual(tuple.__getitem__(t, -3), 1)

    def test_dunder_getitem_with_valid_indices_returns_subtuple(self):
        t = (1, 2, 3, 4, 5)
        self.assertEqual(tuple.__getitem__(t, slice(2, -1)), (3, 4))

    def test_dunder_getitem_with_negative_start_returns_trailing(self):
        t = (1, 2, 3)
        self.assertEqual(tuple.__getitem__(t, slice(-2, 5)), (2, 3))

    def test_dunder_getitem_with_positive_stop_returns_leading(self):
        t = (1, 2, 3)
        self.assertEqual(tuple.__getitem__(t, slice(2)), (1, 2))

    def test_dunder_getitem_with_negative_stop_returns_all_but_trailing(self):
        t = (1, 2, 3)
        self.assertEqual(tuple.__getitem__(t, slice(-2)), (1,))

    def test_dunder_getitem_with_positive_step_returns_forwards_list(self):
        t = (1, 2, 3)
        self.assertEqual(tuple.__getitem__(t, slice(0, 10, 2)), (1, 3))

    def test_dunder_getitem_with_negative_step_returns_backwards_list(self):
        t = (1, 2, 3)
        self.assertEqual(tuple.__getitem__(t, slice(2, -10, -2)), (3, 1))

    def test_dunder_getitem_with_large_negative_start_returns_copy(self):
        t = (1, 2, 3)
        self.assertIs(tuple.__getitem__(t, slice(-10, 10)), t)

    def test_dunder_getitem_with_large_positive_start_returns_empty(self):
        t = (1, 2, 3)
        self.assertEqual(tuple.__getitem__(t, slice(10, 10)), ())

    def test_dunder_getitem_with_large_negative_stop_returns_empty(self):
        t = (1, 2, 3)
        self.assertEqual(tuple.__getitem__(t, slice(-10)), ())

    def test_dunder_getitem_with_large_positive_stop_returns_same_object(self):
        t = (1, 2, 3)
        self.assertIs(tuple.__getitem__(t, slice(10)), t)

    def test_dunder_gt_with_non_tuple_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            tuple.__gt__(None, ())
        self.assertIn(
            "'__gt__' requires a 'tuple' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_dunder_gt_with_non_tuple_other_return_not_implemented(self):
        self.assertIs(tuple.__gt__((), None), NotImplemented)

    def test_dunder_gt_with_equal_tuples_returns_false(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3, 4)
        self.assertFalse(tuple.__gt__(lhs, rhs))

    def test_dunder_gt_with_longer_rhs_returns_false(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3, 4, 5)
        self.assertFalse(tuple.__gt__(lhs, rhs))

    def test_dunder_gt_with_shorter_rhs_returns_true(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3)
        self.assertTrue(tuple.__gt__(lhs, rhs))

    def test_dunder_gt_with_smaller_element_returns_true(self):
        lhs = (1, 2, 3)
        rhs = (1, 2, 2, 4)
        self.assertTrue(tuple.__gt__(lhs, rhs))

    def test_dunder_gt_with_larger_element_returns_false(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 4)
        self.assertFalse(tuple.__gt__(lhs, rhs))

    def test_dunder_le_with_non_tuple_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            tuple.__le__(None, ())
        self.assertIn(
            "'__le__' requires a 'tuple' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_dunder_le_with_non_tuple_other_return_not_implemented(self):
        self.assertIs(tuple.__le__((), None), NotImplemented)

    def test_dunder_le_with_equal_tuples_returns_true(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3, 4)
        self.assertTrue(tuple.__le__(lhs, rhs))

    def test_dunder_le_with_longer_rhs_returns_true(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3, 4, 5)
        self.assertTrue(tuple.__le__(lhs, rhs))

    def test_dunder_le_with_shorter_rhs_returns_false(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3)
        self.assertFalse(tuple.__le__(lhs, rhs))

    def test_dunder_le_with_smaller_element_returns_false(self):
        lhs = (1, 2, 3)
        rhs = (1, 2, 2, 4)
        self.assertFalse(tuple.__le__(lhs, rhs))

    def test_dunder_le_with_larger_element_returns_true(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 4)
        self.assertTrue(tuple.__le__(lhs, rhs))

    def test_dunder_lt_with_non_tuple_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            tuple.__lt__(None, ())
        self.assertIn(
            "'__lt__' requires a 'tuple' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_dunder_lt_with_non_tuple_other_returns_not_implemented(self):
        self.assertIs(tuple.__lt__((), None), NotImplemented)

    def test_dunder_lt_with_equal_tuples_returns_false(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3, 4)
        self.assertFalse(tuple.__lt__(lhs, rhs))

    def test_dunder_lt_with_longer_rhs_returns_true(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3, 4, 5)
        self.assertTrue(tuple.__lt__(lhs, rhs))

    def test_dunder_lt_with_shorter_rhs_returns_false(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3)
        self.assertFalse(tuple.__lt__(lhs, rhs))

    def test_dunder_lt_with_smaller_element_returns_false(self):
        lhs = (1, 2, 3)
        rhs = (1, 2, 2, 4)
        self.assertFalse(tuple.__lt__(lhs, rhs))

    def test_dunder_lt_with_larger_element_returns_true(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 4)
        self.assertTrue(tuple.__lt__(lhs, rhs))

    def test_dunder_ne_with_non_tuple_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            tuple.__ne__(None, ())
        self.assertIn(
            "'__ne__' requires a 'tuple' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_dunder_ne_with_non_tuple_other_returns_not_implemented(self):
        self.assertIs(tuple.__lt__((), None), NotImplemented)

    def test_dunder_ne_with_equal_tuples_returns_false(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3, 4)
        self.assertFalse(tuple.__ne__(lhs, rhs))

    def test_dunder_ne_with_different_element_returns_true(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3, 5)
        self.assertTrue(tuple.__ne__(lhs, rhs))

    def test_dunder_ne_with_longer_rhs_returns_true(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3, 4, 5)
        self.assertTrue(tuple.__ne__(lhs, rhs))

    def test_dunder_ne_with_shorter_rhs_returns_true(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 2, 3)
        self.assertTrue(tuple.__ne__(lhs, rhs))

    def test_dunder_ne_with_smaller_element_returns_true(self):
        lhs = (1, 2, 3)
        rhs = (1, 2, 2, 4)
        self.assertTrue(tuple.__ne__(lhs, rhs))

    def test_dunder_ne_with_larger_element_returns_true(self):
        lhs = (1, 2, 3, 4)
        rhs = (1, 4)
        self.assertTrue(tuple.__ne__(lhs, rhs))

    def test_dunder_new_with_no_iterable_arg_returns_empty_tuple(self):
        result = tuple.__new__(tuple)
        self.assertIs(result, ())

    def test_dunder_new_with_tuple_returns_same_object(self):
        src = (1, 2, 3)
        result = tuple.__new__(tuple, src)
        self.assertIs(result, src)

    def test_dunder_new_with_list_returns_tuple(self):
        self.assertEqual(tuple([1, 2, 3]), (1, 2, 3))

    def test_dunder_new_with_tuple_subclass_and_tuple_returns_new_object(self):
        class C(tuple):
            pass

        src = (1, 2, 3)
        result = tuple.__new__(C, src)
        self.assertTrue(result is not src)

    def test_dunder_new_with_iterable_returns_tuple_with_elements(self):
        result = tuple.__new__(tuple, [1, 2, 3])
        self.assertEqual(result, (1, 2, 3))

    def test_dunder_new_with_tuple_subclass_calls_dunder_iter(self):
        class C(tuple):
            def __iter__(self):
                raise UserWarning("foo")

        c = C()
        self.assertRaises(UserWarning, tuple, c)

    def test_dunder_new_with_list_subclass_calls_dunder_iter(self):
        class C(list):
            def __iter__(self):
                raise UserWarning("foo")

        c = C()
        self.assertRaises(UserWarning, tuple, c)

    def test_dunder_new_with_raising_dunder_iter_descriptor_raises_type_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise UserWarning("foo")

        class C:
            __iter__ = Desc()

        with self.assertRaises(TypeError) as context:
            tuple(C())
        self.assertEqual(str(context.exception), "'C' object is not iterable")

    def test_dunder_new_with_raising_dunder_next_descriptor_propagates_exception(self):
        class Desc:
            def __get__(self, obj, type):
                raise UserWarning("foo")

        class C:
            def __iter__(self):
                return self

            __next__ = Desc()

        with self.assertRaises(UserWarning):
            tuple(C())

    def test_dunder_rmul_with_one_element(self):
        left = (1,)
        result = tuple.__rmul__(left, 4)
        self.assertTupleEqual(result, (1, 1, 1, 1))

    def test_dunder_rmul_with_length_greater_than_one_and_times_is_one(self):
        left = (1, 2, 3)
        result = tuple.__rmul__(left, 1)
        self.assertIs(result, left)

    def test_dunder_rmul_with_length_greater_than_one_and_times_greater_than_one(self):
        left = (1, 2, 3)
        result = tuple.__rmul__(left, 2)
        self.assertTupleEqual(result, (1, 2, 3, 1, 2, 3))

    def test_dunder_rmul_with_empty_tuple(self):
        left = ()
        result = tuple.__rmul__(left, 5)
        self.assertIs(result, left)

    def test_dunder_rmul_with_negative_times(self):
        left = (1, 2, 3)
        result = tuple.__rmul__(left, -2)
        self.assertIs(result, ())

    def test_dunder_rmul_with_tuple_subclass_returns_tuple(self):
        class C(tuple):
            pass

        instance = C((1, 2, 3))
        result = tuple.__rmul__(instance, 2)
        self.assertEqual(result.__class__, tuple)
        self.assertTupleEqual(result, (1, 2, 3, 1, 2, 3))

    def test_index_with_non_tuple_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            tuple.index(object(), object())

    def test_index_with_no_start_starts_at_zero(self):
        result = (1, 2, 3, 1, 2, 3).index(1)
        self.assertEqual(result, 0)

    def test_index_with_negative_start_starts_from_end(self):
        result = (1, 2, 3, 1, 2, 3).index(3, -1)
        self.assertEqual(result, 5)

    def test_index_with_negative_start_greater_than_length_starts_from_zero(self):
        result = (1, 2, 3, 1, 2, 3).index(3, -10)
        self.assertEqual(result, 2)

    def test_index_with_no_stop_ends_at_length(self):
        result = (1, 2, 3, 4).index(4)
        self.assertEqual(result, 3)

    def test_index_starts_at_given_start(self):
        result = (1, 2, 3, 1, 2, 3).index(1, 2)
        self.assertEqual(result, 3)

    def test_index_with_negative_one_stops_before_end(self):
        with self.assertRaisesRegex(ValueError, "not in tuple"):
            (1, 2, 3, 4).index(4, 0, -1)

    def test_index_with_negative_stop_stops_at_end(self):
        result = (1, 2, 3, 4).index(3, 0, -1)
        self.assertEqual(result, 2)

    def test_index_with_negative_stop_greater_than_length_stops_at_zero(self):
        with self.assertRaisesRegex(ValueError, "not in tuple"):
            (1, 2, 3).index(3, 0, -10)

    def test_index_stops_at_given_stop(self):
        with self.assertRaisesRegex(ValueError, "not in tuple"):
            (1, 2, 3, 4).index(4, 0, 3)

    def test_index_compares_identity(self):
        class C:
            __eq__ = Mock(name="__eq__", return_value=False)

        instance = C()
        result = (instance,).index(instance)
        self.assertEqual(result, 0)
        instance.__eq__.assert_not_called()

    def test_index_calls_dunder_index_on_start(self):
        class C:
            __index__ = Mock(name="__index__", return_value=0)

        result = (1, 2, 3).index(2, C())
        self.assertEqual(result, 1)
        C.__index__.assert_called_once()

    def test_index_calls_dunder_index_on_stop(self):
        class C:
            __index__ = Mock(name="__index__", return_value=5)

        result = (1, 2, 3).index(2, 0, C())
        self.assertEqual(result, 1)
        C.__index__.assert_called_once()

    def test_index_calls_dunder_eq(self):
        class C:
            __eq__ = Mock(name="__eq__", return_value=False)

        instance = C()
        result = (instance, 4).index(4)
        self.assertEqual(result, 1)
        instance.__eq__.assert_called_once()

    def test_count_with_non_tuple_raises_type_error(self):
        with self.assertRaises(TypeError):
            tuple.count("foo", 3)

    def test_count_with_empty_returns_zero(self):
        result = ().count(3)
        self.assertEqual(result, 0)

    def test_count_with_no_matches_returns_zero(self):
        result = (2, 4, 6, 8).count(3)
        self.assertEqual(result, 0)

    def test_count_with_mixed_elements(self):
        t = (2, [1], 6, 8)
        self.assertEqual(t.count(3), 0)
        self.assertEqual(t.count(2), 1)

    def test_count_with_identical_elements(self):
        N = 5
        t = (2,) * N
        self.assertEqual(t.count(3), 0)
        self.assertEqual(t.count(2), N)

    def test_count_with_equivalent_objects(self):
        o = object()
        self.assertEqual(().count(o), 0)
        self.assertEqual((o,).count(o), 1)
        self.assertEqual((o, o).count(o), 2)

    def test_count_with_incomparable_elements_raises(self):
        class MyException(Exception):
            pass

        class NoCompare:
            def __eq__(self, other):
                raise MyException("oops")

        # Create an object that doesn't support __eq__
        # The exception should pass through the count
        bad = NoCompare()

        with self.assertRaises(MyException):
            (1, 2, bad, 3).count(3)

        with self.assertRaises(MyException):
            (1, 2, 3, 4).count(bad)

        self.assertEqual((bad,).count(bad), 1)

    def test_count_with_nan(self):
        nan = float("nan")
        self.assertEqual(().count(nan), 0)
        self.assertEqual((nan,).count(nan), 1)

    def test_count_does_not_call_dunder_iter(self):
        class NoIterTuple(tuple):
            def __iter__(self):
                raise NotImplementedError("no iter")

        t = NoIterTuple((1, 2, 3, 4, 3, 2, 1))
        self.assertEqual(t.count(3), 2)


class TypeTests(unittest.TestCase):
    def test_abstract_methods_get_with_builtin_type_raises_attribute_error(self):
        with self.assertRaises(AttributeError) as context:
            type.__abstractmethods__
        self.assertEqual(str(context.exception), "__abstractmethods__")

    def test_abstract_methods_get_with_type_subclass_raises_attribute_error(self):
        class Foo(type):
            pass

        with self.assertRaises(AttributeError) as context:
            Foo.__abstractmethods__
        self.assertEqual(str(context.exception), "__abstractmethods__")

    def test_abstract_methods_get_with_non_type_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__dict__["__abstractmethods__"].__get__(42)

    def test_abstract_methods_set_with_builtin_type_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            int.__abstractmethods__ = ["foo"]
        self.assertEqual(
            str(context.exception),
            "can't set attributes of built-in/extension type 'int'",
        )

    def test_abstract_methods_get_with_type_subclass_sets_attribute(self):
        class Foo(type):
            pass

        Foo.__abstractmethods__ = 1
        self.assertEqual(Foo.__abstractmethods__, 1)

    def test_abstract_methods_del_with_builtin_type_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            del str.__abstractmethods__
        self.assertEqual(
            str(context.exception),
            "can't set attributes of built-in/extension type 'str'",
        )

    def test_abstract_methods_del_unset_with_type_subclass_raises_attribute_error(self):
        class Foo(type):
            pass

        with self.assertRaises(AttributeError) as context:
            del Foo.__abstractmethods__
        self.assertEqual(str(context.exception), "__abstractmethods__")

    def test_abstract_methods_del_with_non_type_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__dict__["__abstractmethods__"].__delete__(42)

    def test_abstract_methods_set_with_non_type_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__dict__["__abstractmethods__"].__set__(42, [])

    def test_builtin_types_have_no_module_attribute(self):
        from types import FrameType

        self.assertNotIn("__module__", int.__dict__)
        self.assertNotIn("__module__", object.__dict__)
        self.assertNotIn("__module__", tuple.__dict__)
        self.assertNotIn("__module__", FrameType.__dict__)

    def test_dunder_base_with_object_type_returns_none(self):
        self.assertIs(object.__base__, None)

    def test_dunder_base_with_builtin_type_returns_supertype(self):
        self.assertIs(bool.__base__, int)
        self.assertIs(int.__base__, object)
        self.assertIs(str.__base__, object)
        self.assertIs(type.__base__, object)

    def test_dunder_base_with_user_type_returns_best_base(self):
        class A:
            pass

        class B(A, str):
            pass

        class C(B):
            pass

        self.assertIs(A.__base__, object)
        self.assertIs(B.__base__, str)
        self.assertIs(C.__base__, B)

    def test_dunder_bases_del_with_builtin_type_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            del object.__bases__
        self.assertEqual(
            str(context.exception),
            "can't set attributes of built-in/extension type 'object'",
        )

    def test_dunder_bases_del_with_user_type_raises_type_error(self):
        class C:
            pass

        with self.assertRaises(TypeError) as context:
            del C.__bases__
        self.assertEqual(str(context.exception), "can't delete C.__bases__")

    def test_dunder_bases_del_with_non_type_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__dict__["__bases__"].__delete__(42)

    def test_dunder_bases_get_with_builtin_type_returns_tuple(self):
        self.assertEqual(object.__bases__, ())
        self.assertEqual(type.__bases__, (object,))
        self.assertEqual(int.__bases__, (object,))
        self.assertEqual(bool.__bases__, (int,))

    def test_dunder_bases_get_with_user_type_returns_tuple(self):
        class A:
            pass

        class B:
            pass

        class C(A):
            pass

        class D(C, B):
            pass

        self.assertEqual(A.__bases__, (object,))
        self.assertEqual(B.__bases__, (object,))
        self.assertEqual(C.__bases__, (A,))
        self.assertEqual(D.__bases__, (C, B))

    def test_dunder_bases_get_with_non_type_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__dict__["__bases__"].__get__(42)

    def test_dunder_basicsize_get_with_non_type_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__dict__["__basicsize__"].__get__(42)

    def test_dunder_dir_with_non_type_object_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__dir__(None)

    def test_dunder_doc_on_empty_class_is_none(self):
        class C:
            pass

        self.assertIsNone(C.__doc__)
        instance = C()
        self.assertIsNone(instance.__doc__)

    def test_dunder_doc_accessible_via_instance(self):
        class C:
            """docstring"""

            pass

        self.assertEqual(C.__doc__, "docstring")
        instance = C()
        self.assertEqual(instance.__doc__, "docstring")

    def test_type_dunder_doc_is_not_inheritable(self):
        class C:
            """docstring"""

            pass

        class D(C):
            pass

        self.assertEqual(C.__doc__, "docstring")
        self.assertIsNone(D.__doc__)

    def test_dunder_flags_returns_basetype_set(self):
        class C:
            pass

        self.assertTrue(object.__flags__ & Py_TPFLAGS_BASETYPE)
        self.assertTrue(float.__flags__ & Py_TPFLAGS_BASETYPE)
        self.assertTrue(C.__flags__ & Py_TPFLAGS_BASETYPE)

    def test_dunder_flags_returns_basetype_clear(self):
        self.assertFalse(bool.__flags__ & Py_TPFLAGS_BASETYPE)
        str_iter_type = type(iter(""))
        self.assertFalse(str_iter_type.__flags__ & Py_TPFLAGS_BASETYPE)

    def test_dunder_flags_with_managed_type_is_heap_type(self):
        class C:
            pass

        self.assertTrue(C.__flags__ & Py_TPFLAGS_HEAPTYPE)

    def test_dunder_flags_with_managed_type_is_ready(self):
        class C:
            pass

        self.assertTrue(C.__flags__ & Py_TPFLAGS_READY)
        self.assertFalse(C.__flags__ & Py_TPFLAGS_READYING)

    def test_dunder_flags_without_dunder_abstractmethods_returns_false(self):
        class C:
            pass

        with self.assertRaises(AttributeError):
            C.__abstractmethods__

        self.assertFalse(C.__flags__ & Py_TPFLAGS_IS_ABSTRACT)

    def test_dunder_flags_with_empty_dunder_abstractmethods_returns_false(self):
        import abc

        class C(metaclass=abc.ABCMeta):
            pass

        self.assertEqual(len(C.__abstractmethods__), 0)
        self.assertFalse(C.__flags__ & Py_TPFLAGS_IS_ABSTRACT)

    def test_dunder_flags_with_non_empty_dunder_abstractmethods_returns_true(self):
        import abc

        class C(metaclass=abc.ABCMeta):
            @abc.abstractmethod
            def foo(self):
                pass

        self.assertEqual(len(C.__abstractmethods__), 1)
        self.assertTrue(C.__flags__ & Py_TPFLAGS_IS_ABSTRACT)

    def test_dunder_flags_with_non_type_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__dict__["__flags__"].__get__(42)

    def test_dunder_flags_sets_long_subclass_if_int_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & Py_TPFLAGS_LONG_SUBCLASS)

        class D(int):
            pass

        self.assertTrue(int.__flags__ & Py_TPFLAGS_LONG_SUBCLASS)
        self.assertTrue(D.__flags__ & Py_TPFLAGS_LONG_SUBCLASS)

    def test_dunder_flags_sets_list_subclass_if_list_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & Py_TPFLAGS_LIST_SUBCLASS)

        class D(list):
            pass

        self.assertTrue(list.__flags__ & Py_TPFLAGS_LIST_SUBCLASS)
        self.assertTrue(D.__flags__ & Py_TPFLAGS_LIST_SUBCLASS)

    def test_dunder_flags_sets_tuple_subclass_if_tuple_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & Py_TPFLAGS_TUPLE_SUBCLASS)

        class D(tuple):
            pass

        self.assertTrue(tuple.__flags__ & Py_TPFLAGS_TUPLE_SUBCLASS)
        self.assertTrue(D.__flags__ & Py_TPFLAGS_TUPLE_SUBCLASS)

    def test_dunder_flags_sets_bytes_subclass_if_bytes_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & Py_TPFLAGS_BYTES_SUBCLASS)

        class D(bytes):
            pass

        self.assertTrue(bytes.__flags__ & Py_TPFLAGS_BYTES_SUBCLASS)
        self.assertTrue(D.__flags__ & Py_TPFLAGS_BYTES_SUBCLASS)

    def test_dunder_flags_sets_unicode_subclass_if_str_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & Py_TPFLAGS_UNICODE_SUBCLASS)

        class D(str):
            pass

        self.assertTrue(str.__flags__ & Py_TPFLAGS_UNICODE_SUBCLASS)
        self.assertTrue(D.__flags__ & Py_TPFLAGS_UNICODE_SUBCLASS)

    def test_dunder_flags_sets_dict_subclass_if_dict_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & Py_TPFLAGS_DICT_SUBCLASS)

        class D(dict):
            pass

        self.assertTrue(dict.__flags__ & Py_TPFLAGS_DICT_SUBCLASS)
        self.assertTrue(D.__flags__ & Py_TPFLAGS_DICT_SUBCLASS)

    def test_dunder_flags_sets_base_exc_subclass_if_base_exception_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & Py_TPFLAGS_BASE_EXC_SUBCLASS)

        class D(BaseException):
            pass

        self.assertTrue(BaseException.__flags__ & Py_TPFLAGS_BASE_EXC_SUBCLASS)
        self.assertTrue(D.__flags__ & Py_TPFLAGS_BASE_EXC_SUBCLASS)
        self.assertTrue(MemoryError.__flags__ & Py_TPFLAGS_BASE_EXC_SUBCLASS)

    def test_dunder_flags_sets_type_subclass_if_type_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & Py_TPFLAGS_TYPE_SUBCLASS)

        class D(type):
            pass

        self.assertTrue(type.__flags__ & Py_TPFLAGS_TYPE_SUBCLASS)
        self.assertTrue(D.__flags__ & Py_TPFLAGS_TYPE_SUBCLASS)

    def test_dunder_init_with_no_name_or_object_param_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            type.__init__(type)

        self.assertIn("type.__init__() takes 1 or 3 arguments", str(context.exception))

    def test_dunder_init_with_too_many_args_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            type.__init__(type, "C", (), {}, 1, 2, 3, 4, 5, foobar=42)

        self.assertIn("type.__init__() takes 1 or 3 arguments", str(context.exception))

    def test_dunder_init_with_kwargs_does_not_raise(self):
        type.__init__(type, "C", (), {}, foobar=42)

    def test_dunder_instancecheck_with_instance_returns_true(self):
        self.assertIs(int.__instancecheck__(3), True)
        self.assertIs(int.__instancecheck__(False), True)
        self.assertIs(object.__instancecheck__(type), True)
        self.assertIs(str.__instancecheck__("123"), True)
        self.assertIs(type.__instancecheck__(type, int), True)
        self.assertIs(type.__instancecheck__(type, object), True)

    def test_dunder_instancecheck_with_non_instance_returns_false(self):
        self.assertIs(bool.__instancecheck__(3), False)
        self.assertIs(int.__instancecheck__("123"), False)
        self.assertIs(str.__instancecheck__(b"123"), False)
        self.assertIs(type.__instancecheck__(type, 3), False)

    def test_dunder_subclasscheck_with_subclass_returns_true(self):
        self.assertIs(int.__subclasscheck__(int), True)
        self.assertIs(int.__subclasscheck__(bool), True)
        self.assertIs(object.__subclasscheck__(int), True)
        self.assertIs(object.__subclasscheck__(type), True)

    def test_dunder_subclasscheck_with_non_subclass_returns_false(self):
        self.assertIs(bool.__subclasscheck__(int), False)
        self.assertIs(int.__subclasscheck__(object), False)
        self.assertIs(str.__subclasscheck__(object), False)
        self.assertIs(type.__subclasscheck__(type, object), False)

    def test_dunder_module_set_on_builtin_raises_type_error(self):
        with self.assertRaisesRegex(TypeError, ".*int.*"):
            int.__module__ = "foo"
        with self.assertRaisesRegex(TypeError, ".*int.*"):
            type.__dict__["__module__"].__set__(int, "foo")

    def test_dunder_module_sets_module(self):
        class C:
            pass

        type.__dict__["__module__"].__set__(C, "foo")
        self.assertEqual(type.__dict__["__module__"].__get__(C), "foo")
        self.assertEqual(C.__dict__["__module__"], "foo")
        self.assertEqual(C.__module__, "foo")

    def test_dunder_module_set_accepts_anything(self):
        class C:
            pass

        type.__dict__["__module__"].__set__(C, 42.42)
        self.assertEqual(C.__module__, 42.42)

        C.__module__ = None
        self.assertIsNone(type.__dict__["__module__"].__get__(C))

    def test_dunder_module_with_builtin_type_returns_builtins(self):
        self.assertEqual(int.__module__, "builtins")
        self.assertEqual(dict.__module__, "builtins")
        self.assertEqual(OSError.__module__, "builtins")

    def test_dunder_name_returns_name(self):
        class FooBar:
            pass

        self.assertEqual(FooBar.__name__, "FooBar")
        self.assertEqual(type.__dict__["__name__"].__get__(FooBar), "FooBar")

    def test_dunder_name_sets_name(self):
        class C:
            pass

        type.__dict__["__name__"].__set__(C, "foo")
        self.assertEqual(type.__dict__["__name__"].__get__(C), "foo")
        self.assertEqual(C.__name__, "foo")

    def test_dunder_name_set_with_non_string_raises_type_error(self):
        class C:
            pass

        with self.assertRaisesRegex(
            TypeError, "can only assign string to C.__name__, not 'int'"
        ):
            C.__name__ = 42
        with self.assertRaisesRegex(
            TypeError, "can only assign string to C.__name__, not 'float'"
        ):
            type.__dict__["__name__"].__set__(C, 1.2)

    def test_dunder_name_set_on_builtin_raises_type_error(self):
        with self.assertRaisesRegex(TypeError, ".*int.*"):
            int.__name__ = "foo"
        with self.assertRaisesRegex(TypeError, ".*int.*"):
            type.__dict__["__name__"].__set__(int, "foo")

    def test_dunder_new_with_one_arg_returns_type_of_arg(self):
        class C:
            pass

        self.assertIs(type.__new__(type, 1), int)
        self.assertIs(type.__new__(type, "hello"), str)
        self.assertIs(type.__new__(type, C()), C)

    def test_dunder_new_with_non_type_cls_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__new__(1, "X", (object,), {})

    def test_dunder_new_with_non_str_name_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__new__(type, 1, (object,), {})

    def test_dunder_new_with_non_tuple_bases_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__new__(type, "X", [object], {})

    def test_dunder_new_with_non_basetype_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, "type 'bool' is not an acceptable base type"
        ):
            type.__new__(type, "X", (bool,), {})

    def test_dunder_new_with_non_dict_type_dict_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__new__(type, "X", (object,), 1)

    def test_dunder_new_with_duplicated_base_raises_type_error(self):
        class C:
            pass

        error_msg = "duplicate base class C"
        with self.assertRaisesRegex(TypeError, error_msg):
            type.__new__(type, "X", (C, C), {})

    def test_dunder_new_returns_type_instance(self):
        X = type.__new__(type, "X", (object,), {})
        self.assertIsInstance(X, type)
        self.assertEqual(X.__name__, "X")
        self.assertEqual(X.__qualname__, "X")
        self.assertTrue(X.__flags__ & Py_TPFLAGS_HEAPTYPE)

    def test_dunder_new_sets_dunder_module(self):
        globals = {"__name__": 8.13}
        X = eval("type('X', (), {})", globals)  # noqa: P204
        self.assertEqual(X.__module__, 8.13)
        self.assertEqual(X.__dict__["__module__"], 8.13)

    def test_dunder_new_does_not_override_dunder_module(self):
        X = type.__new__(type, "X", (), {"__module__": "foobar"})
        self.assertEqual(X.__module__, "foobar")
        self.assertEqual(X.__dict__["__module__"], "foobar")

    def test_dunder_new_sets_qualname(self):
        X = type.__new__(type, "foo.bar", (), {})
        self.assertEqual(X.__qualname__, "foo.bar")
        self.assertNotIn("__qualname__", X.__dict__)

    def test_dunder_new_sets_qualname_from_dict(self):
        namespace = {"__qualname__": "quux"}
        X = type.__new__(type, "X", (), namespace)
        self.assertEqual(X.__qualname__, "quux")
        self.assertNotIn("__qualname__", X.__dict__)
        self.assertEqual(namespace["__qualname__"], "quux")

    def test_dunder_new_with_non_string_qualname_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, "type __qualname__ must be a str, not float"
        ):
            type.__new__(type, "X", (), {"__qualname__": 2.3})

    def test_dunder_new_adds_to_base_dunder_subclasses(self):
        A = type.__new__(type, "A", (object,), {})
        B = type.__new__(type, "B", (object,), {})
        C = type.__new__(type, "C", (A, B), {})
        D = type.__new__(type, "D", (A,), {})
        self.assertEqual(A.__subclasses__(), [C, D])
        self.assertEqual(B.__subclasses__(), [C])
        self.assertEqual(C.__subclasses__(), [])
        self.assertEqual(D.__subclasses__(), [])

    def test_dunder_qualname_returns_qualname(self):
        class C:
            __qualname__ = "bar"

        self.assertEqual(C.__qualname__, "bar")
        self.assertEqual(type.__dict__["__qualname__"].__get__(C), "bar")

    def test_dunder_qualname_sets_qualname(self):
        class C:
            pass

        type.__dict__["__qualname__"].__set__(C, "baz")
        self.assertEqual(C.__qualname__, "baz")
        C.__qualname__ = "bam"
        self.assertEqual(type.__dict__["__qualname__"].__get__(C), "bam")

    def test_dunder_qualname_set_with_non_string_raises_type_error(self):
        class C:
            pass

        with self.assertRaisesRegex(
            TypeError, "can only assign string to C.__qualname__, not 'int'"
        ):
            C.__qualname__ = 42
        with self.assertRaisesRegex(
            TypeError, "can only assign string to C.__qualname__, not 'float'"
        ):
            type.__dict__["__qualname__"].__set__(C, 1.2)

    def test_dunder_qualname_set_on_builtin_raises_type_error(self):
        with self.assertRaisesRegex(TypeError, ".*list.*"):
            list.__qualname__ = "foo"
        with self.assertRaisesRegex(TypeError, ".*list.*"):
            type.__dict__["__qualname__"].__set__(list, "foo")

    def test_dunder_repr_with_bytes_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__repr__(b"")

    def test_dunder_repr_returns_string_with_module_and_name(self):
        class C:
            pass

        self.assertEqual(
            type.__repr__(C),
            "<class '__main__.TypeTests.test_dunder_repr_returns_string_with_module_and_name.<locals>.C'>",
        )

    def test_dunder_repr_for_builtins_returns_string_with_only_name(self):
        self.assertEqual(type.__repr__(list), "<class 'list'>")
        self.assertEqual(type.__repr__(TypeError), "<class 'TypeError'>")

    def test_dunder_repr_for_imported_class_returns_string_with_module_and_name(self):
        self.assertEqual(type.__repr__(ast.NodeVisitor), "<class 'ast.NodeVisitor'>")
        self.assertEqual(
            type.__repr__(contextlib.ContextDecorator),
            "<class 'contextlib.ContextDecorator'>",
        )

    def test_dunder_subclasses_with_leaf_type_returns_empty_list(self):
        class C:
            pass

        self.assertEqual(C.__subclasses__(), [])

    def test_dunder_subclasses_with_supertype_returns_list(self):
        class C:
            pass

        class D(C):
            pass

        self.assertEqual(C.__subclasses__(), [D])

    def test_dunder_subclasses_returns_new_list(self):
        class C:
            pass

        self.assertIsNot(C.__subclasses__(), C.__subclasses__())

    @pyro_only
    def test_dunder_subclasses_does_not_return_dead_types(self):
        class C:
            pass

        class D(C):
            pass

        D = None  # noqa: F841
        _gc()
        self.assertEqual(type.__subclasses__(C), [])

    @pyro_only
    def test_dunder_subclasses_with_multiple_subclasses_returns_list(self):
        class B:
            pass

        class S0(B):
            pass

        class S1(B):
            pass

        class S2(B):
            pass

        class S3(B):
            pass

        class S4(B):
            pass

        S1 = None  # noqa: F841
        S3 = None  # noqa: F841
        _gc()
        self.assertEqual(len(type.__subclasses__(B)), 3)
        S0 = None  # noqa: F841
        _gc()
        self.assertEqual(type.__subclasses__(B), [S2, S4])

    def test_mro_returns_list(self):
        class C:
            pass

        mro = C.mro()
        self.assertIsInstance(mro, list)
        self.assertEqual(mro, [C, object])

    def test_mro_with_multiple_inheritance_returns_linearization(self):
        class A:
            pass

        class B:
            pass

        class C(A, B):
            pass

        mro = type.mro(C)
        self.assertIsInstance(mro, list)
        self.assertEqual(mro, [C, A, B, object])

    def test_mro_with_invalid_linearization_raises_type_error(self):
        class A:
            pass

        class B(A):
            pass

        with self.assertRaisesRegex(
            TypeError,
            r"Cannot create a consistent method resolution\s+order \(MRO\) for bases A, B",
        ):

            class C(A, B):
                pass

    def test_mro_with_custom_method_propagates_exception(self):
        class Meta(type):
            def mro(cls):
                raise KeyError

        with self.assertRaises(KeyError):

            class Foo(metaclass=Meta):
                pass

    def test_new_calculates_metaclass_and_calls_dunder_new(self):
        class M0(type):
            def __new__(metacls, name, bases, type_dict, **kwargs):
                metacls.new_args = (metacls, name, bases, kwargs)
                return type.__new__(metacls, name, bases, type_dict)

        class M1(M0):
            foo = 7

        class A(metaclass=M0):
            pass

        class B(metaclass=M1):
            pass

        C = type.__new__(type, "C", (A, B), {}, bar=8)
        self.assertIs(type(C), M1)
        self.assertEqual(C.new_args, (M1, "C", (A, B), {"bar": 8}))
        self.assertEqual(C.foo, 7)

    def test_new_with_mro_entries_base_raises_type_error(self):
        class X:
            def __mro_entries__(self, bases):
                return (object,)

        pseudo_base = X()
        with self.assertRaises(TypeError) as ctx:
            type.__new__(type, "C", (pseudo_base,), {})
        self.assertEqual(
            str(ctx.exception),
            "type() doesn't support MRO entry resolution; use types.new_class()",
        )

    def test_mro_returning_iterable_returns_class_with_mro_tuple(self):
        class A:
            foo = 42

        class X:
            def __init__(self, cls):
                self.cls = cls

            def __iter__(self):
                return iter((self.cls, A))

        class Meta(type):
            def mro(cls):
                return X(cls)

        C = Meta.__new__(Meta, "C", (object,), {})
        self.assertIs(type(C.__mro__), tuple)
        self.assertEqual(C.__mro__, (C, A))
        self.assertEqual(C.foo, 42)

    def test_new_calls_init_subclass(self):
        class Foo:
            def __init_subclass__(cls, *args, **kwargs):
                cls.called_init = True

                self.assertIsInstance(cls, type)
                self.assertIn("foo", kwargs)

        class Bar(Foo, foo=True):
            pass

        self.assertTrue(Bar.called_init)

    def test_new_duplicates_dict(self):
        d = {"foo": 42, "bar": 17}
        T = type("T", (object,), d)
        d["foo"] = -7
        del d["bar"]
        self.assertEqual(T.foo, 42)
        self.assertEqual(T.bar, 17)
        T.foo = 20
        self.assertEqual(d["foo"], -7)
        self.assertFalse("bar" in d)

    @pyro_only
    def test_non_heaptype_returns_non_heaptype(self):
        X = _non_heaptype("X", (), {})
        self.assertFalse(X.__flags__ & Py_TPFLAGS_HEAPTYPE)
        with self.assertRaises(TypeError):
            X.foo = 1

    def test_non_heaptype_has_no_module_attribute(self):
        from types import SimpleNamespace

        self.assertNotIn("__module__", SimpleNamespace.__dict__)
        self.assertNotIn("__module__", zip.__dict__)
        self.assertNotIn("__module__", map.__dict__)

    def test_setattr_with_metaclass_does_not_abort(self):
        class Meta(type):
            pass

        class C(metaclass=Meta):
            __slots__ = "attr"

            def __init__(self, data):
                self.attr = data

        m = C("foo")
        self.assertEqual(m.attr, "foo")
        m.attr = "bar"
        self.assertEqual(m.attr, "bar")

    def test_non_type_with_type_getattribute(self):
        class C:
            __getattribute__ = type.__getattribute__

        c = C()
        with self.assertRaises(TypeError):
            c.foo


class TypeProxyTests(unittest.TestCase):
    def setUp(self):
        class A:
            placeholder = "placeholder_value"

        class B(A):
            pass

        def make_placeholder():
            b = B()
            return b.placeholder

        self.tested_type = B
        self.type_proxy = B.__dict__
        self.assertEqual(make_placeholder(), "placeholder_value")

    def test_dunder_contains_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).__contains__(None, None)

    def test_dunder_contains_returns_true_for_existing_item(self):
        self.tested_type.x = 40
        self.assertTrue(self.type_proxy.__contains__("x"))

    def test_dunder_contains_returns_false_for_not_existing_item(self):
        self.assertFalse(self.type_proxy.__contains__("x"))

    def test_dunder_contains_returns_false_for_placeholder(self):
        self.assertFalse(self.type_proxy.__contains__("placeholder"))

    def test_copy_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).copy(None)

    def test_copy_returns_dict_copy(self):
        self.tested_type.x = 40
        result = self.type_proxy.copy()
        self.assertEqual(type(result), dict)
        self.assertEqual(result["x"], 40)
        self.tested_type.y = 50
        self.assertNotIn("y", result)

    def test_dunder_getitem_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).__getitem__(None, None)

    def test_dunder_getitem_for_existing_key_returns_that_item(self):
        self.tested_type.x = 40
        self.assertEqual(self.type_proxy.__getitem__("x"), 40)

    def test_dunder_getitem_for_not_existing_key_raises_key_error(self):
        with self.assertRaises(KeyError) as context:
            self.type_proxy.__getitem__("x")
        self.assertIn("'x'", str(context.exception))

    def test_dunder_getitem_for_placeholder_raises_key_error(self):
        with self.assertRaises(KeyError) as context:
            self.type_proxy.__getitem__("placeholder")
        self.assertIn("'placeholder'", str(context.exception))

    def test_dunder_iter_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).__iter__(None)

    def test_dunder_iter_returns_key_iterator(self):
        self.tested_type.x = 40
        self.tested_type.y = 50
        result = self.type_proxy.__iter__()
        self.assertTrue(hasattr(result, "__next__"))
        result_list = list(result)
        self.assertIn("x", result_list)
        self.assertIn("y", result_list)

    def test_dunder_len_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).__len__(None)

    def test_dunder_len_returns_num_items(self):
        length = self.type_proxy.__len__()
        self.tested_type.x = 40
        self.assertEqual(self.type_proxy.__len__(), length + 1)

    def test_dunder_len_returns_num_items_excluding_placeholder(self):
        length = self.type_proxy.__len__()
        # Overwrite the existing placeholder by creating a real one under the same name.
        self.tested_type.placeholder = 1
        self.assertEqual(self.type_proxy.__len__(), length + 1)

    def test_dunder_repr_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).__repr__(None)

    def test_dunder_repr_returns_str_containing_existing_items(self):
        self.tested_type.x = 40
        self.tested_type.y = 50
        result = self.type_proxy.__repr__()
        self.assertIsInstance(result, str)
        self.assertIn("'x': 40", result)
        self.assertIn("'y': 50", result)

    def test_dunder_repr_returns_str_not_containing_placeholder(self):
        result = self.type_proxy.__repr__()
        self.assertNotIn("'placeholder'", result)

    def test_get_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).get(None, None)

    def test_get_returns_existing_item_value(self):
        self.tested_type.x = 40
        self.assertEqual(self.type_proxy.get("x"), 40)

    def test_get_with_default_for_non_existing_item_value_returns_that_default(self):
        self.assertEqual(self.type_proxy.get("x", -1), -1)

    def test_get_for_non_existing_item_returns_none(self):
        self.assertIs(self.type_proxy.get("x"), None)

    def test_get_for_placeholder_returns_none(self):
        self.assertIs(self.type_proxy.get("placeholder"), None)

    def test_items_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).items(None)

    def test_items_returns_container_for_key_value_pairs(self):
        self.tested_type.x = 40
        self.tested_type.y = 50
        result = self.type_proxy.items()
        self.assertTrue(hasattr(result, "__iter__"))
        result_list = list(iter(result))
        self.assertIn(("x", 40), result_list)
        self.assertIn(("y", 50), result_list)

    def test_keys_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).keys(None)

    def test_keys_returns_container_for_keys(self):
        self.tested_type.x = 40
        self.tested_type.y = 50
        result = self.type_proxy.keys()
        self.assertTrue(hasattr(result, "__iter__"))
        result_list = list(iter(result))
        self.assertIn("x", result_list)
        self.assertIn("y", result_list)

    def test_keys_returns_key_iterator_excluding_placeholder(self):
        result = self.type_proxy.keys()
        self.assertNotIn("placeholder", result)

    def test_values_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).values(None)

    def test_values_returns_container_for_values(self):
        self.tested_type.x = 1243314135
        self.tested_type.y = -1243314135
        result = self.type_proxy.values()
        self.assertTrue(hasattr(result, "__iter__"))
        result_list = list(iter(result))
        self.assertIn(1243314135, result_list)
        self.assertIn(-1243314135, result_list)

    def test_values_returns_iterator_excluding_placeholder_value(self):
        result = self.type_proxy.values()
        self.assertNotIn("placeholder_value", result)


class VarsTests(unittest.TestCase):
    def test_no_arg_delegates_to_locals(self):
        def foo():
            a = 4
            a = a  # noqa: F841
            b = 5
            b = b  # noqa: F841
            return vars()

        result = foo()
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result), 2)
        self.assertEqual(result["a"], 4)
        self.assertEqual(result["b"], 5)

    def test_arg_with_no_dunder_dict_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            vars(None)
        self.assertEqual(
            str(context.exception), "vars() argument must have __dict__ attribute"
        )

    def test_arg_with_dunder_dict_raising_exception_raises_type_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise UserWarning("called descriptor")

        class C:
            __dict__ = Desc()

        c = C()
        with self.assertRaises(TypeError) as context:
            vars(c)
        self.assertEqual(
            str(context.exception), "vars() argument must have __dict__ attribute"
        )

    def test_arg_with_dunder_dict_returns_dunder_dict(self):
        class C:
            __dict__ = 4

        c = C()
        self.assertEqual(vars(c), 4)


@pyro_only
class UnderNumberCheckTests(unittest.TestCase):
    def test_number_check_with_builtin_number_returns_true(self):
        self.assertTrue(_number_check(2))
        self.assertTrue(_number_check(False))
        self.assertTrue(_number_check(5.0))

    def test_number_check_without_class_method_returns_false(self):
        class Foo:
            pass

        foo = Foo()
        foo.__float__ = lambda: 1.0
        foo.__int__ = lambda: 2
        self.assertFalse(_number_check(foo))

    def test_number_check_with_dunder_index_descriptor_does_not_call(self):
        class Raise:
            def __get__(self, obj, type):
                raise AttributeError("bad")

        class FloatLike:
            __float__ = Raise()

        class IntLike:
            __int__ = Raise()

        self.assertTrue(_number_check(FloatLike()))
        self.assertTrue(_number_check(IntLike()))


@unittest.skipIf(
    sys.implementation.name == "cpython" and sys.version_info < (3, 10),
    "requires at least CPython 3.10",
)
class UnionTests(unittest.TestCase):
    def test_or_type_operator(self):
        self.assertEqual(int | str, typing.Union[int, str])
        self.assertNotEqual(int | list, typing.Union[int, str])
        self.assertEqual(str | int, typing.Union[int, str])
        self.assertEqual(int | None, typing.Union[int, None])
        self.assertEqual(None | int, typing.Union[int, None])
        self.assertEqual(int | str | list, typing.Union[int, str, list])
        self.assertEqual(int | (str | list), typing.Union[int, str, list])
        self.assertEqual(str | (int | list), typing.Union[int, str, list])
        self.assertEqual(
            typing.List | typing.Tuple, typing.Union[typing.List, typing.Tuple]
        )
        self.assertEqual(
            typing.List[int] | typing.Tuple[int],
            typing.Union[typing.List[int], typing.Tuple[int]],
        )
        self.assertEqual(typing.List[int] | None, typing.Union[typing.List[int], None])
        self.assertEqual(None | typing.List[int], typing.Union[None, typing.List[int]])
        self.assertEqual(
            str | float | int | complex | int, (int | str) | (float | complex)
        )
        self.assertEqual(
            typing.Union[str, int, typing.List[int]], str | int | typing.List[int]
        )
        self.assertEqual(
            typing.Union[str, int, float, complex], (int | str) | (float | complex)
        )
        self.assertEqual(int | int, int)
        self.assertEqual(
            BaseException | bool | bytes | complex | float | int | list | map | set,
            typing.Union[
                BaseException,
                bool,
                bytes,
                complex,
                float,
                int,
                list,
                map,
                set,
            ],
        )
        with self.assertRaises(TypeError):
            int | 3
        with self.assertRaises(TypeError):
            3 | int
        with self.assertRaises(TypeError):
            Example() | int
        with self.assertRaises(TypeError):
            (int | str) < typing.Union[str, int]
        with self.assertRaises(TypeError):
            (int | str) < (int | bool)
        with self.assertRaises(TypeError):
            (int | str) <= (int | str)

    def test_or_type_operator_with_Alias(self):
        self.assertEqual(list | str, typing.Union[list, str])
        self.assertEqual(typing.List | str, typing.Union[typing.List, str])

    def test_or_type_operator_with_IO(self):
        self.assertEqual(typing.IO | str, typing.Union[typing.IO, str])

    def test_or_type_operator_with_NamedTuple(self):
        NT = namedtuple("A", ["B", "C", "D"])
        self.assertEqual(NT | str, typing.Union[NT, str])

    def test_or_type_operator_with_NewType(self):
        UserId = typing.NewType("UserId", int)
        self.assertEqual(UserId | str, typing.Union[UserId, str])

    def test_or_type_operator_with_SpecialForm(self):
        self.assertEqual(typing.Any | str, typing.Union[typing.Any, str])
        self.assertEqual(typing.NoReturn | str, typing.Union[typing.NoReturn, str])
        self.assertEqual(
            typing.Optional[int] | str, typing.Union[typing.Optional[int], str]
        )
        self.assertEqual(typing.Optional[int] | str, typing.Union[int, str, None])
        self.assertEqual(typing.Union[int, bool] | str, typing.Union[int, bool, str])
        self.assertNotEqual(str | int, typing.Tuple[str, int])

    def test_or_type_operator_with_TypeVar(self):
        TV = typing.TypeVar("T")
        self.assertEqual(TV | str, typing.Union[TV, str])
        self.assertEqual(str | TV, typing.Union[str, TV])

    def test_or_type_repr(self):
        self.assertEqual(repr(int | None), "int | None")

    def test_isinstance_with_or_union(self):
        self.assertTrue(isinstance(Super(), Super | int))
        self.assertFalse(isinstance(None, str | int))
        self.assertTrue(isinstance(3, str | int))
        self.assertTrue(isinstance("", str | int))
        self.assertTrue(isinstance(None, int | None))
        self.assertFalse(isinstance(3.14, int | str))

    def test_isinstance_raises_type_error(self):
        with self.assertRaises(TypeError):
            isinstance(2, list[int])
        with self.assertRaises(TypeError):
            isinstance(2, list[int] | int)
        with self.assertRaises(TypeError):
            isinstance(2, int | str | list[int] | float)

    def test_subclass_with_union(self):
        self.assertTrue(issubclass(int, int | float | int))
        self.assertTrue(issubclass(str, str | Child | str))
        self.assertFalse(issubclass(dict, float | str))
        self.assertFalse(issubclass(object, float | str))
        with self.assertRaises(TypeError):
            issubclass(2, Child | Super)
        with self.assertRaises(TypeError):
            issubclass(int, list[int] | Child)


class ZipTests(unittest.TestCase):
    def test_no_iterables_returns_stopped_iterator(self):
        self.assertTupleEqual(tuple(zip()), ())

    def test_one_iterable_returns_1_tuples(self):
        self.assertTupleEqual(tuple(zip(range(3))), ((0,), (1,), (2,)))

    def test_two_iterables_returns_2_tuples(self):
        self.assertTupleEqual(
            tuple(zip(range(0, 6, 2), range(1, 6, 2))), ((0, 1), (2, 3), (4, 5))
        )

    def test_two_iterables_returns_shortest(self):
        self.assertTupleEqual(tuple(zip(range(10), range(3))), ((0, 0), (1, 1), (2, 2)))
        self.assertTupleEqual(tuple(zip(range(3), range(10))), ((0, 0), (1, 1), (2, 2)))

    def test_three_iterables_returns_3_tuples(self):
        self.assertTupleEqual(
            tuple(zip(range(0, 10, 3), range(1, 10, 3), range(2, 10, 3))),
            ((0, 1, 2), (3, 4, 5), (6, 7, 8)),
        )

    def test_three_iterables_returns_shortest(self):
        self.assertTupleEqual(
            tuple(zip(range(3), range(10), range(4))), ((0, 0, 0), (1, 1, 1), (2, 2, 2))
        )
        self.assertTupleEqual(
            tuple(zip(range(3), range(4), range(10))), ((0, 0, 0), (1, 1, 1), (2, 2, 2))
        )
        self.assertTupleEqual(
            tuple(zip(range(10), range(3), range(4))), ((0, 0, 0), (1, 1, 1), (2, 2, 2))
        )
        self.assertTupleEqual(
            tuple(zip(range(10), range(4), range(3))), ((0, 0, 0), (1, 1, 1), (2, 2, 2))
        )
        self.assertTupleEqual(
            tuple(zip(range(4), range(3), range(10))), ((0, 0, 0), (1, 1, 1), (2, 2, 2))
        )
        self.assertTupleEqual(
            tuple(zip(range(4), range(10), range(3))), ((0, 0, 0), (1, 1, 1), (2, 2, 2))
        )


if __name__ == "__main__":
    unittest.main()
