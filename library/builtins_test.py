#!/usr/bin/env python3
import ast
import builtins
import contextlib
import sys
import types
import unittest
import warnings
from unittest.mock import Mock, call as mock_call

from test_support import pyro_only


try:
    from builtins import _number_check, instance_proxy
    from _builtins import _async_generator_op_iter_get_state, _gc
except ImportError:
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


class ByteArrayTests(unittest.TestCase):
    def test_dunder_contains_with_non_bytearray_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.__contains__("not_bytearray", 123)

    def test_dunder_contains_with_element_in_bytearray_returns_true(self):
        self.assertTrue(bytearray(b"abc").__contains__(ord("a")))

    def test_dunder_contains_with_element_not_in_bytearray_returns_false(self):
        self.assertFalse(bytearray(b"abc").__contains__(ord("z")))

    def test_dunder_contains_calls_dunder_index(self):
        class C:
            __index__ = Mock(name="__index__", return_value=ord("a"))

        c = C()
        self.assertTrue(bytearray(b"abc").__contains__(c))
        c.__index__.assert_called_once()

    def test_dunder_contains_calls_dunder_index_before_checking_byteslike(self):
        class C(bytes):
            __index__ = Mock(name="__index__", return_value=ord("a"))

        c = C(b"q")
        self.assertTrue(bytearray(b"abc").__contains__(c))
        c.__index__.assert_called_once()

    def test_dunder_contains_ignores_errors_from_dunder_index(self):
        class C:
            __index__ = Mock(name="__index__", side_effect=MemoryError("foo"))

        c = C()
        container = bytearray(b"abc")
        with self.assertRaises(TypeError):
            container.__contains__(c)
        c.__index__.assert_called_once()

    def test_delitem_with_out_of_bounds_index_raises_index_error(self):
        result = bytearray(b"")
        with self.assertRaises(IndexError) as context:
            del result[0]
        self.assertEqual(str(context.exception), "bytearray index out of range")

    def test_delitem_with_int_subclass_does_not_call_dunder_index(self):
        class C(int):
            def __index__(self):
                raise ValueError("foo")

        result = bytearray(b"01234")
        del result[C(0)]
        self.assertEqual(result, bytearray(b"1234"))

    def test_delitem_with_dunder_index_calls_dunder_index(self):
        class C:
            def __index__(self):
                return 2

        result = bytearray(b"01234")
        del result[C()]
        self.assertEqual(result, bytearray(b"0134"))

    def test_delslice_negative_indexes_removes_first_element(self):
        result = bytearray(b"01")
        del result[-2:-1]
        self.assertEqual(result, bytearray(b"1"))

    def test_delslice_negative_start_no_stop_removes_trailing_elements(self):
        result = bytearray(b"01")
        del result[-1:]
        self.assertEqual(result, bytearray(b"0"))

    def test_delslice_with_negative_step_deletes_every_other_element(self):
        result = bytearray(b"01234")
        del result[::-2]
        self.assertEqual(result, bytearray(b"13"))

    def test_delslice_with_start_and_negative_step_deletes_every_other_element(self):
        result = bytearray(b"01234")
        del result[1::-2]
        self.assertEqual(result, bytearray(b"0234"))

    def test_delslice_with_large_step_deletes_last_element(self):
        result = bytearray(b"01234")
        del result[4 :: 1 << 333]
        self.assertEqual(result, bytearray(b"0123"))

    def test_dunder_getitem_with_non_index_raises_type_error(self):
        ba = bytearray()
        with self.assertRaises(TypeError) as context:
            ba["not int or slice"]
        self.assertEqual(
            str(context.exception),
            "bytearray indices must be integers or slices, not str",
        )

    def test_dunder_getitem_with_large_int_raises_index_error(self):
        ba = bytearray()
        with self.assertRaises(IndexError) as context:
            ba[2 ** 63]
        self.assertEqual(
            str(context.exception), "cannot fit 'int' into an index-sized integer"
        )

    def test_dunder_getitem_positive_out_of_bounds_raises_index_error(self):
        ba = bytearray(b"foo")
        with self.assertRaises(IndexError) as context:
            ba[3]
        self.assertEqual(str(context.exception), "bytearray index out of range")

    def test_dunder_getitem_negative_out_of_bounds_raises_index_error(self):
        ba = bytearray(b"foo")
        with self.assertRaises(IndexError) as context:
            ba[-4]
        self.assertEqual(str(context.exception), "bytearray index out of range")

    def test_dunder_getitem_with_positive_int_returns_int(self):
        ba = bytearray(b"foo")
        self.assertEqual(ba[0], 102)
        self.assertEqual(ba[1], 111)
        self.assertEqual(ba[2], 111)

    def test_dunder_getitem_with_negative_int_returns_int(self):
        ba = bytearray(b"foo")
        self.assertEqual(ba[-3], 102)
        self.assertEqual(ba[-2], 111)
        self.assertEqual(ba[-1], 111)

    def test_dunder_getitem_with_int_subclass_returns_int(self):
        class N(int):
            pass

        ba = bytearray(b"foo")
        self.assertEqual(ba[N(0)], 102)
        self.assertEqual(ba[N(-2)], 111)

    def test_dunder_getitem_with_slice_returns_bytearray(self):
        ba = bytearray(b"hello world")
        result = ba[1:-1:3]
        self.assertIsInstance(result, bytearray)
        self.assertEqual(result, b"eoo")

    def test_dunder_hash_is_none(self):
        self.assertIs(bytearray.__hash__, None)

    def test_dunder_init_with_non_bytearray_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__init__' requires a 'bytearray' object but received a 'bytes'",
            bytearray.__init__,
            b"",
        )

    def test_dunder_init_no_args_clears_array(self):
        ba = bytearray(b"123")
        self.assertIsNone(ba.__init__())
        self.assertEqual(ba, b"")

    def test_dunder_init_with_encoding_without_source_raises_type_error(self):
        ba = bytearray.__new__(bytearray)
        with self.assertRaises(TypeError) as context:
            ba.__init__(encoding="utf-8")
        self.assertEqual(
            str(context.exception), "encoding or errors without sequence argument"
        )

    def test_dunder_init_with_errors_without_source_raises_type_error(self):
        ba = bytearray.__new__(bytearray)
        with self.assertRaises(TypeError) as context:
            ba.__init__(errors="strict")
        self.assertEqual(
            str(context.exception), "encoding or errors without sequence argument"
        )

    def test_dunder_init_with_str_without_encoding_raises_type_error(self):
        ba = bytearray.__new__(bytearray)
        with self.assertRaises(TypeError) as context:
            ba.__init__("")
        self.assertEqual(str(context.exception), "string argument without an encoding")

    def test_dunder_init_with_ascii_str_copies_bytes(self):
        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__("hello", "ascii"))
        self.assertEqual(ba, b"hello")

    def test_dunder_init_with_invalid_unicode_propagates_surrogate_error(self):
        ba = bytearray.__new__(bytearray)
        with self.assertRaises(UnicodeEncodeError) as context:
            ba.__init__("hello\uac80world", "ascii")
        self.assertIn("ascii", str(context.exception))

    def test_dunder_init_with_ignore_errors_copies_bytes(self):
        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__("hello\uac80world", "ascii", "ignore"))
        self.assertEqual(ba, b"helloworld")

    def test_dunder_init_with_non_str_and_encoding_raises_type_error(self):
        ba = bytearray.__new__(bytearray)
        with self.assertRaises(TypeError) as context:
            ba.__init__(0, encoding="utf-8")
        self.assertEqual(
            str(context.exception), "encoding or errors without a string argument"
        )

    def test_dunder_init_with_non_str_and_errors_raises_type_error(self):
        ba = bytearray.__new__(bytearray)
        with self.assertRaises(TypeError) as context:
            ba.__init__(0, errors="ignore")
        self.assertEqual(
            str(context.exception), "encoding or errors without a string argument"
        )

    def test_dunder_init_with_int_fills_with_null_bytes(self):
        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__(5))
        self.assertEqual(ba, b"\x00\x00\x00\x00\x00")

    def test_dunder_init_with_int_subclass_fills_with_null_bytes(self):
        class N(int):
            pass

        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__(N(3)))
        self.assertEqual(ba, b"\x00\x00\x00")

    def test_dunder_init_with_index_fills_with_null_bytes(self):
        class N:
            def __index__(self):
                return 3

        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__(N()))
        self.assertEqual(ba, b"\x00\x00\x00")

    def test_dunder_init_with_bytes_copies_bytes(self):
        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__(b"abc"))
        self.assertEqual(ba, b"abc")

    def test_dunder_init_with_other_bytearray_copies_bytes(self):
        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__(bytearray(b"abc")))
        self.assertEqual(ba, b"abc")

    def test_dunder_init_with_self_empties(self):
        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__(ba))
        self.assertEqual(ba, b"")

    def test_dunder_init_with_iterable_copies_bytes(self):
        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__([100, 101, 102]))
        self.assertEqual(ba, b"def")

    def test_dunder_setitem_with_non_bytearray_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.__setitem__(None, 1, 5)

    def test_dunder_setitem_with_non_int_value_raises_type_error(self):
        with self.assertRaises(TypeError):
            ba = bytearray(b"foo")
            bytearray.__setitem__(ba, 1, b"x")

    def test_dunder_setitem_with_int_key_sets_value_at_index(self):
        ba = bytearray(b"foo")
        bytearray.__setitem__(ba, 1, 1)
        self.assertEqual(ba, bytearray(b"f\01o"))

    def test_dunder_setitem_calls_dunder_index(self):
        class C:
            def __index__(self):
                raise UserWarning("foo")

        ba = bytearray(b"foo")
        with self.assertRaises(UserWarning):
            bytearray.__setitem__(ba, C(), 1)

    def test_dunder_setitem_slice_with_subclass_does_not_call_dunder_iter(self):
        class C(bytearray):
            def __iter__(self):
                raise UserWarning("foo")

            def __buffer__(self):
                raise UserWarning("bar")

        result = bytearray(b"foo")
        other = C(b"abc")
        result[1:] = other
        self.assertEqual(result, bytearray(b"fabc"))

    def test_dunder_setitem_slice_with_iterable_calls_dunder_iter(self):
        class C:
            def __iter__(self):
                raise UserWarning("foo")

        ba = bytearray(b"foo")
        it = C()
        with self.assertRaises(UserWarning):
            ba[1:] = it

    def test_dunder_setitem_slice_basic(self):
        result = bytearray(b"abcdefg")
        result[2:5] = b"CDE"
        self.assertEqual(result, bytearray(b"abCDEfg"))

    def test_dunder_setitem_slice_grow(self):
        result = bytearray(b"abcdefg")
        result[2:5] = b"CDEXYZ"
        self.assertEqual(result, bytearray(b"abCDEXYZfg"))

    def test_dunder_setitem_slice_shrink(self):
        result = bytearray(b"abcdefg")
        result[2:6] = b"CD"
        self.assertEqual(result, bytearray(b"abCDg"))

    def test_dunder_setitem_slice_iterable(self):
        result = bytearray(b"abcdefg")
        result[2:6] = (ord("x"), ord("y"), ord("z")).__iter__()
        self.assertEqual(result, bytearray(b"abxyzg"))

    def test_dunder_setitem_slice_self(self):
        result = bytearray(b"abcdefg")
        result[2:5] = result
        self.assertEqual(result, bytearray(b"ababcdefgfg"))

    def test_dunder_setitem_slice_rev_bounds(self):
        # Reverse ordered bounds, but step still +1
        result = bytearray(b"1234567890")
        result[5:2] = b"abcde"
        self.assertEqual(result, bytearray(b"12345abcde67890"))

    def test_dunder_setitem_slice_step(self):
        result = bytearray(b"0123456789xxxx")
        result[2:10:3] = b"abc"
        self.assertEqual(result, bytearray(b"01a34b67c9xxxx"))

    def test_dunder_setitem_slice_step_neg(self):
        result = bytearray(b"0123456789xxxxx")
        result[10:2:-3] = b"abc"
        self.assertEqual(result, bytearray(b"0123c56b89axxxx"))

    def test_dunder_setitem_slice_tuple(self):
        result = bytearray(b"0123456789xxxx")
        result[2:10:3] = (ord("a"), ord("b"), ord("c"))
        self.assertEqual(result, bytearray(b"01a34b67c9xxxx"))

    def test_dunder_setitem_slice_short_stop(self):
        result = bytearray(b"1234567890")
        result[:1] = b"000"
        self.assertEqual(result, bytearray(b"000234567890"))

    def test_dunder_setitem_slice_long_stop(self):
        result = bytearray(b"111")
        result[:1] = b"00000"
        self.assertEqual(result, bytearray(b"0000011"))

    def test_dunder_setitem_slice_short_step(self):
        result = bytearray(b"1234567890")
        result[::1] = b"000"
        self.assertEqual(result, bytearray(b"000"))

    def test_dunder_setitem_slice_appends_to_end(self):
        result = bytearray(b"abc")
        result[3:] = b"000"
        self.assertEqual(result, bytearray(b"abc000"))

    def test_dunder_setitem_slice_with_rhs_bigger_than_slice_length_raises_value_error(
        self,
    ):
        result = bytearray(b"0123456789xxxx")
        with self.assertRaises(ValueError) as context:
            result[2:10:3] = b"abcd"

        self.assertEqual(
            str(context.exception),
            "attempt to assign bytes of size 4 to extended slice of size 3",
        )

    def test_dunder_setitem_slice_with_rhs_shorter_than_slice_length_raises_value_error(
        self,
    ):
        result = bytearray(b"0123456789")
        with self.assertRaises(ValueError) as context:
            result[:8:2] = b"000"

        self.assertEqual(
            str(context.exception),
            "attempt to assign bytes of size 3 to extended slice of size 4",
        )

    def test_dunder_setitem_slice_with_non_iterable_rhs_raises_type_error(self):
        result = bytearray(b"abcdefg")
        with self.assertRaises(TypeError) as context:
            result[2:6] = 5

        self.assertEqual(
            str(context.exception),
            "can assign only bytes, buffers, or iterables of ints in range(0, 256)",
        )

    def test_append_appends_item_to_bytearray(self):
        result = bytearray()
        result.append(ord("a"))
        self.assertEqual(result, bytearray(b"a"))

    def test_append_with_index_calls_dunder_index(self):
        class Idx:
            def __index__(self):
                return ord("q")

        result = bytearray()
        result.append(Idx())
        self.assertEqual(result, bytearray(b"q"))

    def test_append_with_string_raises_type_error(self):
        result = bytearray()
        with self.assertRaises(TypeError) as context:
            result.append("a")
        self.assertEqual(
            "'str' object cannot be interpreted as an integer", str(context.exception)
        )

    def test_append_with_large_int_raises_value_error(self):
        result = bytearray()
        with self.assertRaises(ValueError) as context:
            result.append(256)
        self.assertEqual("byte must be in range(0, 256)", str(context.exception))

    def test_append_with_negative_int_raises_value_error(self):
        result = bytearray()
        with self.assertRaises(ValueError) as context:
            result.append(-10)
        self.assertEqual("byte must be in range(0, 256)", str(context.exception))

    def test_clear_with_non_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray.clear(b"")
        self.assertIn(
            "'clear' requires a 'bytearray' object but received a 'bytes'",
            str(context.exception),
        )

    def test_copy_with_non_bytearray_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray.copy(b"")
        self.assertIn(
            "'copy' requires a 'bytearray' object but received a 'bytes'",
            str(context.exception),
        )

    def test_copy_returns_new_object(self):
        array = bytearray(b"123")
        copy = array.copy()
        self.assertIsInstance(copy, bytearray)
        self.assertIsNot(copy, array)
        self.assertEqual(array, copy)

    def test_count_with_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray.count(b"", bytearray())
        self.assertIn(
            "'count' requires a 'bytearray' object but received a 'bytes'",
            str(context.exception),
        )

    def test_count_with_nonbyte_int_raises_value_error(self):
        haystack = bytearray()
        needle = 266
        with self.assertRaises(ValueError) as context:
            haystack.count(needle)
        self.assertEqual(str(context.exception), "byte must be in range(0, 256)")

    # TODO(T65863013): Make bytearray.count behavior match CPython and remove
    # this skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_count_with_string_raises_type_error(self):
        haystack = bytearray()
        needle = "133"
        with self.assertRaises(TypeError) as context:
            haystack.count(needle)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    # TODO(T65863013): Make bytearray.count behavior match CPython and remove
    # this skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_count_with_non_number_index_raises_type_error(self):
        class Idx:
            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        with self.assertRaises(TypeError) as context:
            haystack.count(needle)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'Idx'"
        )

    def test_count_with_dunder_int_calls_dunder_index(self):
        class Desc:
            def __get__(self, obj, type):
                raise NotImplementedError("called descriptor")

        class Idx:
            def __index__(self):
                return ord("a")

            __int__ = Desc()

        haystack = bytearray(b"abc")
        needle = Idx()
        self.assertEqual(haystack.count(needle), 1)

    def test_count_with_dunder_float_calls_dunder_index(self):
        class Desc:
            def __get__(self, obj, type):
                raise NotImplementedError("called descriptor")

        class Idx:
            __float__ = Desc()

            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        self.assertEqual(haystack.count(needle), 1)

    # TODO(T65863013): Make bytearray.count behavior match CPython and remove
    # this skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_count_with_index_overflow_raises_value_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise OverflowError("not a byte!")

        haystack = bytearray(b"abc")
        needle = Idx()
        with self.assertRaises(ValueError) as context:
            haystack.count(needle)
        self.assertEqual(str(context.exception), "byte must be in range(0, 256)")

    # TODO(T65863013): Make bytearray.count behavior match CPython and remove
    # this skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_count_with_dunder_index_error_raises_type_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise MemoryError("not a byte!")

        haystack = bytearray(b"abc")
        needle = Idx()
        with self.assertRaises(TypeError) as context:
            haystack.count(needle)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'Idx'"
        )

    def test_count_with_empty_sub_returns_length_minus_adjusted_start_plus_one(self):
        haystack = bytearray(b"abcde")
        needle = b""
        self.assertEqual(haystack.count(needle, -3), 4)

    def test_count_with_missing_returns_zero(self):
        haystack = bytearray(b"abc")
        needle = b"d"
        self.assertEqual(haystack.count(needle), 0)

    def test_count_with_missing_stays_within_bounds(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"c")
        self.assertEqual(haystack.count(needle, None, 2), 0)

    def test_count_with_large_start_returns_zero(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"")
        self.assertEqual(haystack.count(needle, 10), 0)

    def test_count_with_negative_bounds_returns_count(self):
        haystack = bytearray(b"ababa")
        needle = b"a"
        self.assertEqual(haystack.count(needle, -4, -1), 1)

    def test_count_returns_non_overlapping_count(self):
        haystack = bytearray(b"0000000000")
        needle = bytearray(b"000")
        self.assertEqual(haystack.count(needle), 3)

    def test_endswith_with_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray.endswith(b"", bytearray())
        self.assertIn(
            "'endswith' requires a 'bytearray' object but received a 'bytes'",
            str(context.exception),
        )

    def test_endswith_with_list_other_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().endswith([])
        self.assertEqual(
            str(context.exception),
            "endswith first arg must be bytes or a tuple of bytes, not list",
        )

    def test_endswith_with_tuple_other_checks_each(self):
        haystack = bytearray(b"123")
        needle1 = (b"12", b"13", b"23", b"d")
        needle2 = (b"2", b"asd", b"122222")
        self.assertTrue(haystack.endswith(needle1))
        self.assertFalse(haystack.endswith(needle2))

    def test_endswith_with_end_searches_from_end(self):
        haystack = bytearray(b"12345")
        needle1 = bytearray(b"1")
        needle4 = b"34"
        self.assertFalse(haystack.endswith(needle1, 0))
        self.assertFalse(haystack.endswith(needle4, 1))
        self.assertTrue(haystack.endswith(needle1, 0, 1))
        self.assertTrue(haystack.endswith(needle4, 1, 4))

    def test_endswith_with_empty_returns_true_for_valid_bounds(self):
        haystack = bytearray(b"12345")
        self.assertTrue(haystack.endswith(bytearray()))
        self.assertTrue(haystack.endswith(b"", 5))
        self.assertTrue(haystack.endswith(bytearray(), -9, 1))

    def test_endswith_with_empty_returns_false_for_invalid_bounds(self):
        haystack = bytearray(b"12345")
        self.assertFalse(haystack.endswith(b"", 3, 2))
        self.assertFalse(haystack.endswith(bytearray(), 6))

    def test_extend_with_self_copies_data(self):
        array = bytearray(b"hello")
        self.assertIs(array.extend(array), None)
        self.assertEqual(array, b"hellohello")

    def test_extend_empty_with_bytes(self):
        array = bytearray(b"")
        self.assertIs(array.extend(b"hello"), None)
        self.assertEqual(array, b"hello")

    def test_extend_with_tuple_appends_to_end(self):
        array = bytearray(b"foo")
        self.assertIs(array.extend((42, 42, 42)), None)
        self.assertEqual(array, b"foo***")

    def test_extend_with_empty_string_is_noop(self):
        array = bytearray(b"foo")
        self.assertIs(array.extend(""), None)
        self.assertEqual(array, b"foo")

    def test_extend_with_nonempty_string_raises_type_error(self):
        array = bytearray(b"foo")
        self.assertRaises(TypeError, array.extend, "bar")

    def test_extend_with_non_int_raises_value_error(self):
        array = bytearray(b"foo")
        self.assertRaises(ValueError, array.extend, [256])
        self.assertRaises(ValueError, array.extend, (-1,))

    def test_extend_with_iterable_appends_to_end(self):
        duck = b"duck "
        goose = b"goose"
        array = bytearray()
        array.extend(map(int, duck * 2))
        array.extend(map(int, goose))
        self.assertEqual(array, b"duck duck goose")

    def test_find_with_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.find(b"", bytearray())

    def test_find_with_empty_sub_returns_start(self):
        haystack = bytearray(b"abc")
        needle = b""
        self.assertEqual(haystack.find(needle, 1), 1)

    def test_find_with_missing_returns_negative(self):
        haystack = bytearray(b"abc")
        needle = b"d"
        self.assertEqual(haystack.find(needle), -1)

    def test_find_with_missing_stays_within_bounds(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"c")
        self.assertEqual(haystack.find(needle, None, 2), -1)

    def test_find_with_large_start_returns_negative(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"")
        self.assertEqual(haystack.find(needle, 10), -1)

    def test_find_with_negative_bounds_returns_index(self):
        haystack = bytearray(b"ababa")
        needle = b"a"
        self.assertEqual(haystack.find(needle, -4, -1), 2)

    def test_find_with_multiple_matches_returns_first_index_in_range(self):
        haystack = bytearray(b"abbabbabba")
        needle = bytearray(b"abb")
        self.assertEqual(haystack.find(needle, 1), 3)

    def test_find_with_nonbyte_int_raises_value_error(self):
        haystack = bytearray()
        needle = 266
        with self.assertRaises(ValueError):
            haystack.find(needle)

    def test_find_with_string_raises_type_error(self):
        haystack = bytearray()
        needle = "133"
        with self.assertRaises(TypeError):
            haystack.find(needle)

    # TODO(T65863013): Make bytearray.find behavior match CPython and remove
    # this skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_find_with_non_number_index_raises_type_error(self):
        class Idx:
            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        with self.assertRaises(TypeError):
            haystack.find(needle)

    def test_find_with_dunder_int_calls_dunder_index(self):
        class Desc:
            def __get__(self, obj, type):
                raise NotImplementedError("called descriptor")

        class Idx:
            def __index__(self):
                return ord("a")

            __int__ = Desc()

        haystack = bytearray(b"abc")
        needle = Idx()
        self.assertEqual(haystack.find(needle), 0)

    def test_find_with_dunder_float_calls_dunder_index(self):
        class Desc:
            def __get__(self, obj, type):
                raise NotImplementedError("called descriptor")

        class Idx:
            __float__ = Desc()

            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        self.assertEqual(haystack.find(needle), 0)

    # TODO(T65863013): Make bytearray.find behavior match CPython and remove
    # this skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_find_with_index_overflow_raises_value_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise OverflowError("not a byte!")

        haystack = bytearray(b"abc")
        needle = Idx()
        with self.assertRaises(ValueError) as context:
            haystack.find(needle)
        self.assertEqual(str(context.exception), "byte must be in range(0, 256)")

    # TODO(T65863013): Make bytearray.find behavior match CPython and remove
    # this skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_find_with_dunder_index_error_raises_type_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise MemoryError("not a byte!")

        haystack = bytearray(b"abc")
        needle = Idx()
        with self.assertRaises(TypeError) as context:
            haystack.find(needle)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'Idx'"
        )

    def test_index_with_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.index(b"", bytearray())

    def test_index_with_subsequence_returns_first_in_range(self):
        haystack = bytearray(b"-a---a-aa")
        needle = ord("a")
        self.assertEqual(haystack.index(needle, 3), 5)

    def test_index_with_missing_raises_value_error(self):
        haystack = bytearray(b"abc")
        needle = b"d"
        with self.assertRaises(ValueError) as context:
            haystack.index(needle)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_index_outside_of_bounds_raises_value_error(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"c")
        with self.assertRaises(ValueError) as context:
            haystack.index(needle, 0, 2)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_join_with_non_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray.join(b"", [])
        self.assertIn(
            "'join' requires a 'bytearray' object but received a 'bytes'",
            str(context.exception),
        )

    def test_ljust_without_growth_returns_copy(self):
        foo = bytearray(b"foo")
        self.assertEqual(foo.ljust(-1), foo)
        self.assertEqual(foo.ljust(0), foo)
        self.assertEqual(foo.ljust(1), foo)
        self.assertEqual(foo.ljust(2), foo)
        self.assertEqual(foo.ljust(3), foo)

        self.assertIsNot(foo.ljust(-1), foo)
        self.assertIsNot(foo.ljust(0), foo)
        self.assertIsNot(foo.ljust(1), foo)
        self.assertIsNot(foo.ljust(2), foo)
        self.assertIsNot(foo.ljust(3), foo)

    def test_ljust_with_custom_fillchar_returns_bytearray(self):
        orig = bytearray(b"ba")
        filled = bytearray(b"ba").ljust(7, b"@")
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"ba@@@@@")
        self.assertEqual(orig, b"ba")

    def test_ljust_pads_end_of_array(self):
        self.assertEqual(bytearray(b"abc").ljust(4), b"abc ")
        self.assertEqual(bytearray(b"abc").ljust(7), b"abc    ")

    def test_ljust_with_bytearray_fillchar_returns_bytearray(self):
        orig = bytearray(b"ba")
        fillchar = bytearray(b"@")
        filled = bytearray(b"ba").ljust(7, fillchar)
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"ba@@@@@")
        self.assertEqual(orig, b"ba")

    def test_ljust_with_bytes_subclass_fillchar_returns_bytearray(self):
        class C(bytes):
            # access to character is done in native, so this is not called
            def __getitem__(self, key):
                return 0

        orig = bytearray(b"ba")
        fillchar = C(b"@")
        filled = bytearray(b"ba").ljust(7, fillchar)
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"ba@@@@@")
        self.assertEqual(orig, b"ba")

    def test_ljust_with_bytearray_subclass_fillchar_returns_bytearray(self):
        class C(bytearray):
            # access to character is done in native, so this is not called
            def __getitem__(self, key):
                return 0

        orig = bytearray(b"ba")
        fillchar = C(b"@")
        filled = bytearray(b"ba").ljust(7, fillchar)
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"ba@@@@@")
        self.assertEqual(orig, b"ba")

    def test_ljust_with_dunder_index_returns_bytearray(self):
        class C:
            def __index__(self):
                return 5

        orig = bytearray(b"ba")
        filled = bytearray(b"ba").ljust(C(), b"@")
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"ba@@@")
        self.assertEqual(orig, b"ba")

    def test_ljust_with_wrong_type_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().ljust(2, ord(" "))
        self.assertEqual(
            str(context.exception),
            "ljust() argument 2 must be a byte string of length 1, not int",
        )

    def test_ljust_with_wrong_length_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().ljust(2, b",,")
        self.assertEqual(
            str(context.exception),
            "ljust() argument 2 must be a byte string of length 1, not bytes",
        )

    def test_lower_with_non_bytearray_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.lower("not a bytearray")

    def test_lower_empty_self_returns_new_bytearray(self):
        src = bytearray(b"")
        dst = src.lower()
        self.assertIsNot(src, dst)
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, src)

    def test_lower_all_lower_returns_new_bytearray(self):
        src = bytearray(b"abcdefghijklmnopqrstuvwxyz1234567890")
        dst = src.lower()
        self.assertIsNot(src, dst)
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, src)

    def test_lower_all_lower_and_non_alphanumeric_returns_new_bytearray(self):
        src = bytearray(b"abcdefghijklmnopqrstuvwxyz1234567890")
        dst = src.lower()
        self.assertIsNot(src, dst)
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, src)

    def test_lower_all_uppercase_returns_all_lowercase(self):
        src = bytearray(b"ABCDEFGHIJKLMNOPQRSTUVWXYZ")
        dst = src.lower()
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, bytearray(b"abcdefghijklmnopqrstuvwxyz"))

    def test_lower_mixed_case_returns_all_lowercase(self):
        src = bytearray(b"aBcDeFgHiJkLmNoPqRsTuVwXyZ")
        dst = src.lower()
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, bytearray(b"abcdefghijklmnopqrstuvwxyz"))

    def test_lower_mixed_case_returns_all_lowercase(self):
        src = bytearray(b"a1!B2@c3#D4$e5%F6^g7&H8*i9(J0)")
        dst = src.lower()
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, bytearray(b"a1!b2@c3#d4$e5%f6^g7&h8*i9(j0)"))

    def test_lstrip_with_non_byteslike_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().lstrip("")
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_lstrip_with_none_or_default_strips_ascii_space(self):
        self.assertEqual(bytearray().lstrip(None), b"")
        self.assertEqual(bytearray(b"    ").lstrip(None), b"")
        self.assertEqual(bytearray(b"  hi  ").lstrip(), b"hi  ")

    def test_lstrip_with_byteslike_strips_bytes(self):
        self.assertEqual(bytearray().lstrip(b"123"), b"")
        self.assertEqual(bytearray(b"1aaa1").lstrip(bytearray()), b"1aaa1")
        self.assertEqual(bytearray(b"1 aaa1").lstrip(bytearray(b" 1")), b"aaa1")
        self.assertEqual(bytearray(b"hello").lstrip(b"ho"), b"ello")

    def test_lstrip_noop_returns_new_bytearray(self):
        arr = bytearray(b"abcd")
        result = arr.lstrip(b"e")
        self.assertIsInstance(result, bytearray)
        self.assertEqual(result, arr)
        self.assertIsNot(result, arr)

    def test_rfind_with_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray.rfind(b"", bytearray())
        self.assertIn(
            "'rfind' requires a 'bytearray' object but received a 'bytes'",
            str(context.exception),
        )

    def test_rfind_with_empty_sub_returns_end(self):
        haystack = bytearray(b"abc")
        needle = b""
        self.assertEqual(haystack.rfind(needle, 0, 2), 2)

    def test_rfind_with_missing_returns_negative(self):
        haystack = bytearray(b"abc")
        needle = b"d"
        self.assertEqual(haystack.rfind(needle), -1)

    def test_rfind_with_missing_stays_within_bounds(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"c")
        self.assertEqual(haystack.rfind(needle, None, 2), -1)

    def test_rfind_with_large_start_returns_negative(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"")
        self.assertEqual(haystack.rfind(needle, 10), -1)

    def test_rfind_with_negative_bounds_returns_index(self):
        haystack = bytearray(b"ababa")
        needle = b"a"
        self.assertEqual(haystack.rfind(needle, -4, -1), 2)

    def test_rfind_with_multiple_matches_returns_last_index_in_range(self):
        haystack = bytearray(b"abbabbabba")
        needle = bytearray(b"abb")
        self.assertEqual(haystack.rfind(needle, 0, 7), 3)

    def test_rfind_with_nonbyte_int_raises_value_error(self):
        haystack = bytearray()
        needle = 266
        with self.assertRaises(ValueError) as context:
            haystack.rfind(needle)
        self.assertEqual(str(context.exception), "byte must be in range(0, 256)")

    # TODO(T65863013): Make bytearray.rfind behavior match CPython and remove
    # this skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_rfind_with_string_raises_type_error(self):
        haystack = bytearray()
        needle = "133"
        with self.assertRaises(TypeError) as context:
            haystack.rfind(needle)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    # TODO(T65863013): Make bytearray.rfind behavior match CPython and remove
    # this skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_rfind_with_non_number_index_raises_type_error(self):
        class Idx:
            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        with self.assertRaises(TypeError) as context:
            haystack.rfind(needle)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'Idx'"
        )

    def test_rfind_with_dunder_int_calls_dunder_index(self):
        class Idx:
            def __int__(self):
                raise NotImplementedError("called __int__")

            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        self.assertEqual(haystack.rfind(needle), 0)

    def test_rfind_with_dunder_float_calls_dunder_index(self):
        class Idx:
            def __float__(self):
                raise NotImplementedError("called __float__")

            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        self.assertEqual(haystack.rfind(needle), 0)

    def test_rindex_with_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.rindex(b"", bytearray())

    def test_rindex_with_subsequence_returns_last_in_range(self):
        haystack = bytearray(b"-a-a--a--")
        needle = ord("a")
        self.assertEqual(haystack.rindex(needle, 1, 6), 3)

    def test_rindex_with_missing_raises_value_error(self):
        haystack = bytearray(b"abc")
        needle = b"d"
        with self.assertRaises(ValueError) as context:
            haystack.rindex(needle)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_rindex_outside_of_bounds_raises_value_error(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"c")
        with self.assertRaises(ValueError) as context:
            haystack.rindex(needle, 0, 2)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_rjust_without_growth_returns_copy(self):
        foo = bytearray(b"foo")
        self.assertEqual(foo.rjust(-1), foo)
        self.assertEqual(foo.rjust(0), foo)
        self.assertEqual(foo.rjust(1), foo)
        self.assertEqual(foo.rjust(2), foo)
        self.assertEqual(foo.rjust(3), foo)

        self.assertIsNot(foo.rjust(-1), foo)
        self.assertIsNot(foo.rjust(0), foo)
        self.assertIsNot(foo.rjust(1), foo)
        self.assertIsNot(foo.rjust(2), foo)
        self.assertIsNot(foo.rjust(3), foo)

    def test_rjust_with_custom_fillchar_returns_bytearray(self):
        orig = bytearray(b"ba")
        filled = bytearray(b"ba").rjust(7, b"@")
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"@@@@@ba")
        self.assertEqual(orig, b"ba")

    def test_rjust_pads_beginning_of_array(self):
        self.assertEqual(bytearray(b"abc").rjust(4), b" abc")
        self.assertEqual(bytearray(b"abc").rjust(7), b"    abc")

    def test_rjust_with_bytearray_fillchar_returns_bytearray(self):
        orig = bytearray(b"ba")
        fillchar = bytearray(b"@")
        filled = bytearray(b"ba").rjust(7, fillchar)
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"@@@@@ba")
        self.assertEqual(orig, b"ba")

    def test_rjust_with_bytes_subclass_fillchar_returns_bytearray(self):
        class C(bytes):
            # access to character is done in native, so this is not called
            def __getitem__(self, key):
                return 0

        orig = bytearray(b"ba")
        fillchar = C(b"@")
        filled = bytearray(b"ba").rjust(7, fillchar)
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"@@@@@ba")
        self.assertEqual(orig, b"ba")

    def test_rjust_with_bytearray_subclass_fillchar_returns_bytearray(self):
        class C(bytearray):
            # access to character is done in native, so this is not called
            def __getitem__(self, key):
                return 0

        orig = bytearray(b"ba")
        fillchar = C(b"@")
        filled = bytearray(b"ba").rjust(7, fillchar)
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"@@@@@ba")
        self.assertEqual(orig, b"ba")

    def test_rjust_with_dunder_index_returns_bytearray(self):
        class C:
            def __index__(self):
                return 5

        orig = bytearray(b"ba")
        filled = bytearray(b"ba").rjust(C(), b"@")
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"@@@ba")
        self.assertEqual(orig, b"ba")

    def test_rjust_with_wrong_type_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().rjust(2, ord(" "))
        self.assertEqual(
            str(context.exception),
            "rjust() argument 2 must be a byte string of length 1, not int",
        )

    def test_rjust_with_wrong_length_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().rjust(2, b",,")
        self.assertEqual(
            str(context.exception),
            "rjust() argument 2 must be a byte string of length 1, not bytes",
        )

    def test_rstrip_with_non_byteslike_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().rstrip("")
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_rstrip_with_none_or_default_strips_ascii_space(self):
        self.assertEqual(bytearray().rstrip(None), b"")
        self.assertEqual(bytearray(b"    ").rstrip(None), b"")
        self.assertEqual(bytearray(b"  hi  ").rstrip(), b"  hi")

    def test_rstrip_with_byteslike_strips_bytes(self):
        self.assertEqual(bytearray().rstrip(b"123"), b"")
        self.assertEqual(bytearray(b"1aaa1").rstrip(bytearray()), b"1aaa1")
        self.assertEqual(bytearray(b"1 aaa1").rstrip(bytearray(b" 1")), b"1 aaa")
        self.assertEqual(bytearray(b"hello").rstrip(b"lo"), b"he")

    def test_rstrip_noop_returns_new_bytearray(self):
        arr = bytearray(b"abcd")
        result = arr.rstrip(b"e")
        self.assertIsInstance(result, bytearray)
        self.assertEqual(result, arr)
        self.assertIsNot(result, arr)

    def test_split_with_non_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray.split(b"foo bar")
        self.assertIn(
            "'split' requires a 'bytearray' object but received a 'bytes'",
            str(context.exception),
        )

    def test_split_with_non_byteslike_sep_raises_type_error(self):
        b = bytearray(b"foo bar")
        sep = ""
        with self.assertRaises(TypeError) as context:
            b.split(sep)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_split_with_non_index_maxsplit_raises_type_error(self):
        b = bytearray(b"foo bar")
        with self.assertRaises(TypeError) as context:
            b.split(maxsplit=None)
        self.assertEqual(
            str(context.exception),
            "'NoneType' object cannot be interpreted as an integer",
        )

    def test_split_with_large_int_raises_overflow_error(self):
        b = bytearray(b"foo bar")
        with self.assertRaises(OverflowError) as context:
            b.split(maxsplit=2 ** 64)
        self.assertEqual(
            str(context.exception), "Python int too large to convert to C ssize_t"
        )

    def test_split_with_empty_sep_raises_value_error(self):
        b = bytearray(b"foo bar")
        with self.assertRaises(ValueError) as context:
            b.split(b"")
        self.assertEqual(str(context.exception), "empty separator")

    def test_split_empty_bytes_without_sep_returns_empty_list(self):
        b = bytearray(b"")
        self.assertEqual(b.split(), [])

    def test_split_empty_bytes_with_sep_returns_list_of_empty_bytes(self):
        b = bytearray(b"")
        self.assertEqual(b.split(b"a"), [b""])

    def test_split_without_sep_splits_whitespace(self):
        result = bytearray(b" foo bar  \t \nbaz\r\n   ").split()
        self.assertIsInstance(result[0], bytearray)
        self.assertEqual(result, [b"foo", b"bar", b"baz"])

    def test_split_with_none_sep_splits_whitespace_maxsplit_times(self):
        result = bytearray(b" foo bar  \t \nbaz\r\n   ").split(None, 2)
        self.assertIsInstance(result[0], bytearray)
        self.assertEqual(result, [b"foo", b"bar", b"baz\r\n   "])

    def test_split_by_byteslike_returns_list(self):
        b = bytearray(b"foo bar baz")
        self.assertEqual(b.split(b" "), [b"foo", b"bar", b"baz"])
        self.assertEqual(b.split(bytearray(b"o")), [b"f", b"", b" bar baz"])
        self.assertEqual(b.split(b"ba"), [b"foo ", b"r ", b"z"])
        self.assertEqual(b.split(b"not found"), [b])

    def test_startswith_with_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray.startswith(b"", bytearray())
        self.assertIn(
            "'startswith' requires a 'bytearray' object but received a 'bytes'",
            str(context.exception),
        )

    def test_startswith_with_list_other_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().startswith([])
        self.assertEqual(
            str(context.exception),
            "startswith first arg must be bytes or a tuple of bytes, not list",
        )

    def test_startswith_with_tuple_other_checks_each(self):
        haystack = bytearray(b"123")
        needle1 = (b"12", b"13", b"23", b"d")
        needle2 = (b"2", b"asd", b"122222")
        self.assertTrue(haystack.startswith(needle1))
        self.assertFalse(haystack.startswith(needle2))

    def test_startswith_with_start_searches_from_start(self):
        haystack = bytearray(b"12345")
        needle1 = bytearray(b"1")
        needle4 = b"34"
        self.assertFalse(haystack.startswith(needle1, 1))
        self.assertFalse(haystack.startswith(needle4, 0))
        self.assertTrue(haystack.startswith(needle1, 0, 1))
        self.assertTrue(haystack.startswith(needle4, 2))

    def test_startswith_with_empty_returns_true_for_valid_bounds(self):
        haystack = bytearray(b"12345")
        self.assertTrue(haystack.startswith(bytearray()))
        self.assertTrue(haystack.startswith(b"", 5))
        self.assertTrue(haystack.startswith(bytearray(), -9, 1))

    def test_startswith_with_empty_returns_false_for_invalid_bounds(self):
        haystack = bytearray(b"12345")
        self.assertFalse(haystack.startswith(b"", 3, 2))
        self.assertFalse(haystack.startswith(bytearray(), 6))

    def test_strip_with_non_byteslike_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().strip("")
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_strip_with_none_or_default_strips_ascii_space(self):
        self.assertEqual(bytearray().strip(None), b"")
        self.assertEqual(bytearray(b"     ").strip(None), b"")
        self.assertEqual(bytearray(b"  h i  ").strip(), b"h i")

    def test_strip_with_byteslike_strips_bytes(self):
        self.assertEqual(bytearray().strip(b"123"), b"")
        self.assertEqual(bytearray(b"1aaa1").strip(bytearray()), b"1aaa1")
        self.assertEqual(bytearray(b"1 aaa1").strip(bytearray(b" 1")), b"aaa")
        self.assertEqual(bytearray(b"hello").strip(b"ho"), b"ell")

    def test_strip_noop_returns_new_bytearray(self):
        arr = bytearray(b"abcd")
        result = arr.strip(b"e")
        self.assertIsInstance(result, bytearray)
        self.assertEqual(result, arr)
        self.assertIsNot(result, arr)

    def test_upper_with_non_bytearray_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.upper("not a bytearray")

    def test_upper_empty_self_returns_new_bytearray(self):
        src = bytearray(b"")
        dst = src.lower()
        self.assertIsNot(src, dst)
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, src)

    def test_upper_all_upper_returns_new_bytearray(self):
        src = bytearray(b"ABCDEFGHIJKLMNOPQRSTUVWXYZ")
        dst = src.upper()
        self.assertIsNot(src, dst)
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(src, dst)

    def test_upper_all_upper_and_non_alphanumeric_returns_new_bytearray(self):
        src = bytearray(b"ABCDEFGHIJ1234567890!@#$%^&*()")
        dst = src.upper()
        self.assertIsNot(src, dst)
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(src, dst)

    def test_upper_all_lower_returns_all_uppercase(self):
        src = bytearray(b"abcdefghijklmnopqrstuvwxyz")
        dst = src.upper()
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, b"ABCDEFGHIJKLMNOPQRSTUVWXYZ")

    def test_upper_mixed_case_returns_all_uppercase(self):
        src = bytearray(b"aBcDeFgHiJkLmNoPqRsTuVwXyZ")
        dst = src.upper()
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, b"ABCDEFGHIJKLMNOPQRSTUVWXYZ")

    def test_upper_mixed_case_returns_all_uppercase(self):
        src = bytearray(b"a1!B2@c3#D4$e5%F6^g7&H8*i9(J0)")
        dst = src.upper()
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, b"A1!B2@C3#D4$E5%F6^G7&H8*I9(J0)")


class BytesTests(unittest.TestCase):
    def test_center_without_growth_returns_original_bytes(self):
        foo = b"foo"
        self.assertIs(foo.center(-1), foo)
        self.assertIs(foo.center(0), foo)
        self.assertIs(foo.center(1), foo)
        self.assertIs(foo.center(2), foo)
        self.assertIs(foo.center(3), foo)

    def test_center_with_both_odd_centers_bytes(self):
        self.assertEqual(b"abc".center(5), b" abc ")
        self.assertEqual(b"abc".center(7), b"  abc  ")

    def test_center_with_both_even_centers_bytes(self):
        self.assertEqual(b"".center(4), b"    ")
        self.assertEqual(b"abcd".center(8), b"  abcd  ")

    def test_center_with_odd_length_and_even_number_centers_bytes(self):
        self.assertEqual(b"foo".center(4), b"foo ")
        self.assertEqual(b"\t \n".center(6), b" \t \n  ")

    def test_center_with_even_length_and_odd_number_centers_bytes(self):
        self.assertEqual(b"food".center(5), b" food")
        self.assertEqual(b"\t  \n".center(7), b"  \t  \n ")

    def test_center_with_custom_fillchar_returns_bytes(self):
        self.assertEqual(b"ba".center(7, b"@"), b"@@@ba@@")

    def test_center_with_non_bytes_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".center(2, ord(" "))
        self.assertEqual(
            str(context.exception),
            "center() argument 2 must be a byte string of length 1, not int",
        )

    def test_center_with_wrong_length_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".center(2, b",,")
        self.assertEqual(
            str(context.exception),
            "center() argument 2 must be a byte string of length 1, not bytes",
        )

    def test_decode_finds_ascii(self):
        self.assertEqual(b"abc".decode("ascii"), "abc")

    def test_decode_finds_latin_1(self):
        self.assertEqual(b"abc\xE5".decode("latin-1"), "abc\xE5")

    def test_dunder_add_with_bytes_like_other_returns_bytes(self):
        self.assertEqual(b"123".__add__(bytearray(b"456")), b"123456")

    def test_dunder_add_with_non_bytes_like_other_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".__add__(2)
        self.assertEqual(str(context.exception), "can't concat int to bytes")

    def test_dunder_contains_with_non_bytes_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.__contains__("not_bytes", 123)

    def test_dunder_contains_with_element_in_bytes_returns_true(self):
        self.assertTrue(b"abc".__contains__(ord("a")))

    def test_dunder_contains_with_element_not_in_bytes_returns_false(self):
        self.assertFalse(b"abc".__contains__(ord("z")))

    def test_dunder_contains_calls_dunder_index(self):
        class C:
            __index__ = Mock(name="__index__", return_value=ord("a"))

        c = C()
        self.assertTrue(b"abc".__contains__(c))
        c.__index__.assert_called_once()

    def test_dunder_contains_calls_dunder_index_before_checking_byteslike(self):
        class C(bytearray):
            __index__ = Mock(name="__index__", return_value=ord("a"))

        c = C(b"q")
        self.assertTrue(b"abc".__contains__(c))
        c.__index__.assert_called_once()

    def test_dunder_contains_ignores_errors_from_dunder_index(self):
        class C:
            __index__ = Mock(name="__index__", side_effect=MemoryError("foo"))

        c = C()
        container = b"abc"
        with self.assertRaises(TypeError):
            container.__contains__(c)
        c.__index__.assert_called_once()

    def test_dunder_contains_with_single_byte_byteslike_returns_true(self):
        self.assertTrue(b"abc".__contains__(b"a"))
        self.assertTrue(b"abc".__contains__(bytearray(b"a")))

    def test_dunder_contains_with_single_byte_byteslike_returns_false(self):
        self.assertFalse(b"abc".__contains__(b"z"))
        self.assertFalse(b"abc".__contains__(bytearray(b"z")))

    def test_dunder_contains_with_byteslike_returns_true(self):
        self.assertTrue(b"foobar".__contains__(b"foo"))
        self.assertTrue(b"foobar".__contains__(bytearray(b"bar")))

    def test_dunder_contains_with_byteslike_returns_false(self):
        self.assertFalse(b"foobar".__contains__(b"baz"))
        self.assertFalse(b"foobar".__contains__(bytearray(b"baz")))

    def test_dunder_iter_returns_iterator(self):
        b = b"123"
        it = b.__iter__()
        self.assertTrue(hasattr(it, "__next__"))
        self.assertIs(iter(it), it)

    def test_dunder_new_with_str_without_encoding_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes("foo")

    def test_dunder_new_with_str_and_encoding_returns_bytes(self):
        self.assertEqual(bytes("foo", "ascii"), b"foo")

    def test_dunder_new_with_ignore_errors_returns_bytes(self):
        self.assertEqual(bytes("fo\x80o", "ascii", "ignore"), b"foo")

    def test_dunder_new_with_bytes_subclass_returns_bytes_subclass(self):
        class A(bytes):
            pass

        class B(bytes):
            pass

        class C:
            def __bytes__(self):
                return B()

        self.assertIsInstance(bytes.__new__(A, b""), A)
        self.assertIsInstance(bytes.__new__(B, 2), B)
        self.assertIsInstance(bytes.__new__(A, C()), A)

    def test_dunder_str_returns_same_as_dunder_repr(self):
        b = b"foobar\x80"
        b_str = b.__repr__()
        self.assertEqual(b_str, "b'foobar\\x80'")
        self.assertEqual(b_str, b.__str__())

    def test_count_with_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytes.count(bytearray(), b"")
        self.assertIn(
            "'count' requires a 'bytes' object but received a 'bytearray'",
            str(context.exception),
        )

    def test_count_with_nonbyte_int_raises_value_error(self):
        haystack = b""
        needle = 266
        with self.assertRaises(ValueError) as context:
            haystack.count(needle)
        self.assertEqual(str(context.exception), "byte must be in range(0, 256)")

    # TODO(T65863013): Make bytes.count behavior match CPython and remove this
    # skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_count_with_string_raises_type_error(self):
        haystack = b""
        needle = "133"
        with self.assertRaises(TypeError) as context:
            haystack.count(needle)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    # TODO(T65863013): Make bytes.count behavior match CPython and remove this
    # skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_count_with_non_number_index_raises_type_error(self):
        class Idx:
            def __index__(self):
                return ord("a")

        haystack = b"abc"
        needle = Idx()
        with self.assertRaises(TypeError) as context:
            haystack.count(needle)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'Idx'"
        )

    def test_count_with_dunder_int_calls_dunder_index(self):
        class Desc:
            def __get__(self, obj, type):
                raise NotImplementedError("called descriptor")

        class Idx:
            def __index__(self):
                return ord("a")

            __int__ = Desc()

        haystack = b"abc"
        needle = Idx()
        self.assertEqual(haystack.count(needle), 1)

    def test_count_with_dunder_float_calls_dunder_index(self):
        class Desc:
            def __get__(self, obj, type):
                raise NotImplementedError("called descriptor")

        class Idx:
            __float__ = Desc()

            def __index__(self):
                return ord("a")

        haystack = b"abc"
        needle = Idx()
        self.assertEqual(haystack.count(needle), 1)

    # TODO(T65863013): Make bytes.count behavior match CPython and remove this
    # skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_count_with_index_overflow_raises_value_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise OverflowError("not a byte!")

        haystack = b"abc"
        needle = Idx()
        with self.assertRaises(ValueError) as context:
            haystack.count(needle)
        self.assertEqual(str(context.exception), "byte must be in range(0, 256)")

    # TODO(T65863013): Make bytes.count behavior match CPython and remove this
    # skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_count_with_index_error_raises_type_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise TypeError("not a byte!")

        haystack = b"abc"
        needle = Idx()
        with self.assertRaises(TypeError) as context:
            haystack.count(needle)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'Idx'"
        )

    def test_count_with_empty_sub_returns_length_minus_adjusted_start_plus_one(self):
        haystack = b"abcde"
        needle = bytearray()
        self.assertEqual(haystack.count(needle, -3), 4)

    def test_count_with_missing_returns_zero(self):
        haystack = b"abc"
        needle = b"d"
        self.assertEqual(haystack.count(needle), 0)

    def test_count_with_missing_stays_within_bounds(self):
        haystack = b"abc"
        needle = bytearray(b"c")
        self.assertEqual(haystack.count(needle, None, 2), 0)

    def test_count_with_large_start_returns_zero(self):
        haystack = b"abc"
        needle = bytearray(b"")
        self.assertEqual(haystack.count(needle, 10), 0)

    def test_count_with_negative_bounds_returns_count(self):
        haystack = b"foobar"
        needle = bytearray(b"o")
        self.assertEqual(haystack.count(needle, -6, -1), 2)

    def test_count_returns_non_overlapping_count(self):
        haystack = b"abababab"
        needle = bytearray(b"aba")
        self.assertEqual(haystack.count(needle), 2)

    def test_endswith_with_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytes.endswith(bytearray(), b"")
        self.assertIn(
            "'endswith' requires a 'bytes' object but received a 'bytearray'",
            str(context.exception),
        )

    def test_endswith_with_list_other_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".endswith([])
        self.assertEqual(
            str(context.exception),
            "endswith first arg must be bytes or a tuple of bytes, not list",
        )

    def test_endswith_with_tuple_other_checks_each(self):
        haystack = b"123"
        needle1 = (b"12", b"13", b"23", b"d")
        needle2 = (b"2", b"asd", b"122222")
        self.assertTrue(haystack.endswith(needle1))
        self.assertFalse(haystack.endswith(needle2))

    def test_endswith_with_end_searches_from_end(self):
        haystack = b"12345"
        needle1 = bytearray(b"1")
        needle4 = b"34"
        self.assertFalse(haystack.endswith(needle1, 0))
        self.assertFalse(haystack.endswith(needle4, 1))
        self.assertTrue(haystack.endswith(needle1, 0, 1))
        self.assertTrue(haystack.endswith(needle4, 1, 4))

    def test_endswith_with_empty_returns_true_for_valid_bounds(self):
        haystack = b"12345"
        self.assertTrue(haystack.endswith(bytearray()))
        self.assertTrue(haystack.endswith(b"", 5))
        self.assertTrue(haystack.endswith(bytearray(), -9, 1))

    def test_endswith_with_empty_returns_false_for_invalid_bounds(self):
        haystack = b"12345"
        self.assertFalse(haystack.endswith(b"", 3, 2))
        self.assertFalse(haystack.endswith(bytearray(), 6))

    def test_find_with_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.find(bytearray(), b"")

    def test_find_with_empty_sub_returns_start(self):
        haystack = b"abc"
        needle = bytearray()
        self.assertEqual(haystack.find(needle, 1), 1)

    def test_find_with_missing_returns_negative(self):
        haystack = b"abc"
        needle = b"d"
        self.assertEqual(haystack.find(needle), -1)

    def test_find_with_missing_stays_within_bounds(self):
        haystack = b"abc"
        needle = bytearray(b"c")
        self.assertEqual(haystack.find(needle, None, 2), -1)

    def test_find_with_large_start_returns_negative(self):
        haystack = b"abc"
        needle = bytearray(b"c")
        self.assertEqual(haystack.find(needle, 10), -1)

    def test_find_with_negative_bounds_returns_index(self):
        haystack = b"foobar"
        needle = bytearray(b"o")
        self.assertEqual(haystack.find(needle, -6, -1), 1)

    def test_find_with_multiple_matches_returns_first_index_in_range(self):
        haystack = b"abbabbabba"
        needle = bytearray(b"abb")
        self.assertEqual(haystack.find(needle, 1), 3)

    def test_find_with_nonbyte_int_raises_value_error(self):
        haystack = b""
        needle = 266
        with self.assertRaises(ValueError):
            haystack.find(needle)

    def test_find_with_string_raises_type_error(self):
        haystack = b""
        needle = "133"
        with self.assertRaises(TypeError):
            haystack.find(needle)

    # TODO(T65863013): Make bytes.find behavior match CPython and remove this
    # skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_find_with_non_number_index_raises_type_error(self):
        class Idx:
            def __index__(self):
                return ord("a")

        haystack = b"abc"
        needle = Idx()
        with self.assertRaises(TypeError):
            haystack.find(needle)

    def test_find_with_dunder_int_calls_dunder_index(self):
        class Desc:
            def __get__(self, obj, type):
                raise NotImplementedError("called descriptor")

        class Idx:
            def __index__(self):
                return ord("a")

            __int__ = Desc()

        haystack = b"abc"
        needle = Idx()
        self.assertEqual(haystack.find(needle), 0)

    def test_find_with_dunder_float_calls_dunder_index(self):
        class Desc:
            def __get__(self, obj, type):
                raise NotImplementedError("called descriptor")

        class Idx:
            __float__ = Desc()

            def __index__(self):
                return ord("a")

        haystack = b"abc"
        needle = Idx()
        self.assertEqual(haystack.find(needle), 0)

    # TODO(T65863013): Make bytes.find behavior match CPython and remove this
    # skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_find_with_index_overflow_raises_value_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise OverflowError("not a byte!")

        haystack = b"abc"
        needle = Idx()
        with self.assertRaises(ValueError) as context:
            haystack.find(needle)
        self.assertEqual(str(context.exception), "byte must be in range(0, 256)")

    # TODO(T65863013): Make bytes.find behavior match CPython and remove this
    # skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_find_with_index_error_raises_type_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise TypeError("not a byte!")

        haystack = b"abc"
        needle = Idx()
        with self.assertRaises(TypeError) as context:
            haystack.find(needle)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'Idx'"
        )

    def test_fromhex_returns_bytes_instance(self):
        self.assertEqual(bytes.fromhex("1234 ab AB"), b"\x124\xab\xab")

    def test_fromhex_ignores_spaces(self):
        self.assertEqual(bytes.fromhex("ab cc deff"), b"\xab\xcc\xde\xff")

    def test_fromhex_with_trailing_spaces_returns_bytes(self):
        self.assertEqual(bytes.fromhex("ABCD  "), b"\xab\xcd")

    def test_fromhex_with_number_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.fromhex(1234)

    def test_fromhex_with_bad_byte_groupings_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            bytes.fromhex("abc d")
        self.assertEqual(
            str(context.exception),
            "non-hexadecimal number found in fromhex() arg at position 3",
        )

    def test_fromhex_with_dangling_nibble_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            bytes.fromhex("AB AB C")
        self.assertEqual(
            str(context.exception),
            "non-hexadecimal number found in fromhex() arg at position 7",
        )

    def test_fromhex_with_non_ascii_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            bytes.fromhex("édcb")
        self.assertEqual(
            str(context.exception),
            "non-hexadecimal number found in fromhex() arg at position 0",
        )

    def test_fromhex_with_non_hex_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            bytes.fromhex("0123abcdefgh")
        self.assertEqual(
            str(context.exception),
            "non-hexadecimal number found in fromhex() arg at position 10",
        )

    def test_fromhex_with_bytes_subclass_returns_subclass_instance(self):
        class C(bytes):
            __init__ = Mock(name="__init__", return_value=None)

        c = C.fromhex("1111")
        self.assertIs(c.__class__, C)
        c.__init__.assert_called_once()
        self.assertEqual(c, b"\x11\x11")

    def test_index_with_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.index(bytearray(), b"")

    def test_index_with_subsequence_returns_first_in_range(self):
        haystack = b"-a---a-aa"
        needle = ord("a")
        self.assertEqual(haystack.index(needle, 3), 5)

    def test_index_with_missing_raises_value_error(self):
        haystack = b"abc"
        needle = b"d"
        with self.assertRaises(ValueError) as context:
            haystack.index(needle)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_index_outside_of_bounds_raises_value_error(self):
        haystack = b"abc"
        needle = bytearray(b"c")
        with self.assertRaises(ValueError) as context:
            haystack.index(needle, 0, 2)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_iteration_returns_ints(self):
        expected = [97, 98, 99, 100]
        index = 0
        for val in b"abcd":
            self.assertEqual(val, expected[index])
            index += 1
        self.assertEqual(index, len(expected))

    def test_join_with_non_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytes.join(1, [])
        self.assertIn(
            "'join' requires a 'bytes' object but received a 'int'",
            str(context.exception),
        )

    def test_lower_with_non_bytes_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.lower("not a bytes")

    def test_lower_empty_self_returns_self(self):
        src = b""
        dst = src.lower()
        self.assertIs(src, dst)

    def test_lower_all_lower_returns_new_bytes(self):
        src = b"abcdefghijklmnopqrstuvwxyz"
        dst = src.lower()
        self.assertIsNot(src, dst)
        self.assertIsInstance(dst, bytes)
        self.assertEqual(src, dst)

    def test_lower_all_lower_and_non_alphanumeric_returns_self(self):
        src = b"abcdefghijklmnopqrstuvwxyz1234567890"
        dst = src.lower()
        self.assertIsNot(src, dst)
        self.assertIsInstance(dst, bytes)
        self.assertEqual(src, dst)

    def test_lower_all_uppercase_returns_all_lowercase(self):
        src = b"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        dst = src.lower()
        self.assertIsInstance(dst, bytes)
        self.assertEqual(dst, b"abcdefghijklmnopqrstuvwxyz")

    def test_lower_mixed_case_returns_all_lowercase(self):
        src = b"aBcDeFgHiJkLmNoPqRsTuVwXyZ"
        dst = src.lower()
        self.assertIsInstance(dst, bytes)
        self.assertEqual(dst, b"abcdefghijklmnopqrstuvwxyz")

    def test_lower_mixed_case_returns_all_lowercase(self):
        src = b"a1!B2@c3#D4$e5%F6^g7&H8*i9(J0)"
        dst = src.lower()
        self.assertIsInstance(dst, bytes)
        self.assertEqual(dst, b"a1!b2@c3#d4$e5%f6^g7&h8*i9(j0)")

    def test_ljust_without_growth_returns_original_bytes(self):
        foo = b"foo"
        self.assertIs(foo.ljust(-1), foo)
        self.assertIs(foo.ljust(0), foo)
        self.assertIs(foo.ljust(1), foo)
        self.assertIs(foo.ljust(2), foo)
        self.assertIs(foo.ljust(3), foo)

    def test_ljust_pads_end_of_bytes(self):
        self.assertEqual(b"abc".ljust(4), b"abc ")
        self.assertEqual(b"abc".ljust(7), b"abc    ")

    def test_ljust_with_custom_fillchar_returns_bytes(self):
        self.assertEqual(b"ba".ljust(7, b"@"), b"ba@@@@@")

    def test_ljust_with_non_bytes_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".ljust(2, ord(" "))
        self.assertEqual(
            str(context.exception),
            "ljust() argument 2 must be a byte string of length 1, not int",
        )

    def test_ljust_with_wrong_length_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".ljust(2, b",,")
        self.assertEqual(
            str(context.exception),
            "ljust() argument 2 must be a byte string of length 1, not bytes",
        )

    def test_ljust_with_swapped_width_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".ljust(b",", 2)
        self.assertEqual(
            str(context.exception), "'bytes' object cannot be interpreted as an integer"
        )

    def test_lstrip_with_non_byteslike_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".lstrip("")
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_lstrip_with_none_or_default_strips_ascii_space(self):
        self.assertEqual(b"".lstrip(None), b"")
        self.assertEqual(b"      ".lstrip(None), b"")
        self.assertEqual(b"  hi  ".lstrip(), b"hi  ")

    def test_lstrip_with_byteslike_strips_bytes(self):
        self.assertEqual(b"".lstrip(b"123"), b"")
        self.assertEqual(b"1aaa1".lstrip(bytearray()), b"1aaa1")
        self.assertEqual(b"1 aaa1".lstrip(bytearray(b" 1")), b"aaa1")
        self.assertEqual(b"hello".lstrip(b"eho"), b"llo")

    def test_replace(self):
        test = b"mississippi"
        self.assertEqual(test.replace(b"i", b"a"), b"massassappa")
        self.assertEqual(test.replace(b"i", b"vv"), b"mvvssvvssvvppvv")
        self.assertEqual(test.replace(b"ss", b"x"), b"mixixippi")
        self.assertEqual(test.replace(bytearray(b"ss"), bytearray(b"x")), b"mixixippi")
        self.assertEqual(test.replace(b"i", bytearray()), b"msssspp")
        self.assertEqual(test.replace(bytearray(b"i"), b""), b"msssspp")

    def test_replace_with_count(self):
        test = b"mississippi"
        self.assertEqual(test.replace(b"i", b"a", 0), b"mississippi")
        self.assertEqual(test.replace(b"i", b"a", 2), b"massassippi")

    def test_replace_int_error(self):
        with self.assertRaises(TypeError) as context:
            b"a b".replace(b"r", 4)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'int'"
        )

    def test_replace_str_error(self):
        with self.assertRaises(TypeError) as context:
            b"a b".replace(b"r", "s")
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_replace_float_count_error(self):
        with self.assertRaises(TypeError) as context:
            b"a b".replace(b"r", b"b", 4.2)
        self.assertEqual(str(context.exception), "integer argument expected, got float")

    def test_replace_str_count_error(self):
        test = b"a b"
        self.assertRaises(TypeError, test.replace, "r", b"", "hello")

    def test_rfind_with_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytes.rfind(bytearray(), b"")
        self.assertIn(
            "'rfind' requires a 'bytes' object but received a 'bytearray'",
            str(context.exception),
        )

    def test_rfind_with_empty_sub_returns_end(self):
        haystack = b"abc"
        needle = bytearray()
        self.assertEqual(haystack.rfind(needle), 3)

    def test_rfind_with_missing_returns_negative(self):
        haystack = b"abc"
        needle = b"d"
        self.assertEqual(haystack.rfind(needle), -1)

    def test_rfind_with_missing_stays_within_bounds(self):
        haystack = b"abc"
        needle = bytearray(b"c")
        self.assertEqual(haystack.rfind(needle, None, 2), -1)

    def test_rfind_with_large_start_returns_negative(self):
        haystack = b"abc"
        needle = bytearray(b"c")
        self.assertEqual(haystack.rfind(needle, 10), -1)

    def test_rfind_with_negative_bounds_returns_index(self):
        haystack = b"foobar"
        needle = bytearray(b"o")
        self.assertEqual(haystack.rfind(needle, -6, -1), 2)

    def test_rfind_with_multiple_matches_returns_last_index_in_range(self):
        haystack = b"abbabbabba"
        needle = bytearray(b"abb")
        self.assertEqual(haystack.rfind(needle), 6)

    def test_rfind_with_nonbyte_int_raises_value_error(self):
        haystack = b""
        needle = 266
        with self.assertRaises(ValueError) as context:
            haystack.rfind(needle)
        self.assertEqual(str(context.exception), "byte must be in range(0, 256)")

    # TODO(T65863013): Make bytes.rfind behavior match CPython and remove this
    # skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_rfind_with_string_raises_type_error(self):
        haystack = b""
        needle = "133"
        with self.assertRaises(TypeError) as context:
            haystack.rfind(needle)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    # TODO(T65863013): Make bytes.rfind behavior match CPython and remove this
    # skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_rfind_with_non_number_index_raises_type_error(self):
        class Idx:
            def __index__(self):
                return ord("a")

        haystack = b"abc"
        needle = Idx()
        with self.assertRaises(TypeError) as context:
            haystack.rfind(needle)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'Idx'"
        )

    def test_rfind_with_dunder_int_calls_dunder_index(self):
        class Idx:
            def __int__(self):
                raise NotImplementedError("called __int__")

            def __index__(self):
                return ord("a")

        haystack = b"abc"
        needle = Idx()
        self.assertEqual(haystack.rfind(needle), 0)

    def test_rfind_with_dunder_float_calls_dunder_index(self):
        class Idx:
            def __float__(self):
                raise NotImplementedError("called __float__")

            def __index__(self):
                return ord("a")

        haystack = b"abc"
        needle = Idx()
        self.assertEqual(haystack.rfind(needle), 0)

    def test_rindex_with_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.rindex(bytearray(), b"")

    def test_rindex_with_subsequence_returns_last_in_range(self):
        haystack = b"-a-aa----a--"
        needle = ord("a")
        self.assertEqual(haystack.rindex(needle, 2, 8), 4)

    def test_rindex_with_missing_raises_value_error(self):
        haystack = b"abc"
        needle = b"d"
        with self.assertRaises(ValueError) as context:
            haystack.rindex(needle)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_rindex_outside_of_bounds_raises_value_error(self):
        haystack = b"abc"
        needle = bytearray(b"c")
        with self.assertRaises(ValueError) as context:
            haystack.rindex(needle, 0, 2)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_rjust_without_growth_returns_original_bytes(self):
        foo = b"foo"
        self.assertIs(foo.rjust(-1), foo)
        self.assertIs(foo.rjust(0), foo)
        self.assertIs(foo.rjust(1), foo)
        self.assertIs(foo.rjust(2), foo)
        self.assertIs(foo.rjust(3), foo)

    def test_rjust_pads_beginning_of_bytes(self):
        self.assertEqual(b"abc".rjust(4), b" abc")
        self.assertEqual(b"abc".rjust(7), b"    abc")

    def test_rjust_with_custom_fillchar_returns_bytes(self):
        self.assertEqual(b"ba".rjust(7, b"@"), b"@@@@@ba")

    def test_rjust_with_non_bytes_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".rjust(2, ord(" "))
        self.assertEqual(
            str(context.exception),
            "rjust() argument 2 must be a byte string of length 1, not int",
        )

    def test_rjust_with_wrong_length_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".rjust(2, b",,")
        self.assertEqual(
            str(context.exception),
            "rjust() argument 2 must be a byte string of length 1, not bytes",
        )

    def test_rstrip_with_non_byteslike_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".rstrip("")
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_rstrip_with_none_or_default_strips_ascii_space(self):
        self.assertEqual(b"".rstrip(None), b"")
        self.assertEqual(b"      ".rstrip(None), b"")
        self.assertEqual(b"  hi  ".rstrip(), b"  hi")

    def test_rstrip_with_byteslike_strips_bytes(self):
        self.assertEqual(b"".rstrip(b"123"), b"")
        self.assertEqual(b"1aaa1".rstrip(bytearray()), b"1aaa1")
        self.assertEqual(b"1aa a1".rstrip(bytearray(b" 1")), b"1aa a")
        self.assertEqual(b"hello".rstrip(b"lo"), b"he")

    def test_split_with_non_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytes.split("foo bar")
        self.assertIn(
            "'split' requires a 'bytes' object but received a 'str'",
            str(context.exception),
        )

    def test_split_with_non_byteslike_sep_raises_type_error(self):
        b = b"foo bar"
        sep = ""
        with self.assertRaises(TypeError) as context:
            b.split(sep)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_split_with_non_index_maxsplit_raises_type_error(self):
        b = b"foo bar"
        with self.assertRaises(TypeError) as context:
            b.split(maxsplit=None)
        self.assertEqual(
            str(context.exception),
            "'NoneType' object cannot be interpreted as an integer",
        )

    def test_split_with_large_int_raises_overflow_error(self):
        b = b"foo bar"
        with self.assertRaises(OverflowError) as context:
            b.split(maxsplit=2 ** 64)
        self.assertEqual(
            str(context.exception), "Python int too large to convert to C ssize_t"
        )

    def test_split_with_empty_sep_raises_value_error(self):
        b = b"foo bar"
        with self.assertRaises(ValueError) as context:
            b.split(bytearray())
        self.assertEqual(str(context.exception), "empty separator")

    def test_split_empty_bytes_without_sep_returns_empty_list(self):
        b = b""
        self.assertEqual(b.split(), [])

    def test_split_empty_bytes_with_sep_returns_list_of_empty_bytes(self):
        b = b""
        self.assertEqual(b.split(b"a"), [b""])

    def test_split_without_sep_splits_whitespace(self):
        b = b"foo bar"
        self.assertEqual(b.split(), [b"foo", b"bar"])
        b = b" foo bar  \t \nbaz\r\n   "
        self.assertEqual(b.split(), [b"foo", b"bar", b"baz"])

    def test_split_with_none_sep_splits_whitespace_maxsplit_times(self):
        b = b" foo bar  \t \nbaz\r\n   "
        self.assertEqual(b.split(None, 2), [b"foo", b"bar", b"baz\r\n   "])

    def test_split_by_byteslike_returns_list(self):
        b = b"foo bar baz"
        self.assertEqual(b.split(b" "), [b"foo", b"bar", b"baz"])
        self.assertEqual(b.split(bytearray(b"o")), [b"f", b"", b" bar baz"])
        self.assertEqual(b.split(b"ba"), [b"foo ", b"r ", b"z"])
        self.assertEqual(b.split(b"not found"), [b])

    def test_splitlines_with_non_bytes_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.splitlines(None)

    def test_splitlines_with_float_keepends_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.splitlines(b"hello", 0.4)

    def test_splitlines_with_string_keepends_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.splitlines(b"hello", "1")

    def test_splitlines_returns_list(self):
        self.assertEqual(bytes.splitlines(b"", False), [])
        self.assertEqual(bytes.splitlines(b"a", 0), [b"a"])
        self.assertEqual(bytes.splitlines(b"a\nb\rc"), [b"a", b"b", b"c"])
        self.assertEqual(bytes.splitlines(b"a\r\nb\r\n"), [b"a", b"b"])
        self.assertEqual(bytes.splitlines(b"\n\r\n\r"), [b"", b"", b""])
        self.assertEqual(bytes.splitlines(b"a\x0Bb"), [b"a\x0Bb"])

    def test_splitlines_with_keepend_returns_list(self):
        self.assertEqual(bytes.splitlines(b"", True), [])
        self.assertEqual(bytes.splitlines(b"a", 1), [b"a"])
        self.assertEqual(bytes.splitlines(b"a\nb\rc", 1), [b"a\n", b"b\r", b"c"])
        self.assertEqual(bytes.splitlines(b"a\r\nb\r\n", 1), [b"a\r\n", b"b\r\n"])
        self.assertEqual(bytes.splitlines(b"\n\r\n\r", 1), [b"\n", b"\r\n", b"\r"])

    def test_startswith_with_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytes.startswith(bytearray(), b"")
        self.assertIn(
            "'startswith' requires a 'bytes' object but received a 'bytearray'",
            str(context.exception),
        )

    def test_startswith_with_list_other_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".startswith([])
        self.assertEqual(
            str(context.exception),
            "startswith first arg must be bytes or a tuple of bytes, not list",
        )

    def test_startswith_with_tuple_other_checks_each(self):
        haystack = b"123"
        needle1 = (b"13", b"23", b"12", b"d")
        needle2 = (b"2", b"asd", b"122222")
        self.assertTrue(haystack.startswith(needle1))
        self.assertFalse(haystack.startswith(needle2))

    def test_startswith_with_start_searches_from_start(self):
        haystack = b"12345"
        needle1 = bytearray(b"1")
        needle4 = b"34"
        self.assertFalse(haystack.startswith(needle1, 1))
        self.assertFalse(haystack.startswith(needle4, 0))
        self.assertTrue(haystack.startswith(needle1, 0))
        self.assertTrue(haystack.startswith(needle4, 2))

    def test_startswith_with_empty_returns_true_for_valid_bounds(self):
        haystack = b"12345"
        self.assertTrue(haystack.startswith(bytearray()))
        self.assertTrue(haystack.startswith(b"", 5))
        self.assertTrue(haystack.startswith(bytearray(), -9, 1))

    def test_startswith_with_empty_returns_false_for_invalid_bounds(self):
        haystack = b"12345"
        self.assertFalse(haystack.startswith(b"", 3, 2))
        self.assertFalse(haystack.startswith(bytearray(), 6))

    def test_strip_with_non_byteslike_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".strip("")
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_strip_with_none_or_default_strips_ascii_space(self):
        self.assertEqual(b"".strip(None), b"")
        self.assertEqual(b"    ".strip(None), b"")
        self.assertEqual(b"  hi  ".strip(), b"hi")

    def test_strip_with_byteslike_strips_bytes(self):
        self.assertEqual(b"".strip(b"123"), b"")
        self.assertEqual(b"1aaa1".strip(bytearray()), b"1aaa1")
        self.assertEqual(b"1 aaa1".strip(bytearray(b" 1")), b"aaa")
        self.assertEqual(b"hello".strip(b"ho"), b"ell")

    def test_upper_with_non_bytes_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.upper("not a bytes")

    def test_upper_empty_self_returns_self(self):
        src = b""
        dst = src.upper()
        self.assertIs(src, dst)

    def test_upper_all_upper_returns_new_bytes(self):
        src = b"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        dst = src.upper()
        self.assertIsInstance(dst, bytes)
        self.assertIsNot(src, dst)
        self.assertEqual(src, dst)

    def test_upper_all_upper_and_non_alphanumeric_returns_new_bytes(self):
        src = b"ABCDEFGHIJ1234567890!@#$%^&*()"
        dst = src.upper()
        self.assertIsInstance(dst, bytes)
        self.assertIsNot(src, dst)
        self.assertEqual(src, dst)

    def test_upper_all_lower_returns_all_uppercase(self):
        src = b"abcdefghijklmnopqrstuvwxyz"
        dst = src.upper()
        self.assertIsInstance(dst, bytes)
        self.assertEqual(dst, b"ABCDEFGHIJKLMNOPQRSTUVWXYZ")

    def test_upper_mixed_case_returns_all_uppercase(self):
        src = b"aBcDeFgHiJkLmNoPqRsTuVwXyZ"
        dst = src.upper()
        self.assertIsInstance(dst, bytes)
        self.assertEqual(dst, b"ABCDEFGHIJKLMNOPQRSTUVWXYZ")

    def test_upper_mixed_case_returns_all_uppercase(self):
        src = b"a1!B2@c3#D4$e5%F6^g7&H8*i9(J0)"
        dst = src.upper()
        self.assertIsInstance(dst, bytes)
        self.assertEqual(dst, b"A1!B2@C3#D4$E5%F6^G7&H8*I9(J0)")


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


class CodeTests(unittest.TestCase):
    def foo(self):
        pass

    CodeType = type(foo.__code__)

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

    def test_dunder_new_with_non_int_argcount_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType("not_int", 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1)
        self.assertIn("an integer is required (got type str)", str(context.exception))

    def test_dunder_new_with_non_int_posonlyargcount_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(1, "not_int", 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1)
        self.assertIn("an integer is required (got type str)", str(context.exception))

    def test_dunder_new_with_non_int_kwonlyargcount_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(1, 1, "not_int", 1, 1, 1, 1, 1, 1, 1, 1, 1, 1)
        self.assertIn("an integer is required (got type str)", str(context.exception))

    def test_dunder_new_with_non_int_nlocals_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(1, 1, 1, "not_int", 1, 1, 1, 1, 1, 1, 1, 1, 1)
        self.assertIn("an integer is required (got type str)", str(context.exception))

    def test_dunder_new_with_non_int_stacksize_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(1, 1, 1, 1, "not_int", 1, 1, 1, 1, 1, 1, 1, 1)
        self.assertIn("an integer is required (got type str)", str(context.exception))

    def test_dunder_new_with_non_bytes_code_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(1, 1, 1, 1, 1, "not_bytes", 1, 1, 1, 1, 1, 1, 1)
        self.assertIn("bytes", str(context.exception))
        self.assertIn("str", str(context.exception))

    def test_dunder_new_with_non_tuple_consts_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(1, 1, 1, 1, 1, b"", "not_tuple", 1, 1, 1, 1, 1, 1)
        self.assertIn("tuple", str(context.exception))
        self.assertIn("str", str(context.exception))

    def test_dunder_new_with_non_tuple_names_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(1, 1, 1, 1, 1, b"", (), "not_tuple", 1, 1, 1, 1, 1)
        self.assertIn("tuple", str(context.exception))
        self.assertIn("str", str(context.exception))

    def test_dunder_new_with_non_tuple_varnames_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(1, 1, 1, 1, 1, b"", (), (), "not_tuple", 1, 1, 1, 1)
        self.assertIn("tuple", str(context.exception))
        self.assertIn("str", str(context.exception))

    def test_dunder_new_with_non_str_filename_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(1, 1, 1, 1, 1, b"", (), (), (), b"not_str", 1, 1, 1)
        self.assertIn("str", str(context.exception))
        self.assertIn("bytes", str(context.exception))

    def test_dunder_new_with_non_str_name_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(1, 1, 1, 1, 1, b"", (), (), (), "filename", b"not_str", 1, 1)
        self.assertIn("str", str(context.exception))
        self.assertIn("bytes", str(context.exception))

    def test_dunder_new_with_non_int_firstlineno_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(
                1, 1, 1, 1, 1, b"", (), (), (), "filename", "name", "not_int", 1
            )
        self.assertIn("an integer is required (got type str)", str(context.exception))

    def test_dunder_new_with_non_bytes_lnotab_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(
                1, 1, 1, 1, 1, b"", (), (), (), "filename", "name", 1, "not_bytes"
            )
        self.assertIn("bytes", str(context.exception))
        self.assertIn("str", str(context.exception))

    def test_dunder_new_with_non_tuple_freevars_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(
                1, 1, 1, 1, 1, b"", (), (), (), "filename", "name", 1, b"", "not_tuple"
            )
        self.assertIn("tuple", str(context.exception))
        self.assertIn("str", str(context.exception))

    def test_dunder_new_with_non_tuple_cellvars_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.CodeType(
                1,
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

    def test_dunder_new_returns_code(self):
        result = self.CodeType(
            1, 1, 4, 1, 1, b"", (), (), ("a", "b"), "filename", "name", 1, b"", (), ()
        )
        self.assertIsInstance(result, self.CodeType)


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
        from types import ModuleType
        from types import CodeType

        code = compile("result = 42", "", "exec", 0, True, -1)
        self.assertIsInstance(code, CodeType)
        module = ModuleType("")
        exec(code, module.__dict__)
        self.assertEqual(module.result, 42)

    def test_single_returns_code(self):
        from types import ModuleType
        from types import CodeType

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
        from types import CodeType
        import __future__

        code = compile(
            "7 <> 9", "", "eval", __future__.CO_FUTURE_BARRY_AS_BDFL, True, -1
        )
        self.assertIsInstance(code, CodeType)

    def test_inherits_compile_flags(self):
        from types import CodeType
        import __future__

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

    # TODO(T65863013): Make complex.__new__ behavior match CPython and remove
    # this skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_dunder_new_with_non_complex_dunder_complex(self):
        class C:
            def __complex__(self):
                return 1

        with self.assertRaises(TypeError) as context:
            complex(C())
        self.assertEqual(
            str(context.exception), "__complex__ should return a complex object"
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


class DictTests(unittest.TestCase):
    def test_repr_returns_string(self):
        self.assertEqual(dict.__repr__({}), "{}")
        self.assertEqual(dict.__repr__({42: 42}), "{42: 42}")
        self.assertEqual(
            dict.__repr__({1: 1, 2: 2, "hello": "hello"}),
            "{1: 1, 2: 2, 'hello': 'hello'}",
        )

        class M(type):
            def __repr__(cls):
                return "<M instance>"

        class C(metaclass=M):
            def __repr__(self):
                return "<C instance>"

        self.assertEqual(
            dict.__repr__({1: C, 2: C()}), "{1: <M instance>, 2: <C instance>}"
        )

    def test_repr_with_recursive_dict_prints_ellipsis(self):
        d = {}
        d[1] = d
        self.assertEqual(dict.__repr__(d), "{1: {...}}")

    def test_setitem_for_new_keys_keeps_insertion_order(self):
        d = {}
        d["a"] = 1
        d["b"] = 2
        d["c"] = 3
        self.assertEqual(list(d.keys()), ["a", "b", "c"])

    def test_setitem_for_existing_key_preserves_order(self):
        d = {}
        d["a"] = 1
        d["b"] = 2
        d["c"] = 3
        d["a"] = 100
        self.assertEqual(list(d.keys()), ["a", "b", "c"])
        self.assertIs(d["a"], 100)

    def test_setitem_for_deleted_key_inserted_last(self):
        d = {}
        d["a"] = 1
        d["b"] = 2
        d["c"] = 3
        del d["a"]
        d["a"] = 100
        self.assertEqual(list(d.keys()), ["b", "c", "a"])
        self.assertIs(d["a"], 100)

    def test_clear_with_non_dict_raises_type_error(self):
        with self.assertRaises(TypeError):
            dict.clear(None)

    def test_clear_removes_all_elements(self):
        d = {"a": 1}
        self.assertEqual(dict.clear(d), None)
        self.assertEqual(d.__len__(), 0)
        self.assertNotIn("a", d)

    def test_copy_with_non_dict_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            dict.copy(None)
        self.assertIn(
            "'copy' requires a 'dict' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_dunder_hash_is_none(self):
        self.assertIs(dict.__hash__, None)

    def test_fromkeys_with_non_iterable_raises_type_error(self):
        with self.assertRaisesRegex(TypeError, "'int' object is not iterable"):
            dict.fromkeys(5, ())

    def test_fromkeys_calls_dunder_iter_on_iterable(self):
        class C:
            def __iter__(self):
                return [1, 2, 3].__iter__()

        result = dict.fromkeys(C())
        self.assertDictEqual(result, {1: None, 2: None, 3: None})

    def test_fromkeys_sets_dict_values_to_given_value(self):
        class C:
            def __iter__(self):
                return [1, 2, 3].__iter__()

        result = dict.fromkeys(C(), "abc")
        self.assertDictEqual(result, {1: "abc", 2: "abc", 3: "abc"})

    def test_fromkeys_calls_subclass_dunder_init(self):
        class C(dict):
            __init__ = Mock(name="__init__", return_value=None)

        result = C.fromkeys(())
        self.assertDictEqual(result, {})
        C.__init__.assert_called_once()

    def test_fromkeys_calls_subclass_dunder_new(self):
        class C(dict):
            __new__ = Mock(name="__new__", return_value={})

        result = C.fromkeys(())
        self.assertDictEqual(result, {})
        C.__new__.assert_called_once()

    def test_fromkeys_calls_subclass_dunder_setitem(self):
        class C(dict):
            __setitem__ = Mock(name="__setitem__", return_value={})

        result = C.fromkeys((1, 2, 3))
        self.assertDictEqual(result, {})
        C.__setitem__.assert_has_calls(
            [mock_call(1, None), mock_call(2, None), mock_call(3, None)]
        )

    def test_update_with_malformed_sequence_elt_raises_type_error(self):
        with self.assertRaises(ValueError):
            dict.update({}, [("a",)])

    def test_update_with_no_params_does_nothing(self):
        d = {"a": 1}
        d.update()
        self.assertEqual(len(d), 1)

    def test_update_with_mapping_adds_elements(self):
        d = {"a": 1}
        d.update([("a", "b"), ("c", "d")])
        self.assertIn("a", d)
        self.assertIn("c", d)
        self.assertEqual(d["a"], "b")
        self.assertEqual(d["c"], "d")

    def test_update_with_mapping_of_non_pair_tuple_raises_value_error(self):
        mapping = [("a", "b"), ("c", "d", "e")]
        d = {}
        with self.assertRaises(ValueError) as context:
            d.update(mapping)
        self.assertIn(
            "dictionary update sequence element #1 has length 3; 2 is required",
            str(context.exception),
        )

    def test_update_with_keys_attribute_calls_keys_method(self):
        class C:
            keys = Mock(name="keys", return_value=())
            __getitem__ = Mock(name="__getitem__")

        c = C()
        {}.update(c)
        c.keys.assert_called_once()

    def test_update_with_raising_keys_method_propagates_exception(self):
        class C:
            keys = Mock(name="keys", side_effect=Exception("foo"))
            __getitem__ = Mock(name="__getitem__")

        c = C()
        with self.assertRaisesRegex(Exception, "foo"):
            {}.update(c)
        c.keys.assert_called_once()
        c.__getitem__.assert_not_called()

    def test_update_with_keys_instance_attribute_calls_keys(self):
        class C:
            pass

        d = {}
        c = C()
        c.keys = Mock(name="keys", return_value=())
        c.__getitem__ = Mock(name="__getitem__")
        d.update(c)
        c.keys.assert_called_once()

    def test_update_with_keys_calls_dunder_iter_on_result(self):
        class D:
            __iter__ = Mock(name="__iter__", return_value=[].__iter__())

        iterable = D()

        class C:
            keys = Mock(name="keys", return_value=iterable)
            __getitem__ = Mock(name="__getitem__")

        c = C()
        {}.update(c)
        iterable.__iter__.assert_called_once()

    def test_update_with_keys_attribute_calls_getitem_with_key(self):
        class C:
            def keys(self):
                return ("foo", "bar")

            __getitem__ = Mock(name="__getitem__", return_value="baz")

        d = {}
        c = C()
        d.update(c)
        self.assertEqual(c.__getitem__.call_count, 2)
        self.assertEqual(d["foo"], "baz")
        self.assertEqual(d["bar"], "baz")

    def test_update_with_kwargs_returns_updated_dict(self):
        d1 = {"a": 1, "b": 2}
        d1.update(y=25)
        self.assertEqual(d1["y"], 25)

    def test_update_with_dict_and_kwargs_returns_updated_dict(self):
        d1 = {"a": 1, "b": 2}
        d2 = {"a": 10, "b": 11}
        d1.update(d2, b=20)
        self.assertEqual(d1["a"], 10)
        self.assertEqual(d1["b"], 20)

    def test_update_with_self_and_other_kwargs_adds_to_dict(self):
        d1 = {}
        d1.update(self="hello", seq="world")
        self.assertEqual(d1["self"], "hello")
        self.assertEqual(d1["seq"], "world")

    def test_dunder_delitem_with_none_dunder_hash(self):
        class C:
            __hash__ = None

        with self.assertRaises(TypeError):
            dict.__delitem__({}, C())

    def test_dunder_delitem_propagates_exceptions_from_dunder_hash(self):
        class C:
            def __hash__(self):
                raise UserWarning("foo")

        with self.assertRaises(UserWarning):
            dict.__delitem__({}, C())

    def test_concrete_dict_has_no_dunder_missing(self):
        with self.assertRaises(AttributeError):
            dict.__missing__

    def test_dunder_getitem_calls_dunder_missing(self):
        class C(dict):
            def __missing__(self, key):
                raise UserWarning("foo")

        result = C()
        self.assertRaises(UserWarning, result.__getitem__, "hello")

    def test_dunder_eq_with_different_item_count_returns_false(self):
        d0 = {4: 0}
        d1 = {4: 0, 8: 0}
        self.assertFalse(dict.__eq__(d0, d1))
        self.assertFalse(dict.__eq__(d1, d0))
        self.assertFalse(dict.__eq__({}, d0))
        self.assertFalse(dict.__eq__(d1, {}))

    def test_dunder_eq_with_different_keys_returns_false(self):
        d0 = {4: 0, "b": 17}
        d1 = {4: 0, "c": 17}
        self.assertFalse(dict.__eq__(d0, d1))

    def test_dunder_eq_with_different_values_returns_false(self):
        d0 = {4: 0, "b": 17}
        d1 = {4: 0, "b": 15}
        self.assertFalse(dict.__eq__(d0, d1))

    def test_dunder_eq_returns_true(self):
        self.assertTrue(dict.__eq__({}, {}))
        nan = float("nan")
        d0 = {4: "b", "a": 88, 42: nan, None: (42.42, b"x")}
        d1 = {4: "b", "a": 88, 42: nan, None: (42.42, b"x")}
        self.assertFalse(d0 is d1)
        self.assertTrue(dict.__eq__(d0, d1))

    def test_dunder_eq_checks_identity_before_calling_dunder_eq(self):
        class C:
            def __eq__(self, other):
                return False

        i = C()
        d0 = {"a": i}
        d1 = {"a": i}
        self.assertTrue(dict.__eq__(d0, d1))

    def test_dunder_eq_with_non_dict_returns_not_implemented(self):
        self.assertIs(dict.__eq__({}, "not a dict"), NotImplemented)

    def test_dunder_eq_calls_dunder_bool(self):
        class B:
            def __bool__(self):
                raise UserWarning()

        class C:
            def __eq__(self, other):
                return B()

            def __hash__(self):
                return 0

        d0 = {0: C()}
        d1 = {0: C()}
        with self.assertRaises(UserWarning):
            dict.__eq__(d0, d1)

    def test_dunder_ne_returns_not_eq(self):
        nan = float("nan")
        d0 = {4: "b", "a": 88, 42: nan, None: (42.42, b"x")}
        d1 = {4: "b", "a": 88, 42: nan, None: (42.42, b"x")}
        self.assertFalse(d0.__ne__(d1))
        self.assertTrue(d0.__ne__({}))

    def test_dunder_ne_returns_not_implemented_if_wrong_types(self):
        orig = {4: "b", "a": 88, None: (42.42, b"x")}
        self.assertIs(orig.__ne__(1), NotImplemented)
        self.assertIs(orig.__ne__([]), NotImplemented)

    def test_mix_bool_and_int_keys(self):
        d = {}
        d[True] = 42
        self.assertIn(1, d)
        self.assertEqual(d[True], 42)
        self.assertEqual(d[1], 42)
        self.assertNotIn(False, d)
        d[1] = "foo"
        self.assertEqual(len(d), 1)
        self.assertEqual(d[True], "foo")
        self.assertEqual(d[1], "foo")
        self.assertNotIn(False, d)

        d[0] = "bar"
        self.assertEqual(d[False], "bar")
        self.assertEqual(d[0], "bar")
        d[False] = "bar"
        self.assertEqual(len(d), 2)

    def test_popitem_with_non_dict_raise_type_error(self):
        with self.assertRaises(TypeError) as context:
            dict.popitem(None)
        self.assertIn("'popitem' requires a 'dict' object", str(context.exception))

    def test_popitem_with_empty_dict_raises_key_error(self):
        d = {}
        with self.assertRaises(KeyError) as context:
            dict.popitem(d)
        self.assertIn("popitem(): dictionary is empty", str(context.exception))

    def test_popitem_deletes_last_inserted_item_and_returns_it(self):
        d = {"a": 1, "b": 2, "c": 3}
        self.assertEqual(len(d), 3)
        key0, value0 = dict.popitem(d)
        key1, value1 = dict.popitem(d)
        key2, value2 = dict.popitem(d)
        self.assertEqual((key0, value0), ("c", 3))
        self.assertEqual((key1, value1), ("b", 2))
        self.assertEqual((key2, value2), ("a", 1))
        self.assertEqual(len(d), 0)

    def test_update_with_tuple_keys_propagates_exceptions_from_dunder_hash(self):
        class C:
            def __hash__(self):
                raise UserWarning("foo")

        class D:
            def keys(self):
                return (C(),)

            def __getitem__(self, key):
                return "foo"

        with self.assertRaises(UserWarning):
            dict.update({}, D())

    def test_update_with_list_keys_propagates_exceptions_from_dunder_hash(self):
        class C:
            def __hash__(self):
                raise UserWarning("foo")

        class D:
            def keys(self):
                return [C()]

            def __getitem__(self, key):
                return "foo"

        with self.assertRaises(UserWarning):
            dict.update({}, D())

    def test_update_with_iter_keys_propagates_exceptions_from_dunder_hash(self):
        class C:
            def __hash__(self):
                raise UserWarning("foo")

        class D:
            def keys(self):
                return [C()].__iter__()

            def __getitem__(self, key):
                return "foo"

        with self.assertRaises(UserWarning):
            dict.update({}, D())


class DictItemsTests(unittest.TestCase):
    DictItemsType = type({}.items())

    def test_dunder_and_with_non_dict_items_lhs_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.DictItemsType.__and__({}.keys(), [])
        self.assertIn(
            "'__and__' requires a 'dict_items' object but received a 'dict_keys'",
            str(context.exception),
        )

    def test_dunder_and_with_non_iterable_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            {"hello": "world", "foo": "bar"}.items().__and__(5)
        self.assertEqual("'int' object is not iterable", str(context.exception))

    def test_dunder_and_with_iterable_returns_set_with_intersection(self):
        mapping = {"hello": "world", "foo": "bar"}
        result = mapping.items().__and__([("hello", "world"), ("fizz", "buzz")])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {("hello", "world")})

    def test_dunder_and_with_no_rhs_raises_type_error(self):
        with self.assertRaises(TypeError):
            {"hello": "world"}.items().__and__()

    def test_dunder_and_with_empty_lhs_and_empty_rhs_returns_empty_set(self):
        result = {}.items().__and__(())
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_and_with_empty_lhs_and_non_empty_rhs_returns_empty_set(self):
        result = {}.items().__and__([("hello", "world")])
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_and_with_non_empty_lhs_and_empty_rhs_returns_empty_set(self):
        result = {"hello": "world"}.items().__and__([])
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_eq_with_non_dict_items_lhs_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.DictItemsType.__eq__({}.keys(), [])
        self.assertIn(
            "'__eq__' requires a 'dict_items' object but received a 'dict_keys'",
            str(context.exception),
        )

    def test_dunder_eq_with_no_rhs_raises_type_error(self):
        with self.assertRaises(TypeError):
            {"hello": "world"}.items().__eq__()

    def test_dunder_eq_with_empty_lhs_and_empty_rhs_returns_true(self):
        self.assertIs({}.items().__eq__(set()), True)

    def test_dunder_eq_with_empty_lhs_and_non_empty_rhs_returns_false(self):
        self.assertIs({}.items().__eq__(set("foo")), False)

    def test_dunder_eq_with_non_empty_lhs_and_empty_rhs_returns_false(self):
        self.assertIs({"hello": "world"}.items().__eq__(set()), False)

    def test_dunder_eq_with_non_set_or_dictview_rhs_returns_notimplemented(self):
        self.assertIs({"hello": "world"}.items().__eq__(()), NotImplemented)

    def test_dunder_eq_with_set_rhs(self):
        self.assertIs({"hello": "world"}.items().__eq__({("hello", "world")}), True)

    def test_dunder_eq_with_frozenset_rhs(self):
        self.assertIs(
            {"hello": "world"}.items().__eq__(frozenset({("hello", "world")})), True
        )

    def test_dunder_eq_with_dict_items_rhs(self):
        mapping = {"hello": "world"}
        self.assertIs(mapping.items().__eq__(mapping.items()), True)

    def test_dunder_eq_with_dict_keys_rhs(self):
        mapping = {"hello": "world"}
        other_mapping = {("hello", "world"): "baz"}
        self.assertIs(mapping.items().__eq__(other_mapping.keys()), True)

    def test_dunder_eq_with_dict_values_rhs_returns_notimplemented(self):
        mapping = {"hello": "world"}
        self.assertIs(mapping.items().__eq__(mapping.values()), NotImplemented)

    def test_dunder_eq_with_different_lengths_returns_false(self):
        mapping = {"hello": "world"}
        other_mapping = {"hello": "world", "foo": "bar"}
        self.assertIs(mapping.items().__eq__(other_mapping.items()), False)

    def test_dunder_eq_with_id_equal_but_inequal_element_returns_true(self):
        class C:
            def __eq__(self, other):
                return False

            def __hash__(self):
                return 1

        instance = C()
        self.assertIs({instance: "world"}.items().__eq__({(instance, "world")}), True)

    def test_dunder_repr_with_non_dict_items_lhs_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.DictItemsType.__repr__({}.keys())
        self.assertIn(
            "'__repr__' requires a 'dict_items' object but received a 'dict_keys'",
            str(context.exception),
        )

    def test_dunder_repr_prints_items(self):
        result = repr({"hello": "world", "foo": "bar"}.items())
        self.assertEqual(result, "dict_items([('hello', 'world'), ('foo', 'bar')])")

    def test_dunder_repr_with_recursive_prints_ellipsis(self):
        x = {}
        x[1] = x.items()
        self.assertEqual(repr(x.items()), "dict_items([(1, dict_items([(1, ...)]))])")

    def test_dunder_repr_calls_key_dunder_repr(self):
        class C:
            def __repr__(self):
                return "foo"

        result = repr({"hello": C()}.items())
        self.assertEqual(result, "dict_items([('hello', foo)])")

    def test_recursive_dunder_repr(self):
        circular_mapping = {}
        circular_mapping["hello"] = circular_mapping.items()
        self.assertEqual(
            repr(circular_mapping.items()),
            "dict_items([('hello', dict_items([('hello', ...)]))])",
        )

    def test_dunder_len_with_non_dict_items_lhs_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.DictItemsType.__len__({}.keys())
        self.assertIn(
            "'__len__' requires a 'dict_items' object but received a 'dict_keys'",
            str(context.exception),
        )

    def test_dunder_len_with_non_dict_items_raises_type_error(self):
        dict_items = type({}.items())
        with self.assertRaisesRegex(
            TypeError, "requires a 'dict_items' object but .+ 'int'"
        ):
            dict_items.__len__(5)

    def test_dunder_len_returns_length_of_underlying_dict(self):
        mapping = {"hello": "world", "foo": "bar"}
        items = mapping.items()
        self.assertEqual(items.__len__(), 2)
        mapping["szechuan"] = "broccoli"
        self.assertEqual(items.__len__(), 3)

    def test_dunder_or_with_non_dict_items_lhs_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.DictItemsType.__or__({}.keys(), [])
        self.assertIn(
            "'__or__' requires a 'dict_items' object but received a 'dict_keys'",
            str(context.exception),
        )

    def test_dunder_or_with_non_iterable_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            {"hello": "world", "foo": "bar"}.items().__or__(5)
        self.assertEqual("'int' object is not iterable", str(context.exception))

    def test_dunder_or_with_iterable_returns_set_with_union(self):
        mapping = {"hello": "world", "foo": "bar"}
        result = mapping.items().__or__([("hello", "baz")])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {("hello", "world"), ("foo", "bar"), ("hello", "baz")})

    def test_dunder_or_with_no_rhs_raises_type_error(self):
        with self.assertRaises(TypeError):
            {"hello": "world"}.items().__or__()

    def test_dunder_or_with_empty_lhs_and_empty_rhs_returns_empty_set(self):
        result = {}.items().__or__(())
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_or_with_empty_lhs_and_non_empty_rhs_returns_rhs(self):
        result = {}.items().__or__([("hello", "world")])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {("hello", "world")})

    def test_dunder_or_with_non_empty_lhs_and_empty_rhs_adds_none(self):
        result = {"hello": "world"}.items().__or__([])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {("hello", "world")})

    def test_dunder_ror_with_non_dict_items_lhs_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.DictItemsType.__ror__({}.keys(), [])
        self.assertIn(
            "'__ror__' requires a 'dict_items' object but received a 'dict_keys'",
            str(context.exception),
        )

    def test_dunder_ror_with_non_iterable_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            {"hello": "world", "foo": "bar"}.items().__ror__(5)
        self.assertEqual("'int' object is not iterable", str(context.exception))

    def test_dunder_ror_with_iterable_returns_set_with_union(self):
        mapping = {"hello": "world", "foo": "bar"}
        result = mapping.items().__ror__([("hello", "baz")])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {("hello", "world"), ("foo", "bar"), ("hello", "baz")})

    def test_dunder_ror_with_no_rhs_raises_type_error(self):
        with self.assertRaises(TypeError):
            {"hello": "world"}.items().__ror__()

    def test_dunder_ror_with_empty_lhs_and_empty_rhs_returns_empty_set(self):
        result = {}.items().__ror__(())
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_ror_with_empty_lhs_and_non_empty_rhs_returns_rhs(self):
        result = {}.items().__ror__([("hello", "world")])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {("hello", "world")})

    def test_dunder_ror_with_non_empty_lhs_and_empty_rhs_adds_none(self):
        result = {"hello": "world"}.items().__ror__([])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {("hello", "world")})

    def test_dunder_sub_with_non_dict_items_lhs_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.DictItemsType.__sub__({}.keys(), [])
        self.assertIn(
            "'__sub__' requires a 'dict_items' object but received a 'dict_keys'",
            str(context.exception),
        )

    def test_dunder_sub_with_non_iterable_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            {"hello": "world", "foo": "bar"}.items().__sub__(5)
        self.assertEqual("'int' object is not iterable", str(context.exception))

    def test_dunder_sub_with_nonexistent_pair_does_not_remove_pair(self):
        mapping = {"hello": "world", "foo": "bar"}
        result = mapping.items().__sub__([("hello", "baz")])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {("hello", "world"), ("foo", "bar")})

    def test_dunder_sub_with_list_containing_pair_removes_pair(self):
        mapping = {"hello": "world", "foo": "bar"}
        result = mapping.items().__sub__([("hello", "world")])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {("foo", "bar")})

    def test_dunder_sub_with_list_removes_all_pairs_in_list(self):
        mapping = {"hello": "world", "foo": "bar"}
        result = mapping.items().__sub__([("hello", "world"), ("foo", "bar")])
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_sub_with_no_rhs_raises_type_error(self):
        with self.assertRaises(TypeError):
            {"hello": "world"}.items().__sub__()

    def test_dunder_sub_with_empty_lhs_and_empty_rhs_returns_empty_set(self):
        result = {}.items().__sub__(())
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_sub_with_empty_lhs_and_non_empty_rhs_returns_empty_set(self):
        result = {}.items().__sub__([("hello", "world")])
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_sub_with_non_empty_lhs_and_empty_rhs_removes_none(self):
        result = {"hello": "world"}.items().__sub__([])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {("hello", "world")})

    def test_dunder_xor_with_non_dict_items_lhs_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.DictItemsType.__xor__({}.keys(), [])
        self.assertIn(
            "'__xor__' requires a 'dict_items' object but received a 'dict_keys'",
            str(context.exception),
        )

    def test_dunder_xor_with_non_iterable_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            {"hello": "world", "foo": "bar"}.items().__xor__(5)
        self.assertEqual("'int' object is not iterable", str(context.exception))

    def test_dunder_xor_with_iterable_returns_set_with_xor(self):
        mapping = {"hello": "world", "foo": "bar"}
        result = mapping.items().__xor__([("hello", "world"), ("fizz", "buzz")])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {("foo", "bar"), ("fizz", "buzz")})

    def test_dunder_xor_with_no_rhs_raises_type_error(self):
        with self.assertRaises(TypeError):
            {"hello": "world"}.items().__xor__()

    def test_dunder_xor_with_empty_lhs_and_empty_rhs_returns_empty_set(self):
        result = {}.items().__xor__(())
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_xor_with_empty_lhs_and_non_empty_rhs_returns_rhs(self):
        result = {}.items().__xor__([("hello", "world")])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {("hello", "world")})

    def test_dunder_xor_with_non_empty_lhs_and_empty_rhs_removes_none(self):
        result = {"hello": "world"}.items().__xor__([])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {("hello", "world")})


class DictKeysTests(unittest.TestCase):
    DictKeysType = type({}.keys())

    def test_dunder_and_with_non_dict_keys_lhs_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.DictKeysType.__and__({}.items(), [])
        self.assertIn(
            "'__and__' requires a 'dict_keys' object but received a 'dict_items'",
            str(context.exception),
        )

    def test_dunder_and_with_non_iterable_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            {"hello": "world", "foo": "bar"}.keys().__and__(5)
        self.assertEqual("'int' object is not iterable", str(context.exception))

    def test_dunder_and_with_iterable_returns_set_with_intersection(self):
        mapping = {"hello": "world", "foo": "bar"}
        result = mapping.keys().__and__(["hello", "fizz"])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {"hello"})

    def test_dunder_and_with_no_rhs_raises_type_error(self):
        with self.assertRaises(TypeError):
            {"hello": "world"}.keys().__and__()

    def test_dunder_and_with_empty_lhs_and_empty_rhs_returns_empty_set(self):
        result = {}.keys().__and__(())
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_and_with_empty_lhs_and_non_empty_rhs_returns_empty_set(self):
        result = {}.keys().__and__(["hello"])
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_and_with_non_empty_lhs_and_empty_rhs_returns_empty_set(self):
        result = {"hello": "world"}.keys().__and__([])
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_eq_with_non_dict_keys_lhs_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.DictKeysType.__eq__({}.items(), [])
        self.assertIn(
            "'__eq__' requires a 'dict_keys' object but received a 'dict_items'",
            str(context.exception),
        )

    def test_dunder_eq_with_no_rhs_raises_type_error(self):
        with self.assertRaises(TypeError):
            {"hello": "world"}.keys().__eq__()

    def test_dunder_eq_with_empty_lhs_and_empty_rhs_returns_true(self):
        self.assertIs({}.keys().__eq__(set()), True)

    def test_dunder_eq_with_empty_lhs_and_non_empty_rhs_returns_false(self):
        self.assertIs({}.keys().__eq__(set("foo")), False)

    def test_dunder_eq_with_non_empty_lhs_and_empty_rhs_returns_false(self):
        self.assertIs({"hello": "world"}.keys().__eq__(set()), False)

    def test_dunder_eq_with_non_set_or_dictview_rhs_returns_notimplemented(self):
        self.assertIs({"hello": "world"}.keys().__eq__(()), NotImplemented)

    def test_dunder_eq_with_set_rhs(self):
        self.assertIs({"hello": "world"}.keys().__eq__({"hello"}), True)

    def test_dunder_eq_with_frozenset_rhs(self):
        self.assertIs({"hello": "world"}.keys().__eq__(frozenset({"hello"})), True)

    def test_dunder_eq_with_dict_items_rhs(self):
        self.assertIs({"hello": "world"}.keys().__eq__({"hello": "world"}.keys()), True)

    def test_dunder_eq_with_dict_keys_rhs(self):
        mapping = {("hello", "world"): "baz"}
        other_mapping = {"hello": "world"}
        self.assertIs(mapping.keys().__eq__(other_mapping.items()), True)

    def test_dunder_eq_with_dict_values_rhs_returns_notimplemented(self):
        mapping = {"hello": "world"}
        self.assertIs(mapping.keys().__eq__(mapping.values()), NotImplemented)

    def test_dunder_eq_with_different_lengths_returns_false(self):
        mapping = {"hello": "world"}
        other_mapping = {"hello": "world", "foo": "bar"}
        self.assertIs(mapping.keys().__eq__(other_mapping.keys()), False)

    def test_dunder_eq_with_id_equal_but_inequal_element_returns_true(self):
        class C:
            def __eq__(self, other):
                return False

            def __hash__(self):
                return 1

        instance = C()
        self.assertIs({instance: "world"}.keys().__eq__({instance}), True)

    def test_dunder_repr_with_non_dict_keys_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.DictKeysType.__repr__({}.items())
        self.assertIn(
            "'__repr__' requires a 'dict_keys' object but received a 'dict_items'",
            str(context.exception),
        )

    def test_dunder_repr_prints_keys(self):
        result = repr({"hello": "world", "foo": "bar"}.keys())
        self.assertEqual(result, "dict_keys(['hello', 'foo'])")

    def test_dunder_repr_with_recursive_prints_ellipsis(self):
        x = {}
        x[1] = x.values()
        self.assertEqual(repr(x.values()), "dict_values([dict_values([...])])")

    def test_dunder_repr_calls_key_dunder_repr(self):
        class C:
            def __repr__(self):
                return "foo"

        result = repr({C(): "world"}.keys())
        self.assertEqual(result, "dict_keys([foo])")

    def test_recursive_dunder_repr(self):
        class C:
            def __init__(self, value):
                self.value = value

            def __eq__(self, other):
                return self is other

            def __hash__(self):
                return 5

            def __repr__(self):
                return self.value.__repr__()

        circular_mapping = {}
        circular_mapping[C(circular_mapping.keys())] = 10
        self.assertEqual(repr(circular_mapping.keys()), "dict_keys([dict_keys([...])])")

    def test_dunder_len_with_non_dict_keys_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, "requires a 'dict_keys' object but .+ 'int'"
        ):
            self.DictKeysType.__len__(5)

    def test_dunder_len_returns_length_of_underlying_dict(self):
        mapping = {"hello": "world", "foo": "bar"}
        keys = mapping.keys()
        self.assertEqual(keys.__len__(), 2)
        mapping["szechuan"] = "broccoli"
        self.assertEqual(keys.__len__(), 3)

    def test_dunder_or_with_non_dict_keys_lhs_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.DictKeysType.__or__({}.items(), [])
        self.assertIn(
            "'__or__' requires a 'dict_keys' object but received a 'dict_items'",
            str(context.exception),
        )

    def test_dunder_or_with_non_iterable_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            {"hello": "world", "foo": "bar"}.keys().__or__(5)
        self.assertEqual("'int' object is not iterable", str(context.exception))

    def test_dunder_or_with_iterable_returns_set_with_union(self):
        mapping = {"hello": "world", "foo": "bar"}
        result = mapping.keys().__or__(["baz"])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {"hello", "foo", "baz"})

    def test_dunder_or_with_no_rhs_raises_type_error(self):
        with self.assertRaises(TypeError):
            {"hello": "world"}.keys().__or__()

    def test_dunder_or_with_empty_lhs_and_empty_rhs_returns_empty_set(self):
        result = {}.keys().__or__(())
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_or_with_empty_lhs_and_non_empty_rhs_returns_rhs(self):
        result = {}.keys().__or__(["hello"])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {"hello"})

    def test_dunder_or_with_non_empty_lhs_and_empty_rhs_adds_none(self):
        result = {"hello": "world"}.keys().__or__([])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {"hello"})

    def test_dunder_ror_with_non_dict_keys_lhs_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.DictKeysType.__ror__({}.items(), [])
        self.assertIn(
            "'__ror__' requires a 'dict_keys' object but received a 'dict_items'",
            str(context.exception),
        )

    def test_dunder_ror_with_non_iterable_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            {"hello": "world", "foo": "bar"}.keys().__ror__(5)
        self.assertEqual("'int' object is not iterable", str(context.exception))

    def test_dunder_ror_with_iterable_returns_set_with_union(self):
        mapping = {"hello": "world", "foo": "bar"}
        result = mapping.keys().__ror__(["baz"])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {"hello", "foo", "baz"})

    def test_dunder_ror_with_no_rhs_raises_type_error(self):
        with self.assertRaises(TypeError):
            {"hello": "world"}.keys().__ror__()

    def test_dunder_ror_with_empty_lhs_and_empty_rhs_returns_empty_set(self):
        result = {}.keys().__ror__(())
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_ror_with_empty_lhs_and_non_empty_rhs_returns_rhs(self):
        result = {}.keys().__ror__(["hello"])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {"hello"})

    def test_dunder_ror_with_non_empty_lhs_and_empty_rhs_adds_none(self):
        result = {"hello": "world"}.keys().__ror__([])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {"hello"})

    def test_dunder_sub_with_non_dict_keys_lhs_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.DictKeysType.__sub__({}.items(), [])
        self.assertIn(
            "'__sub__' requires a 'dict_keys' object but received a 'dict_items'",
            str(context.exception),
        )

    def test_dunder_sub_with_non_iterable_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            {"hello": "world", "foo": "bar"}.keys().__sub__(5)
        self.assertEqual("'int' object is not iterable", str(context.exception))

    def test_dunder_sub_with_nonexistent_key_does_not_remove_pair(self):
        mapping = {"hello": "world", "foo": "bar"}
        result = mapping.keys().__sub__(["baz"])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {"hello", "foo"})

    def test_dunder_sub_with_list_containing_key_removes_key(self):
        mapping = {"hello": "world", "foo": "bar"}
        result = mapping.keys().__sub__(["hello"])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {"foo"})

    def test_dunder_sub_with_list_removes_all_keys_in_list(self):
        mapping = {"hello": "world", "foo": "bar"}
        result = mapping.keys().__sub__(["hello", "foo"])
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_sub_with_no_rhs_raises_type_error(self):
        with self.assertRaises(TypeError):
            {"hello": "world"}.keys().__sub__()

    def test_dunder_sub_with_empty_lhs_and_empty_rhs_returns_empty_set(self):
        result = {}.keys().__sub__(())
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_sub_with_empty_lhs_and_non_empty_rhs_returns_empty_set(self):
        result = {}.keys().__sub__([("hello", "world")])
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_sub_with_non_empty_lhs_and_empty_rhs_removes_none(self):
        result = {"hello": "world"}.keys().__sub__([])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {"hello"})

    def test_dunder_xor_with_non_dict_keys_lhs_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.DictKeysType.__xor__({}.items(), [])
        self.assertIn(
            "'__xor__' requires a 'dict_keys' object but received a 'dict_items'",
            str(context.exception),
        )

    def test_dunder_xor_with_non_iterable_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            {"hello": "world", "foo": "bar"}.keys().__xor__(5)
        self.assertEqual("'int' object is not iterable", str(context.exception))

    def test_dunder_xor_with_iterable_returns_set_with_xor(self):
        mapping = {"hello": "world", "foo": "bar"}
        result = mapping.keys().__xor__(["hello", "fizz"])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {"foo", "fizz"})

    def test_dunder_xor_with_no_rhs_raises_type_error(self):
        with self.assertRaises(TypeError):
            {"hello": "world"}.keys().__xor__()

    def test_dunder_xor_with_empty_lhs_and_empty_rhs_returns_empty_set(self):
        result = {}.keys().__xor__(())
        self.assertIsInstance(result, set)
        self.assertEqual(result, set())

    def test_dunder_xor_with_empty_lhs_and_non_empty_rhs_returns_rhs(self):
        result = {}.keys().__xor__(["hello"])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {"hello"})

    def test_dunder_xor_with_non_empty_lhs_and_empty_rhs_removes_none(self):
        result = {"hello": "world"}.keys().__xor__([])
        self.assertIsInstance(result, set)
        self.assertEqual(result, {"hello"})


class DictValuesTests(unittest.TestCase):
    def test_dunder_repr_prints_values(self):
        result = repr({"hello": "world", "foo": "bar"}.values())
        self.assertEqual(result, "dict_values(['world', 'bar'])")

    def test_dunder_repr_calls_key_dunder_repr(self):
        class C:
            def __repr__(self):
                return "foo"

        result = repr({"hello": C()}.values())
        self.assertEqual(result, "dict_values([foo])")

    def test_recursive_dunder_repr(self):
        circular_mapping = {}
        circular_mapping["hello"] = circular_mapping.values()
        self.assertEqual(
            repr(circular_mapping.values()), "dict_values([dict_values([...])])"
        )

    def test_dunder_len_with_non_dict_values_raises_type_error(self):
        dict_values = type({}.values())
        with self.assertRaisesRegex(
            TypeError, "requires a 'dict_values' object but .+ 'int'"
        ):
            dict_values.__len__(5)

    def test_dunder_len_returns_length_of_underlying_dict(self):
        mapping = {"hello": "world", "foo": "bar"}
        values = mapping.values()
        self.assertEqual(values.__len__(), 2)
        mapping["szechuan"] = "broccoli"
        self.assertEqual(values.__len__(), 3)


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
        import errno

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

    def test_oserror_with_errno_enoent_returns_file_not_found_error(self):
        import errno

        result = OSError(errno.ENOENT, "foo", "bar.py")
        self.assertIsInstance(result, FileNotFoundError)
        self.assertEqual(result.errno, errno.ENOENT)
        self.assertEqual(result.strerror, "foo")
        self.assertEqual(result.filename, "bar.py")

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

    def test_inherits_compile_flags(self):
        from types import CodeType
        import __future__

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
        from types import ModuleType
        from types import CodeType

        code = compile("result = foo", "", "exec", 0, True, -1)
        self.assertIsInstance(code, CodeType)
        module = ModuleType("")
        module.foo = 99
        self.assertIs(exec(code, module.__dict__), None)
        self.assertEqual(module.result, 99)

    def test_inherits_compile_flags(self):
        from types import ModuleType
        from types import CodeType
        import __future__

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


class FloatTests(unittest.TestCase):
    def test_dunder_divmod_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__divmod__(1, 1.0)

    def test_dunder_divmod_with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__divmod__(1.0, "1"), NotImplemented)

    def test_dunder_divmod_with_zero_denominator_raises_zero_division_error(self):
        with self.assertRaises(ZeroDivisionError):
            float.__divmod__(1.0, 0.0)

    def test_dunder_divmod_with_negative_zero_denominator_raises_zero_division_error(
        self,
    ):
        with self.assertRaises(ZeroDivisionError):
            float.__divmod__(1.0, -0.0)

    def test_dunder_divmod_with_zero_numerator(self):
        floordiv, remainder = float.__divmod__(0.0, 4.0)
        self.assertEqual(floordiv, 0.0)
        self.assertEqual(remainder, 0.0)

    def test_dunder_divmod_with_positive_denominator_positive_numerator(self):
        floordiv, remainder = float.__divmod__(3.25, 1.0)
        self.assertEqual(floordiv, 3.0)
        self.assertEqual(remainder, 0.25)

    def test_dunder_divmod_with_negative_denominator_positive_numerator(self):
        floordiv, remainder = float.__divmod__(-3.25, 1.0)
        self.assertEqual(floordiv, -4.0)
        self.assertEqual(remainder, 0.75)

    def test_dunder_divmod_with_negative_denominator_negative_numerator(self):
        floordiv, remainder = float.__divmod__(-3.25, -1.0)
        self.assertEqual(floordiv, 3.0)
        self.assertEqual(remainder, -0.25)

    def test_dunder_divmod_with_positive_denominator_negative_numerator(self):
        floordiv, remainder = float.__divmod__(3.25, -1.0)
        self.assertEqual(floordiv, -4.0)
        self.assertEqual(remainder, -0.75)

    def test_dunder_divmod_with_nan_denominator(self):
        import math

        floordiv, remainder = float.__divmod__(3.25, float("nan"))
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_nan_numerator(self):
        import math

        floordiv, remainder = float.__divmod__(float("nan"), 1.0)
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_negative_nan_denominator(self):
        import math

        floordiv, remainder = float.__divmod__(3.25, float("-nan"))
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_negative_nan_numerator(self):
        import math

        floordiv, remainder = float.__divmod__(float("-nan"), 1.0)
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_inf_denominator(self):
        floordiv, remainder = float.__divmod__(3.25, float("inf"))
        self.assertEqual(floordiv, 0.0)
        self.assertEqual(remainder, 3.25)

    def test_dunder_divmod_with_inf_numerator(self):
        import math

        floordiv, remainder = float.__divmod__(float("inf"), 1.0)
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_negative_inf_denominator(self):
        floordiv, remainder = float.__divmod__(3.25, float("-inf"))
        self.assertEqual(floordiv, -1.0)
        self.assertEqual(remainder, -float("inf"))

    def test_dunder_divmod_with_negative_inf_numerator(self):
        import math

        floordiv, remainder = float.__divmod__(float("-inf"), 1.0)
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_big_numerator(self):
        floordiv, remainder = float.__divmod__(1e200, 1.0)
        self.assertEqual(floordiv, 1e200)
        self.assertEqual(remainder, 0.0)

    def test_dunder_divmod_with_big_denominator(self):
        floordiv, remainder = float.__divmod__(1.0, 1e200)
        self.assertEqual(floordiv, 0.0)
        self.assertEqual(remainder, 1.0)

    def test_dunder_divmod_with_negative_zero_numerator(self):
        floordiv, remainder = float.__divmod__(-0.0, 4.0)
        self.assertTrue(str(floordiv) == "-0.0")
        self.assertEqual(remainder, 0.0)

    def test_dunder_floordiv_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__floordiv__(1, 1.0)

    def test_dunder_floordiv_with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__floordiv__(1.0, "1"), NotImplemented)

    def test_dunder_floordiv_with_zero_denominator_raises_zero_division_error(self):
        with self.assertRaises(ZeroDivisionError):
            float.__floordiv__(1.0, 0.0)

    def test_dunder_floordiv_returns_floor_quotient(self):
        self.assertEqual(float.__floordiv__(3.25, -1.0), -4.0)

    def test_dunder_getformat_with_float_or_double_returns_format(self):
        import sys

        self.assertEqual(float.__getformat__("double"), f"IEEE, {sys.byteorder}-endian")
        self.assertEqual(float.__getformat__("float"), f"IEEE, {sys.byteorder}-endian")

    def test_dunder_getformat_with_non_float_or_double_raises_value_error(self):
        with self.assertRaises(ValueError):
            float.__getformat__("unknown")

    def test_dunder_int_with_non_float_raise_type_error(self):
        with self.assertRaises(TypeError) as context:
            float.__int__("not a float")
        self.assertIn("'__int__' requires a 'float' object", str(context.exception))

    def test_dunder_hash_from_non_finites_returns_well_known_values(self):
        import sys

        self.assertEqual(float.__hash__(float("inf")), sys.hash_info.inf)
        self.assertEqual(float.__hash__(float("-inf")), -(sys.hash_info.inf))
        self.assertEqual(float.__hash__(float("nan")), sys.hash_info.nan)

    def test_dunder_hash_returns_int(self):
        self.assertIsInstance(float.__hash__(0.0), int)
        self.assertIsInstance(float.__hash__(-1.0), int)
        self.assertIsInstance(float.__hash__(42.34532), int)
        self.assertIsInstance(float.__hash__(1.79769e308), int)

    def test_dunder_hash_matches_int_dunder_hash(self):
        self.assertEqual(float.__hash__(0.0), int.__hash__(0))
        self.assertEqual(float.__hash__(-0.0), int.__hash__(0))
        self.assertEqual(float.__hash__(1.0), int.__hash__(1))
        self.assertEqual(float.__hash__(-1.0), int.__hash__(-1))
        self.assertEqual(float.__hash__(42.0), int.__hash__(42))
        self.assertEqual(float.__hash__(-99.0), int.__hash__(-99))
        self.assertEqual(
            float.__hash__(9.313203665422767e55), int.__hash__(0x3CC58055CE060C << 132)
        )
        self.assertEqual(
            float.__hash__(-7.26682022207011e41), int.__hash__(-0x85786CAA960EE << 88)
        )

    def test_dunder_hash_with_negative_exponent_returns_int(self):
        self.assertEqual(float.__hash__(0.5), 1 << 60)

        import sys

        # the following only works for the given modulus.
        self.assertEqual(sys.hash_info.modulus, (1 << 61) - 1)
        self.assertEqual(float.__hash__(6.716542360700249e-22), 0xCAFEBABE00000)

    def test_dunder_hash_with_subnormals_returns_int(self):
        import sys

        # The following tests assume a specific modulus and need to be adapted
        # if that changes.
        self.assertEqual(sys.hash_info.modulus, (1 << 61) - 1)
        # This is the smallest number that is not a subnormal yet.
        self.assertEqual(float.__hash__(2.2250738585072014e-308), 1 << 15)
        # The following are subnormal numbers:
        self.assertEqual(float.__hash__(1.1125369292536007e-308), 1 << 14)
        self.assertEqual(float.__hash__(2.225073858507201e-308), 0x1FFFFFFFFF007FFF)
        self.assertEqual(float.__hash__(5e-324), 1 << 24)
        # This is the smallest number that is not a subnormal yet.
        self.assertEqual(float.__hash__(-2.2250738585072014e-308), -1 << 15)
        # The following are subnormal numbers:
        self.assertEqual(float.__hash__(-1.1125369292536007e-308), -1 << 14)
        self.assertEqual(float.__hash__(-2.225073858507201e-308), -0x1FFFFFFFFF007FFF)
        self.assertEqual(float.__hash__(-5e-324), -1 << 24)

    def test_dunder_hash_matches_bool_dunder_hash(self):
        self.assertEqual(float.__hash__(float(True)), bool.__hash__(True))
        self.assertEqual(float.__hash__(float(False)), bool.__hash__(False))

    def test_dunder_hash_with_float_subclass_returns_int(self):
        class C(float):
            pass

        self.assertEqual(float.__hash__(C(-77.0)), -77)

    def test_dunder_hash_with_non_float_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.assertEqual(float.__hash__("not a float"))
        self.assertIn("'__hash__' requires a 'float' object", str(context.exception))
        self.assertIn("'str'", str(context.exception))

    def test_dunder_mod_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__mod__(1, 1.0)

    def test_dunder_mod_with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__mod__(1.0, "1"), NotImplemented)

    def test_dunder_mod_with_zero_denominator_raises_zero_division_error(self):
        with self.assertRaises(ZeroDivisionError):
            float.__mod__(1.0, 0.0)

    def test_dunder_mod_returns_remainder(self):
        self.assertEqual(float.__mod__(3.25, -1.0), -0.75)

    def test_dunder_new_with_non_class_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__new__("not a type")

    def test_dunder_new_with_non_float_subclass_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__new__(int)

    def test_dunder_new_with_default_argument_returns_zero(self):
        self.assertEqual(float(), 0.0)

    def test_dunder_new_with_float_returns_same_value(self):
        self.assertEqual(float(1.0), 1.0)

    def test_dunder_new_with_invalid_str_raises_value_error(self):
        with self.assertRaises(ValueError):
            float("1880.3a01")

    def test_dunder_new_with_str_returns_float(self):
        self.assertEqual(float("1.0"), 1.0)

    def test_dunder_new_with_huge_positive_str_returns_inf(self):
        self.assertEqual(float("1.18973e+4932"), float("inf"))

    def test_dunder_new_with_huge_negative_str_returns_negative_inf(self):
        self.assertEqual(float("-1.18973e+4932"), float("-inf"))

    def test_dunder_new_with_invalid_bytes_raises_value_error(self):
        with self.assertRaises(ValueError):
            float(b"1880.3a01")

    def test_dunder_new_with_bytes_returns_float(self):
        self.assertEqual(float(b"1.0"), 1.0)

    def test_dunder_new_with_huge_positive_bytes_returns_inf(self):
        self.assertEqual(float(b"1.18973e+4932"), float("inf"))

    def test_dunder_new_with_huge_negative_bytes_returns_negative_inf(self):
        self.assertEqual(float(b"-1.18973e+4932"), float("-inf"))

    def test_dunder_new_with_float_subclass_calls_dunder_float(self):
        class C(float):
            def __float__(self):
                return 1.0

        c = C()
        self.assertEqual(c, 0.0)
        self.assertEqual(float(c), 1.0)

    def test_dunder_new_with_str_subclass_calls_dunder_float(self):
        class C(str):
            def __float__(self):
                return 1.0

        c = C("0.0")
        self.assertEqual(float(c), 1.0)

    def test_dunder_new_with_int_calls_dunder_float(self):
        self.assertEqual(float(1), 1.0)

    def test_dunder_new_with_raising_descriptor_propagates_exception(self):
        class Desc:
            def __get__(self, obj, type):
                raise IndexError()

        class C:
            __float__ = Desc()

        self.assertRaises(IndexError, float, C())

    def test_dunder_new_without_dunder_float_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            float([])
        self.assertEqual(
            str(context.exception),
            "float() argument must be a string or a number, not 'list'",
        )

    def test_dunder_pow_with_str_returns_float(self):
        result = float.__pow__(2.0, 4)
        self.assertIs(type(result), float)
        self.assertEqual(result, 16.0)

    def test_dunder_pow_with_third_arg_int_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            float.__pow__(2.0, 4.0, 4.0)
        self.assertIn(
            "pow() 3rd argument not allowed unless all arguments are integers",
            str(context.exception),
        )

    def test_dunder_pow_with_third_arg_none_returns_power_of_first_two_args(self):
        self.assertEqual(float.__pow__(2.0, 4.0, None), 16.0)

    def test_dunder_rdivmod_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__rdivmod__(1, 1.0)

    def test_dunder_rdivmod_with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__rdivmod__(1.0, "1"), NotImplemented)

    def test_dunder_rdivmod_with_int_returns_same_result_as_divmod_with_reversed_args(
        self,
    ):
        self.assertEqual(float.__rdivmod__(1.0, 3), float.__divmod__(float(3), 1.0))

    def test_dunder_rdivmod_returns_same_result_as_divmod_with_reversed_args(self):
        self.assertEqual(float.__rdivmod__(1.0, 3.25), float.__divmod__(3.25, 1.0))

    def test_repr_with_infinity_returns_string(self):
        self.assertEqual(float.__repr__(float("inf")), "inf")
        self.assertEqual(float.__repr__(-float("inf")), "-inf")

    def test_repr_with_nan_returns_nan(self):
        self.assertEqual(float.__repr__(float("nan")), "nan")
        self.assertEqual(float.__repr__(float("-nan")), "nan")

    def test_repr_returns_string_without_exponent(self):
        self.assertEqual(float.__repr__(0.0), "0.0")
        self.assertEqual(float.__repr__(-0.0), "-0.0")
        self.assertEqual(float.__repr__(1.0), "1.0")
        self.assertEqual(float.__repr__(-1.0), "-1.0")
        self.assertEqual(float.__repr__(42.5), "42.5")
        self.assertEqual(float.__repr__(1.234567891234567), "1.234567891234567")
        self.assertEqual(float.__repr__(-1.234567891234567), "-1.234567891234567")
        self.assertEqual(float.__repr__(9.99999999999999e15), "9999999999999990.0")
        self.assertEqual(float.__repr__(0.0001), "0.0001")

    def test_repr_returns_string_with_exponent(self):
        self.assertEqual(float.__repr__(1e16), "1e+16")
        self.assertEqual(float.__repr__(0.00001), "1e-05")
        self.assertEqual(float.__repr__(1e100), "1e+100")
        self.assertEqual(float.__repr__(1e-88), "1e-88")
        self.assertEqual(
            float.__repr__(1.23456789123456789e123), "1.2345678912345679e+123"
        )
        self.assertEqual(
            float.__repr__(-1.23456789123456789e-123), "-1.2345678912345678e-123"
        )

    def test_dunder_rfloordiv_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__rfloordiv__(1, 1.0)

    def test_dunder_rfloordiv_with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__rfloordiv__(1.0, "1"), NotImplemented)

    def test_dunder_rfloordiv_with_int_returns_same_result_as_floordiv(self):
        self.assertEqual(float.__rfloordiv__(1.0, 3), float.__floordiv__(float(3), 1.0))

    def test_dunder_rfloordiv_returns_same_result_as_floordiv_for_float_other(self):
        self.assertEqual(float.__rfloordiv__(1.0, 3.25), float.__floordiv__(3.25, 1.0))

    def test_dunder_rmod_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__rmod__(1, 1.0)

    def test_dunder_rmod__with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__rmod__(1.0, "1"), NotImplemented)

    def test_dunder_rmod_with_int_returns_same_result_as_mod_with_reversed_args(self):
        self.assertEqual(float.__rmod__(1.0, 3), float.__mod__(float(3), 1.0))

    def test_dunder_rmod_returns_same_result_as_mod_for_float_other(self):
        self.assertEqual(float.__rmod__(1.0, 3.25), float.__mod__(3.25, 1.0))

    def test_dunder_round_with_one_arg_returns_int(self):
        self.assertEqual(float.__round__(0.0), 0)
        self.assertIsInstance(float.__round__(0.0), int)
        self.assertEqual(float.__round__(-0.0), 0)
        self.assertIsInstance(float.__round__(-0.0), int)
        self.assertEqual(float.__round__(1.0), 1)
        self.assertIsInstance(float.__round__(1.0), int)
        self.assertEqual(float.__round__(-1.0), -1)
        self.assertIsInstance(float.__round__(-1.0), int)
        self.assertEqual(float.__round__(42.42), 42)
        self.assertIsInstance(float.__round__(42.42), int)
        self.assertEqual(float.__round__(0.4), 0)
        self.assertEqual(float.__round__(0.5), 0)
        self.assertEqual(float.__round__(0.5000000000000001), 1)
        self.assertEqual(float.__round__(1.49), 1)
        self.assertEqual(float.__round__(1.5), 2)
        self.assertEqual(float.__round__(1.5000000000000001), 2)
        self.assertEqual(
            float.__round__(1.234567e200),
            123456699999999995062622360556161455756457158443485858665105941107312145749402909576243454437530421952327149599911208391362816498839992520580209467560546813973197632314335145381120371005964774514098176,  # noqa: B950
        )
        self.assertEqual(float.__round__(-13.4, None), -13)
        self.assertIsInstance(float.__round__(-13.4, None), int)

    def test_dunder_round_with_float_subclass_returns_int(self):
        class C(float):
            pass

        self.assertEqual(float.__round__(C(-7654321.7654321)), -7654322)

    def test_dunder_round_with_one_arg_raises_error(self):
        with self.assertRaises(ValueError) as context:
            float.__round__(float("nan"))
        self.assertEqual(str(context.exception), "cannot convert float NaN to integer")
        with self.assertRaises(OverflowError) as context:
            float.__round__(float("inf"))
        self.assertEqual(
            str(context.exception), "cannot convert float infinity to integer"
        )
        with self.assertRaises(OverflowError) as context:
            float.__round__(float("-inf"))
        self.assertEqual(
            str(context.exception), "cannot convert float infinity to integer"
        )

    def test_dunder_round_with_two_args_returns_float(self):
        self.assertEqual(float.__round__(0.0, 0), 0.0)
        self.assertIsInstance(float.__round__(0.0, 0), float)
        self.assertEqual(float.__round__(-0.0, 1), 0.0)
        self.assertIsInstance(float.__round__(-0.0, 1), float)
        self.assertEqual(float.__round__(1.0, 0), 1.0)
        self.assertIsInstance(float.__round__(1.0, 0), float)
        self.assertEqual(float.__round__(-77441.7, -2), -77400.0)
        self.assertIsInstance(float.__round__(-77441.7, -2), float)

        self.assertEqual(float.__round__(12.34567, -(1 << 200)), 0.0)
        self.assertEqual(float.__round__(12.34567, -50), 0.0)
        self.assertEqual(float.__round__(12.34567, -2), 0.0)
        self.assertEqual(float.__round__(12.34567, -1), 10.0)
        self.assertEqual(float.__round__(12.34567, 0), 12.0)
        self.assertEqual(float.__round__(12.34567, 1), 12.3)
        self.assertEqual(float.__round__(12.34567, 2), 12.35)
        self.assertEqual(float.__round__(12.34567, 3), 12.346)
        self.assertEqual(float.__round__(12.34567, 4), 12.3457)
        self.assertEqual(float.__round__(12.34567, 50), 12.34567)
        self.assertEqual(float.__round__(12.34567, 1 << 200), 12.34567)

        self.assertEqual(float("inf"), float("inf"))
        self.assertEqual(float("-inf"), -float("inf"))

        float_max = 1.7976931348623157e308
        self.assertEqual(float.__round__(float_max, -309), 0.0)
        self.assertEqual(float.__round__(float_max, -303), 1.79769e308)

    def test_dunder_round_with_two_args_returns_nan(self):
        import math

        self.assertTrue(math.isnan(float.__round__(float("nan"), 2)))

    def test_dunder_round_with_two_args_raises_error(self):
        float_max = 1.7976931348623157e308
        with self.assertRaises(OverflowError) as context:
            float.__round__(float_max, -308)
        self.assertEqual(str(context.exception), "rounded value too large to represent")

    def test_dunder_rpow_with_non_float_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__rpow__(None, 2.0)

    def test_dunder_rpow_with_float_float_returns_result_from_pow_with_swapped_args(
        self,
    ):
        self.assertEqual(float.__rpow__(2.0, 5.0), float.__pow__(5.0, 2.0))

    def test_dunder_rpow_with_float_int_returns_result_from_pow_with_swapped_args(self):
        result = float.__rpow__(2.0, 5)
        self.assertIsInstance(result, float)
        self.assertEqual(result, float.__pow__(5.0, 2.0))

    def test_dunder_rpow_with_third_arg_int_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            float.__rpow__(2.0, 4.0, 4.0)
        self.assertIn(
            "pow() 3rd argument not allowed unless all arguments are integers",
            str(context.exception),
        )

    def test_dunder_trunc_returns_int(self):
        self.assertEqual(float.__trunc__(0.0), 0)
        self.assertEqual(float.__trunc__(-0.0), 0)
        self.assertEqual(float.__trunc__(1.0), 1)
        self.assertEqual(float.__trunc__(-1.0), -1)
        self.assertEqual(float.__trunc__(42.12345), 42)
        self.assertEqual(float.__trunc__(1.6069380442589903e60), 1 << 200)
        self.assertEqual(float.__trunc__(1e-20), 0)
        self.assertEqual(float.__trunc__(-1e-20), 0)
        self.assertIsInstance(float.__trunc__(0.0), int)
        self.assertIsInstance(float.__trunc__(1.6069380442589903e60), int)
        self.assertIsInstance(float.__trunc__(1e-20), int)

    def test_dunder_trunc_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            float.__trunc__(float("nan"))
        self.assertEqual(str(context.exception), "cannot convert float NaN to integer")
        with self.assertRaises(OverflowError) as context:
            float.__trunc__(float("inf"))
        self.assertEqual(
            str(context.exception), "cannot convert float infinity to integer"
        )
        with self.assertRaises(OverflowError) as context:
            float.__trunc__(float("-inf"))
        self.assertEqual(
            str(context.exception), "cannot convert float infinity to integer"
        )

    def test_hex_with_positive_zero_returns_positive_zero(self):
        self.assertEqual(float(0).hex(), "0x0.0p+0")

    def test_hex_with_negative_zero_returns_negative_zero(self):
        self.assertEqual(float(-0.0).hex(), "-0x0.0p+0")

    def test_hex_with_positive_infinite_returns_inf(self):
        self.assertEqual(float("inf").hex(), "inf")

    def test_hex_with_negative_infinite_returns_minus_inf(self):
        self.assertEqual(float("-inf").hex(), "-inf")

    def test_hex_with_nan_returns_nan(self):
        self.assertEqual(float("nan").hex(), "nan")

    def test_hex_with_positive_decimal_returns_hex_string(self):
        self.assertEqual(float(3.14159).hex(), "0x1.921f9f01b866ep+1")

    def test_hex_with_negative_decimal_returns_hex_string(self):
        self.assertEqual(float(-0.1).hex(), "-0x1.999999999999ap-4")

    def test_hex_with_string_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.hex("")


class FloatDunderFormatTests(unittest.TestCase):
    def test_empty_format_returns_str(self):
        self.assertEqual(float.__format__(0.0, ""), "0.0")
        self.assertEqual(float.__format__(1.2, ""), "1.2")
        self.assertEqual(float.__format__(-1.6, ""), "-1.6")
        self.assertEqual(float("NaN").__format__(""), "nan")
        self.assertEqual(float("inf").__format__(""), "inf")

    def test_empty_format_with_subclass_calls_dunder_str(self):
        class C(float):
            def __str__(self):
                return "foobar"

        self.assertEqual(float.__format__(C(0.0), ""), "foobar")
        self.assertEqual(float.__format__(C("nan"), ""), "foobar")
        self.assertEqual(float.__format__(C("inf"), ""), "foobar")

    def test_nonempty_format_with_subclass_uses_float_value(self):
        class C(float):
            def __str__(self):
                return "foobar"

        self.assertEqual(float.__format__(C(0.0), "e"), "0.000000e+00")
        self.assertEqual(float.__format__(C("nan"), "F"), "NAN")
        self.assertEqual(float.__format__(C("inf"), "g"), "inf")

    def test_e_format_returns_str(self):
        self.assertEqual(float.__format__(0.0, "e"), "0.000000e+00")
        self.assertEqual(float.__format__(-0.0, "e"), "-0.000000e+00")
        self.assertEqual(float.__format__(0.0025, "e"), "2.500000e-03")
        self.assertEqual(float.__format__(-1000.0001, "e"), "-1.000000e+03")
        self.assertEqual(float.__format__(2.0 ** 64, "e"), "1.844674e+19")
        self.assertEqual(float("NAN").__format__("e"), "nan")
        self.assertEqual(float("-INF").__format__("e"), "-inf")

    def test_big_e_format_returns_str(self):
        self.assertEqual(float.__format__(0.0, "E"), "0.000000E+00")
        self.assertEqual(float.__format__(-0.0, "E"), "-0.000000E+00")
        self.assertEqual(float.__format__(0.0025, "E"), "2.500000E-03")
        self.assertEqual(float.__format__(-1000.0001, "E"), "-1.000000E+03")
        self.assertEqual(float.__format__(2.0 ** 64, "E"), "1.844674E+19")
        self.assertEqual(float("nan").__format__("E"), "NAN")
        self.assertEqual(float("-inf").__format__("E"), "-INF")

    def test_f_format_returns_str(self):
        self.assertEqual(float.__format__(0.0, "f"), "0.000000")
        self.assertEqual(float.__format__(-0.0, "f"), "-0.000000")
        self.assertEqual(float.__format__(0.0025, "f"), "0.002500")
        self.assertEqual(float.__format__(-1000.0001, "f"), "-1000.000100")
        self.assertEqual(
            float.__format__(2.0 ** 64, "f"), "18446744073709551616.000000"
        )
        self.assertEqual(float("NaN").__format__("f"), "nan")
        self.assertEqual(float("INF").__format__("f"), "inf")

    def test_big_f_format_returns_str(self):
        self.assertEqual(float.__format__(0.0, "F"), "0.000000")
        self.assertEqual(float.__format__(-0.0, "F"), "-0.000000")
        self.assertEqual(float.__format__(0.0025, "F"), "0.002500")
        self.assertEqual(float.__format__(-1000.0001, "F"), "-1000.000100")
        self.assertEqual(
            float.__format__(2.0 ** 64, "F"), "18446744073709551616.000000"
        )
        self.assertEqual(float("NaN").__format__("F"), "NAN")
        self.assertEqual(float("iNf").__format__("F"), "INF")

    def test_g_format_returns_str(self):
        self.assertEqual(float.__format__(0.0, "g"), "0")
        self.assertEqual(float.__format__(-0.0, "g"), "-0")
        self.assertEqual(float.__format__(0.00025, "g"), "0.00025")
        self.assertEqual(float.__format__(0.000025, "g"), "2.5e-05")
        self.assertEqual(float.__format__(-1000.0001, "g"), "-1000")
        self.assertEqual(float.__format__(123456.789, "g"), "123457")
        self.assertEqual(float.__format__(1234567.89, "g"), "1.23457e+06")
        self.assertEqual(float.__format__(2.0 ** 64, "g"), "1.84467e+19")
        self.assertEqual(float("NaN").__format__("g"), "nan")
        self.assertEqual(float("INF").__format__("g"), "inf")

    def test_big_g_format_returns_str(self):
        self.assertEqual(float.__format__(0.0, "G"), "0")
        self.assertEqual(float.__format__(-0.0, "G"), "-0")
        self.assertEqual(float.__format__(0.00025, "G"), "0.00025")
        self.assertEqual(float.__format__(0.000025, "G"), "2.5E-05")
        self.assertEqual(float.__format__(-1000.0001, "G"), "-1000")
        self.assertEqual(float.__format__(123456.789, "G"), "123457")
        self.assertEqual(float.__format__(1234567.89, "G"), "1.23457E+06")
        self.assertEqual(float.__format__(2.0 ** 64, "G"), "1.84467E+19")
        self.assertEqual(float("Nan").__format__("G"), "NAN")
        self.assertEqual(float("Inf").__format__("G"), "INF")

    # TODO(T52759101): test 'n' uses locale

    def test_percent_format_returns_str(self):
        self.assertEqual(float.__format__(0.0, "%"), "0.000000%")
        self.assertEqual(float.__format__(-0.0, "%"), "-0.000000%")
        self.assertEqual(float.__format__(0.0025, "%"), "0.250000%")
        self.assertEqual(float.__format__(-1000.0001, "%"), "-100000.010000%")
        self.assertEqual(float.__format__(2.0 ** 10, "%"), "102400.000000%")
        self.assertEqual(float("NaN").__format__("%"), "nan%")
        self.assertEqual(float("INF").__format__("%"), "inf%")

    def test_missing_precision_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            float.__format__(0.0, ".f")

        self.assertEqual("Format specifier missing precision", str(context.exception))

    def test_large_integer_precision_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            float.__format__(0.0, ".2147483648f")

        self.assertEqual("precision too big", str(context.exception))

    def test_precision_determines_remainder_length(self):
        self.assertEqual(float.__format__(0.0, ".0e"), "0e+00")
        self.assertEqual(float.__format__(123.0, ".0E"), "1E+02")
        self.assertEqual(float.__format__(4.56, ".0f"), "5")
        self.assertEqual(float.__format__(78.9, ".0F"), "79")
        self.assertEqual(float.__format__(71.0, ".0g"), "7e+01")
        self.assertEqual(float.__format__(0.025, ".0G"), "0.03")
        self.assertEqual(float.__format__(0.005, ".0%"), "0%")
        self.assertEqual(float.__format__(0.0051, ".0%"), "1%")

        self.assertEqual(float.__format__(-0.0, ".2e"), "-0.00e+00")
        self.assertEqual(float.__format__(123.0, ".1E"), "1.2E+02")
        self.assertEqual(float.__format__(4.56, ".10f"), "4.5600000000")
        self.assertEqual(float.__format__(0.00000000000789, ".8F"), "0.00000000")
        self.assertEqual(float.__format__(71.0, ".4g"), "71")
        self.assertEqual(float.__format__(0.000025, ".5G"), "2.5E-05")
        self.assertEqual(float.__format__(0.0005, ".4%"), "0.0500%")

        self.assertEqual(float.__format__(123.456, ".4"), "123.5")
        self.assertEqual(float.__format__(1234.56, ".4"), "1.235e+03")
        self.assertEqual(float.__format__(12345.6, ".4"), "1.235e+04")

    def test_grouping_by_thousands(self):
        self.assertEqual(float.__format__(12345678.9, "_e"), "1.234568e+07")
        self.assertEqual(float.__format__(123456.789, "_E"), "1.234568E+05")
        self.assertEqual(float.__format__(12345678.9, "_f"), "12_345_678.900000")
        self.assertEqual(float.__format__(123456.789, ",F"), "123,456.789000")
        self.assertEqual(float.__format__(12345678.9, ",g"), "1.23457e+07")
        self.assertEqual(float.__format__(123456.789, ",G"), "123,457")
        self.assertEqual(float.__format__(123456.789, "_%"), "12_345_678.900000%")

    def test_padding(self):
        self.assertEqual(float.__format__(123.456, "3e"), "1.234560e+02")
        self.assertEqual(float.__format__(0.0123456789, ">11.4E"), " 1.2346E-02")
        self.assertEqual(float.__format__(0.0, "2f"), "0.000000")
        self.assertEqual(float.__format__(-0.0, "7.2F"), "  -0.00")
        self.assertEqual(float("NaN").__format__("*^8g"), "**nan***")
        self.assertEqual(float.__format__(-10.02, "=8g"), "-  10.02")
        self.assertEqual(float.__format__(-1.234, "!<10G"), "-1.234!!!!")

    def test_alternate_returns_str(self):
        self.assertEqual(float.__format__(0.0, "#"), "0.0")
        self.assertEqual(float.__format__(-123.00, "#.0e"), "-1.e+02")
        self.assertEqual(float.__format__(-0.123, "#.0E"), "-1.E-01")
        self.assertEqual(float.__format__(12.021, "#.2f"), "12.02")
        self.assertEqual(float.__format__(12.021, "#.0F"), "12.")
        self.assertEqual(float.__format__(12.0, "#g"), "12.0000")
        self.assertEqual(float.__format__(2.0 ** 63, "#.1G"), "9.E+18")

    def test_with_sign_returns_str(self):
        self.assertEqual(float.__format__(1.0, " "), " 1.0")
        self.assertEqual(float.__format__(1.0, "+"), "+1.0")
        self.assertEqual(float.__format__(1.0, "-"), "1.0")
        self.assertEqual(float.__format__(-4.0, " "), "-4.0")
        self.assertEqual(float.__format__(-4.0, "+"), "-4.0")
        self.assertEqual(float.__format__(-4.0, "-"), "-4.0")

    def test_sign_aware_zero_padding_allows_zero_width(self):
        self.assertEqual(float.__format__(123.0, "00"), "123.0")
        self.assertEqual(float.__format__(123.34, "00f"), "123.340000")
        self.assertEqual(float.__format__(123.34, "00e"), "1.233400e+02")
        self.assertEqual(float.__format__(123.34, "00g"), "123.34")
        self.assertEqual(float.__format__(123.34, "00.10f"), "123.3400000000")
        self.assertEqual(float.__format__(123.34, "00.10e"), "1.2334000000e+02")
        self.assertEqual(float.__format__(123.34, "00.10g"), "123.34")
        self.assertEqual(float.__format__(123.34, "01f"), "123.340000")

        self.assertEqual(float.__format__(-123.0, "00"), "-123.0")
        self.assertEqual(float.__format__(-123.34, "00f"), "-123.340000")
        self.assertEqual(float.__format__(-123.34, "00e"), "-1.233400e+02")
        self.assertEqual(float.__format__(-123.34, "00g"), "-123.34")
        self.assertEqual(float.__format__(-123.34, "00.10f"), "-123.3400000000")
        self.assertEqual(float.__format__(-123.34, "00.10f"), "-123.3400000000")
        self.assertEqual(float.__format__(-123.34, "00.10e"), "-1.2334000000e+02")
        self.assertEqual(float.__format__(-123.34, "00.10g"), "-123.34")

    def test_unknown_format_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            float.__format__(42.0, "c")
        self.assertEqual(
            str(context.exception), "Unknown format code 'c' for object of type 'float'"
        )

    def test_with_non_float_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            float.__format__("not a float", "")
        self.assertIn("'__format__' requires a 'float' object", str(context.exception))
        self.assertIn("'str'", str(context.exception))


class FrozensetTests(unittest.TestCase):
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


class IntTests(unittest.TestCase):
    def test_dunder_hash_with_small_number_returns_self(self):
        self.assertEqual(int.__hash__(0), 0)
        self.assertEqual(int.__hash__(1), 1)
        self.assertEqual(int.__hash__(-5), -5)
        self.assertEqual(int.__hash__(10000), 10000)
        self.assertEqual(int.__hash__(-10000), -10000)

    def test_dunder_hash_minus_one_returns_minus_two(self):
        self.assertEqual(int.__hash__(-1), -2)

        import sys

        factor = sys.hash_info.modulus + 1
        self.assertEqual(int.__hash__(-1 * factor), -2)
        self.assertEqual(int.__hash__(-1 * factor * factor * factor), -2)

    def test_dunder_hash_applies_modulus(self):
        import sys

        modulus = sys.hash_info.modulus

        self.assertEqual(int.__hash__(modulus - 1), modulus - 1)
        self.assertEqual(int.__hash__(modulus), 0)
        self.assertEqual(int.__hash__(modulus + 1), 1)
        self.assertEqual(int.__hash__(modulus + modulus + 13), 13)
        self.assertEqual(int.__hash__(-modulus + 1), -modulus + 1)
        self.assertEqual(int.__hash__(-modulus), 0)
        self.assertEqual(int.__hash__(-modulus - 2), -2)
        self.assertEqual(int.__hash__(-modulus - modulus - 13), -13)

    def test_dunder_hash_with_largeint_returns_int(self):
        self.assertEqual(int.__hash__(1 << 234), 1 << 51)
        self.assertEqual(int.__hash__(-1 << 234), -1 << 51)

        value = 0xB9F041FF6B5D18158ABA6BDAE4B582C01F55A792
        self.assertEqual(int.__hash__(value), 2278332794247153219)
        self.assertEqual(int.__hash__(-value), -2278332794247153219)

    def test_dunder_new_with_bool_class_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, r"int\.__new__\(bool\) is not safe.*bool\.__new__\(\)"
        ):
            int.__new__(bool, 0)

    def test_dunder_new_with_dunder_int_subclass_warns(self):
        class Num(int):
            pass

        class Foo:
            def __int__(self):
                return Num(42)

        foo = Foo()
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(int(foo), 42)

    def test_dunder_new_uses_type_dunder_int(self):
        class Foo:
            def __int__(self):
                return 0

        foo = Foo()
        foo.__int__ = "not callable"
        self.assertEqual(int(foo), 0)

    def test_dunder_new_uses_type_dunder_trunc(self):
        class Foo:
            def __trunc__(self):
                return 0

        foo = Foo()
        foo.__trunc__ = "not callable"
        self.assertEqual(int(foo), 0)

    def test_dunder_new_with_raising_trunc_propagates_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("failed")

        class Foo:
            __trunc__ = Desc()

        foo = Foo()
        with self.assertRaises(AttributeError) as context:
            int(foo)
        self.assertEqual(str(context.exception), "failed")

    def test_dunder_new_with_base_without_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            int(base=8)

    def test_dunder_new_with_bool_returns_int(self):
        self.assertIs(int(False), 0)
        self.assertIs(int(True), 1)

    def test_dunder_new_with_bytearray_returns_int(self):
        self.assertEqual(int(bytearray(b"23")), 23)
        self.assertEqual(int(bytearray(b"-23"), 8), -0o23)
        self.assertEqual(int(bytearray(b"abc"), 16), 0xABC)
        self.assertEqual(int(bytearray(b"0xabc"), 0), 0xABC)

    def test_dunder_new_with_bytes_returns_int(self):
        self.assertEqual(int(b"-23"), -23)
        self.assertEqual(int(b"23", 8), 0o23)
        self.assertEqual(int(b"abc", 16), 0xABC)
        self.assertEqual(int(b"0xabc", 0), 0xABC)

    def test_dunder_new_strips_whitespace(self):
        self.assertEqual(int(" \t\n123\n\t "), 123)
        self.assertEqual(int(" \t\n123\n\t ", 10), 123)
        self.assertEqual(int(b" \t\n123\n\t "), 123)
        self.assertEqual(int(b" \t\n123\n\t ", 10), 123)
        self.assertEqual(int(bytearray(b" \t\n123\n\t "), 10), 123)

    def test_dunder_new_with_non_space_after_whitespace_raises_value_error(self):
        self.assertRaises(ValueError, int, " \t\n123\n\t 1")
        self.assertRaises(ValueError, int, " \t\n123\n\t 1", 10)
        self.assertRaises(ValueError, int, b" \t\n123\n\t 1")
        self.assertRaises(ValueError, int, b" \t\n123\n\t 1", 10)
        self.assertRaises(ValueError, int, bytearray(b" \t\n123\n\t 1"))
        self.assertRaises(ValueError, int, bytearray(b" \t\n123\n\t 1"), 10)

    def test_dunder_new_with_empty_bytearray_raises_value_error(self):
        with self.assertRaises(ValueError):
            int(bytearray())

    def test_dunder_new_with_empty_bytes_raises_value_error(self):
        with self.assertRaises(ValueError):
            int(b"")

    def test_dunder_new_with_empty_str_raises_value_error(self):
        with self.assertRaises(ValueError):
            int("")

    def test_dunder_new_with_int_returns_int(self):
        self.assertEqual(int(23), 23)

    def test_dunder_new_with_int_and_base_raises_type_error(self):
        with self.assertRaises(TypeError):
            int(4, 5)

    def test_dunder_new_with_invalid_base_raises_value_error(self):
        with self.assertRaises(ValueError):
            int("0", 1)

    def test_dunder_new_with_invalid_chars_raises_value_error(self):
        with self.assertRaises(ValueError):
            int("&2*")

    def test_dunder_new_with_invalid_digits_raises_value_error(self):
        with self.assertRaises(ValueError):
            int(b"789", 6)

    def test_dunder_new_with_str_returns_int(self):
        self.assertEqual(int("23"), 23)
        self.assertEqual(int("-23", 8), -0o23)
        self.assertEqual(int("-abc", 16), -0xABC)
        self.assertEqual(int("0xabc", 0), 0xABC)

    def test_dunder_new_with_zero_args_returns_zero(self):
        self.assertIs(int(), 0)

    def test_dunder_pow_with_zero_returns_one(self):
        self.assertEqual(int.__pow__(4, 0), 1)

    def test_dunder_pow_with_one_returns_self(self):
        self.assertEqual(int.__pow__(4, 1), 4)

    def test_dunder_pow_with_two_squares_number(self):
        self.assertEqual(int.__pow__(4, 2), 16)

    def test_dunder_pow_with_mod_equals_one_returns_zero(self):
        self.assertEqual(int.__pow__(4, 2, 1), 0)

    def test_dunder_pow_with_negative_power_and_mod_raises_value_error(self):
        with self.assertRaises(ValueError):
            int.__pow__(4, -2, 1)

    def test_dunder_pow_with_mod(self):
        self.assertEqual(int.__pow__(4, 8, 10), 6)

    def test_dunder_pow_with_large_mod(self):
        self.assertEqual(int.__pow__(10, 10 ** 1000, 3), 1)

    def test_dunder_pow_with_negative_base_calls_float_dunder_pow(self):
        self.assertLess((int.__pow__(2, -1) - 0.5).__abs__(), 0.00001)

    def test_dunder_pow_with_non_int_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            int.__pow__(None, 1, 1)

    def test_dunder_pow_with_non_int_power_returns_not_implemented(self):
        self.assertEqual(int.__pow__(1, None), NotImplemented)

    def test_from_bytes_returns_int(self):
        self.assertEqual(int.from_bytes(b"\xca\xfe", "little"), 0xFECA)

    def test_from_bytes_with_kwargs_returns_int(self):
        self.assertEqual(
            int.from_bytes(bytes=b"\xca\xfe", byteorder="big", signed=True), -13570
        )

    def test_from_bytes_with_bytes_convertible_returns_int(self):
        class C:
            def __bytes__(self):
                return b"*"

        i = C()
        self.assertEqual(int.from_bytes(i, "big"), 42)

    def test_from_bytes_with_invalid_byteorder_raises_before_invalid_type(self):
        with self.assertRaisesRegex(
            ValueError, "byteorder must be either 'little' or 'big'"
        ):
            int.from_bytes("not a bytes object", byteorder="medium")

    def test_from_bytes_with_invalid_bytes_raises_type_error(self):
        with self.assertRaisesRegex(TypeError, "cannot convert 'str' object to bytes"):
            int.from_bytes("not a bytes object", "big")

    def test_from_bytes_with_invalid_byteorder_raises_value_error(self):
        with self.assertRaisesRegex(
            ValueError, "byteorder must be either 'little' or 'big'"
        ):
            int.from_bytes(b"*", byteorder="medium")

    def test_from_bytes_with_invalid_byteorder_raises_type_error(self):
        with self.assertRaises(TypeError):
            int.from_bytes(b"*", byteorder=42)

    def test_from_bytes_uses_type_dunder_bytes(self):
        class Foo:
            def __bytes__(self):
                return b"*"

        foo = Foo()
        foo.__bytes__ = lambda: b"123"
        self.assertEqual(int.from_bytes(foo, "big"), 42)


class IntDunderFormatTests(unittest.TestCase):
    def test_empty_format_returns_str(self):
        self.assertEqual(int.__format__(0, ""), "0")
        self.assertEqual(int.__format__(1, ""), "1")
        self.assertEqual(int.__format__(-1, ""), "-1")
        self.assertEqual(
            int.__format__(0xFF1FD0035E55A752381015D7, ""),
            "78957136519217238320723531223",
        )

    def test_empty_format_calls_dunder_str(self):
        self.assertEqual(int.__format__(True, ""), "True")
        self.assertEqual(int.__format__(False, ""), "False")

        class C(int):
            def __str__(self):
                return "foobar"

        self.assertEqual(int.__format__(C(), ""), "foobar")

    def test_c_format_returns_str(self):
        self.assertEqual(int.__format__(0, "c"), "\0")
        self.assertEqual(int.__format__(False, "c"), "\0")
        self.assertEqual(int.__format__(True, "c"), "\01")
        self.assertEqual(int.__format__(80, "c"), "P")
        self.assertEqual(int.__format__(128013, "c"), "\U0001f40d")
        import sys

        self.assertEqual(int.__format__(sys.maxunicode, "c"), chr(sys.maxunicode))

    def test_d_format_returns_str(self):
        self.assertEqual(int.__format__(0, "d"), "0")
        self.assertEqual(int.__format__(False, "d"), "0")
        self.assertEqual(int.__format__(-1, "d"), "-1")
        self.assertEqual(int.__format__(1, "d"), "1")
        self.assertEqual(int.__format__(True, "d"), "1")
        self.assertEqual(int.__format__(42, "d"), "42")
        self.assertEqual(
            int.__format__(0x2B3EFBA733D579B55A9074934, "d"),
            "214143955543398893443684452660",
        )
        self.assertEqual(
            int.__format__(-0xF52A2EC166BD52D048CD1EA6C6E478B3, "d"),
            "-325879883749036333909592275151101393075",
        )

    def test_x_format_returns_str(self):
        self.assertEqual(int.__format__(0, "x"), "0")
        self.assertEqual(int.__format__(False, "x"), "0")
        self.assertEqual(int.__format__(-1, "x"), "-1")
        self.assertEqual(int.__format__(1, "x"), "1")
        self.assertEqual(int.__format__(True, "x"), "1")
        self.assertEqual(int.__format__(42, "x"), "2a")
        self.assertEqual(
            int.__format__(214143955543398893443684452660, "x"),
            "2b3efba733d579b55a9074934",
        )
        self.assertEqual(
            int.__format__(-32587988374903633390959227539375, "x"),
            "-19b51782f9224e54288bc2f8faf",
        )

    def test_big_x_format_returns_str(self):
        self.assertEqual(int.__format__(0, "X"), "0")
        self.assertEqual(int.__format__(-1, "X"), "-1")
        self.assertEqual(int.__format__(1, "X"), "1")
        self.assertEqual(int.__format__(42, "X"), "2A")
        self.assertEqual(
            int.__format__(214143955543398893443684452660, "X"),
            "2B3EFBA733D579B55A9074934",
        )

    def test_o_format_returns_str(self):
        self.assertEqual(int.__format__(0, "o"), "0")
        self.assertEqual(int.__format__(1, "o"), "1")
        self.assertEqual(int.__format__(-1, "o"), "-1")
        self.assertEqual(int.__format__(42, "o"), "52")
        self.assertEqual(
            int.__format__(0xFF1FD0035E55A752381015D7, "o"),
            "77617720006571255165107004012727",
        )
        self.assertEqual(
            int.__format__(-0x6D68DA9740D4105E240D77C587602, "o"),
            "-155321552272015202027422015357426073002",
        )

    def test_b_format_returns_str(self):
        self.assertEqual(int.__format__(0, "b"), "0")
        self.assertEqual(int.__format__(1, "b"), "1")
        self.assertEqual(int.__format__(-1, "b"), "-1")
        self.assertEqual(int.__format__(42, "b"), "101010")
        self.assertEqual(
            int.__format__(0x2D393E39DFD869D3DD18D24D6FE8C, "b"),
            "101101001110010011111000111001110111111101100001101001110100"
            "111101110100011000110100100100110101101111111010001100",
        )
        self.assertEqual(
            int.__format__(-0x52471A39A4C1BCB4D711249, "b"),
            "-10100100100011100011010001110011010010011000001101111001011"
            "01001101011100010001001001001001",
        )

    def test_float_format_returns_str(self):
        self.assertEqual(int.__format__(2 ** 64, "e"), "1.844674e+19")
        self.assertEqual(int.__format__(-12345, "E"), "-1.234500E+04")
        self.assertEqual(int.__format__(0, "f"), "0.000000")
        self.assertEqual(int.__format__(321, "F"), "321.000000")
        self.assertEqual(int.__format__(-1024, "g"), "-1024")
        self.assertEqual(int.__format__(10 ** 100, "G"), "1E+100")
        self.assertEqual(int.__format__(1, "%"), "100.000000%")

    def test_alternate_returns_str(self):
        self.assertEqual(int.__format__(0, "#"), "0")
        self.assertEqual(int.__format__(-123, "#"), "-123")
        self.assertEqual(int.__format__(42, "#d"), "42")
        self.assertEqual(int.__format__(-99, "#d"), "-99")
        self.assertEqual(int.__format__(77, "#b"), "0b1001101")
        self.assertEqual(int.__format__(-11, "#b"), "-0b1011")
        self.assertEqual(int.__format__(22, "#o"), "0o26")
        self.assertEqual(int.__format__(-33, "#o"), "-0o41")
        self.assertEqual(int.__format__(123, "#x"), "0x7b")
        self.assertEqual(int.__format__(-44, "#x"), "-0x2c")
        self.assertEqual(int.__format__(123, "#X"), "0X7B")
        self.assertEqual(int.__format__(-44, "#X"), "-0X2C")

    def test_with_sign_returns_str(self):
        self.assertEqual(int.__format__(7, " "), " 7")
        self.assertEqual(int.__format__(7, "+"), "+7")
        self.assertEqual(int.__format__(7, "-"), "7")
        self.assertEqual(int.__format__(-4, " "), "-4")
        self.assertEqual(int.__format__(-4, "+"), "-4")
        self.assertEqual(int.__format__(-4, "-"), "-4")

    def test_with_alignment_returns_str(self):
        self.assertEqual(int.__format__(-42, "0"), "-42")
        self.assertEqual(int.__format__(-42, "1"), "-42")
        self.assertEqual(int.__format__(-42, "5"), "  -42")
        self.assertEqual(int.__format__(-42, "05"), "-0042")
        self.assertEqual(int.__format__(-42, "0005"), "-0042")
        self.assertEqual(int.__format__(-42, "<5"), "-42  ")
        self.assertEqual(int.__format__(-42, ">5"), "  -42")
        self.assertEqual(int.__format__(-42, "=5"), "-  42")
        self.assertEqual(int.__format__(-42, "^6"), " -42  ")
        self.assertEqual(int.__format__(-42, "^^7"), "^^-42^^")
        self.assertEqual(int.__format__(-123, "#=#20x"), "-0x###############7b")
        self.assertEqual(
            int.__format__(-42, "\U0001f40d^6o"), "\U0001f40d-52\U0001f40d\U0001f40d"
        )

    def test_c_format_with_alignment_returns_str(self):
        self.assertEqual(int.__format__(65, ".^12c"), ".....A......")
        self.assertEqual(int.__format__(128013, ".^12c"), ".....\U0001f40d......")
        self.assertEqual(
            int.__format__(90, "\U0001f40d<4c"), "Z\U0001f40d\U0001f40d\U0001f40d"
        )
        self.assertEqual(
            int.__format__(90, "\U0001f40d>4c"), "\U0001f40d\U0001f40d\U0001f40dZ"
        )
        self.assertEqual(
            int.__format__(90, "\U0001f40d=4c"), "\U0001f40d\U0001f40d\U0001f40dZ"
        )

    def test_all_codes_with_alignment_returns_str(self):
        self.assertEqual(int.__format__(-123, "^12"), "    -123    ")
        self.assertEqual(int.__format__(-123, "^12d"), "    -123    ")
        self.assertEqual(int.__format__(-123, ".^#12b"), ".-0b1111011.")
        self.assertEqual(int.__format__(-123, ".^#12o"), "...-0o173...")
        self.assertEqual(int.__format__(-123, ".^#12x"), "...-0x7b....")
        self.assertEqual(int.__format__(-123, ".^#12X"), "...-0X7B....")

    def test_with_alignment_and_sign_returns_str(self):
        self.assertEqual(int.__format__(9, " 5"), "    9")
        self.assertEqual(int.__format__(9, "+5"), "   +9")
        self.assertEqual(int.__format__(9, "\t< 5"), " 9\t\t\t")
        self.assertEqual(int.__format__(9, "<+5"), "+9   ")
        self.assertEqual(int.__format__(9, ";= 5"), " ;;;9")
        self.assertEqual(int.__format__(9, "=+5"), "+   9")

    def test_works_with_subclass(self):
        class C(int):
            pass

        self.assertEqual(int.__format__(C(42), ""), "42")
        self.assertEqual(int.__format__(C(8), "*^#8b"), "*0b1000*")

        class D(str):
            pass

        self.assertEqual(int.__format__(42, D("")), "42")
        self.assertEqual(int.__format__(C(6), D("*^#8b")), "*0b110**")

    def test_precision_raises_error(self):
        with self.assertRaises(ValueError) as context:
            int.__format__(0, ".10")
        self.assertEqual(
            str(context.exception), "Precision not allowed in integer format specifier"
        )
        with self.assertRaises(ValueError) as context:
            int.__format__(0, ".10c")
        self.assertEqual(
            str(context.exception), "Precision not allowed in integer format specifier"
        )
        with self.assertRaises(ValueError) as context:
            int.__format__(0, ".10d")
        self.assertEqual(
            str(context.exception), "Precision not allowed in integer format specifier"
        )
        with self.assertRaises(ValueError) as context:
            int.__format__(0, ".10o")
        self.assertEqual(
            str(context.exception), "Precision not allowed in integer format specifier"
        )
        with self.assertRaises(ValueError) as context:
            int.__format__(0, ".1b")
        self.assertEqual(
            str(context.exception), "Precision not allowed in integer format specifier"
        )
        with self.assertRaises(ValueError) as context:
            int.__format__(0, ".10x")
        self.assertEqual(
            str(context.exception), "Precision not allowed in integer format specifier"
        )
        with self.assertRaises(ValueError) as context:
            int.__format__(0, ".10X")
        self.assertEqual(
            str(context.exception), "Precision not allowed in integer format specifier"
        )

    def test_c_format_raises_overflow_error(self):
        with self.assertRaises(OverflowError) as context:
            int.__format__(-1, "c")
        self.assertEqual(str(context.exception), "%c arg not in range(0x110000)")

        import sys

        with self.assertRaises(OverflowError) as context:
            int.__format__(sys.maxunicode + 1, "c")
        self.assertEqual(str(context.exception), "%c arg not in range(0x110000)")

    def test_c_format_with_sign_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            int.__format__(64, "+c")
        self.assertEqual(
            str(context.exception), "Sign not allowed with integer format specifier 'c'"
        )

    def test_c_format_alternate_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            int.__format__(64, "#c")
        self.assertEqual(
            str(context.exception),
            "Alternate form (#) not allowed with integer format specifier 'c'",
        )

    def test_unknown_format_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            int.__format__(42, "z")
        self.assertEqual(
            str(context.exception), "Unknown format code 'z' for object of type 'int'"
        )

    def test_with_non_int_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            int.__format__("not an int", "")
        self.assertIn("'__format__' requires a 'int' object", str(context.exception))
        self.assertIn("'str'", str(context.exception))


class IsInstanceTests(unittest.TestCase):
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
            "isinstance() arg 2 must be a type or tuple of types",
        )

    def test_isinstance_with_non_type_in_tuple_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            isinstance(5, ("bad - not a type", int))
        self.assertEqual(
            str(context.exception),
            "isinstance() arg 2 must be a type or tuple of types",
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
            "isinstance() arg 2 must be a type or tuple of types",
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
    def test_dunder_add_with_non_list_raises_type_error(self):
        self.assertRaises(TypeError, list.__add__, (), [])
        self.assertRaises(TypeError, list.__add__, [], ())

    def test_dunder_add_with_empty_lists_returns_new_empty_list(self):
        orig = []
        result = list.__add__(orig, orig)
        self.assertEqual(result, [])
        self.assertIsNot(result, orig)

    def test_dunder_add_with_list_returns_new_list(self):
        orig = [1, 2, 3]
        self.assertEqual(list.__add__(orig, []), [1, 2, 3])
        self.assertEqual(list.__add__(orig, [4, 5]), [1, 2, 3, 4, 5])

        # orig is unchanged
        self.assertEqual(orig, [1, 2, 3])

    def test_dunder_eq_with_non_list_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            list.__eq__(False, [])
        self.assertIn(
            "'__eq__' requires a 'list' object but received a 'bool'",
            str(context.exception),
        )

    def test_dunder_iadd_with_list_subclass_uses_iter(self):
        class C(list):
            def __iter__(self):
                return iter([1, 2, 3])

        orig = [1, 2, 3]
        other = C([4, 5])
        self.assertIs(list.__iadd__(orig, other), orig)
        self.assertEqual(orig, [1, 2, 3, 1, 2, 3])

    def test_dunder_iadd_with_iterable_appends_to_list(self):
        orig = []

        self.assertIs(list.__iadd__(orig, [1, 2, 3]), orig)
        self.assertIs(list.__iadd__(orig, set()), orig)
        self.assertIs(list.__iadd__(orig, "abc"), orig)
        self.assertIs(list.__iadd__(orig, (4, 5)), orig)

        # orig now contains all other elements
        self.assertEqual(orig, [1, 2, 3, "a", "b", "c", 4, 5])

    def test_dunder_imul_with_non_list_raises_type_error(self):
        with self.assertRaisesRegex(TypeError, "'__imul__' requires a 'list' object"):
            list.__imul__(False, 3)

    def test_dunder_imul_with_non_int_raises_type_error(self):
        result = []
        with self.assertRaises(TypeError):
            list.__imul__(result, "x")

    def test_dunder_imul_with_empty_returns_self(self):
        orig = []
        result = orig.__imul__(3)
        self.assertIs(result, orig)

    def test_dunder_imul_with_empty_self_and_zero_repeat_does_nothing(self):
        orig = []
        result = orig.__imul__(0)
        self.assertIs(result, orig)
        self.assertEqual(len(result), 0)

    def test_dunder_imul_with_zero_repeat_clears_list(self):
        orig = [1, 2, 3]
        result = orig.__imul__(0)
        self.assertIs(result, orig)
        self.assertEqual(len(result), 0)

    def test_dunder_imul_with_negative_repeat_clears_list(self):
        orig = [1, 2, 3]
        result = orig.__imul__(-3)
        self.assertIs(result, orig)
        self.assertEqual(len(result), 0)

    def test_dunder_imul_with_repeat_equals_one_returns_self(self):
        orig = [1, 2, 3]
        result = orig.__imul__(1)
        self.assertIs(result, orig)

    def test_dunder_imul_with_too_big_repeat_raises_overflow_error(self):
        orig = [1, 2, 3]
        self.assertRaisesRegex(
            OverflowError,
            "cannot fit 'int' into an index-sized integer",
            orig.__imul__,
            100 ** 100,
        )

    def test_dunder_imul_with_too_big_repeat_raises_memory_error(self):
        orig = [1, 2, 3] * 10000
        self.assertRaises(MemoryError, orig.__imul__, 0x7FFFFFFFFFFFFFF)

    def test_dunder_imul_with_repeat_repeats_contents(self):
        orig = [1, 2, 3]
        result = orig.__imul__(2)
        self.assertIs(result, orig)
        self.assertEqual(result, [1, 2, 3, 1, 2, 3])

    def test_dunder_ne_returns_not_eq(self):
        orig = [1, 2, 3]
        self.assertTrue(orig.__ne__([1, 2]))
        self.assertFalse(orig.__ne__(orig))

    def test_dunder_ne_returns_not_implemented_if_wrong_types(self):
        orig = [1, 2, 3]
        self.assertIs(orig.__ne__(1), NotImplemented)
        self.assertIs(orig.__ne__({}), NotImplemented)

    def test_dunder_reversed_returns_reversed_iterator(self):
        orig = [1, 2, 3]
        rev_iter = orig.__reversed__()
        self.assertEqual(next(rev_iter), 3)
        self.assertEqual(next(rev_iter), 2)
        self.assertEqual(next(rev_iter), 1)
        with self.assertRaises(StopIteration):
            next(rev_iter)

    def test_dunder_setitem_with_int_sets_value_at_index(self):
        orig = [1, 2, 3]
        orig[1] = 4
        self.assertEqual(orig, [1, 4, 3])

    def test_dunder_setitem_with_negative_int_sets_value_at_adjusted_index(self):
        orig = [1, 2, 3]
        orig[-1] = 4
        self.assertEqual(orig, [1, 2, 4])

    def test_dunder_setitem_with_str_raises_type_error(self):
        orig = []
        with self.assertRaises(TypeError) as context:
            orig["not an index"] = "element"
        self.assertEqual(
            str(context.exception), "list indices must be integers or slices, not str"
        )

    def test_dunder_setitem_with_large_int_raises_index_error(self):
        orig = [1, 2, 3]
        with self.assertRaises(IndexError) as context:
            orig[2 ** 63] = 4
        self.assertEqual(
            str(context.exception), "cannot fit 'int' into an index-sized integer"
        )

    def test_dunder_setitem_with_positive_out_of_bounds_raises_index_error(self):
        orig = [1, 2, 3]
        with self.assertRaises(IndexError) as context:
            orig[3] = 4
        self.assertEqual(str(context.exception), "list assignment index out of range")

    def test_dunder_setitem_with_negative_out_of_bounds_raises_index_error(self):
        orig = [1, 2, 3]
        with self.assertRaises(IndexError) as context:
            orig[-4] = 4
        self.assertEqual(str(context.exception), "list assignment index out of range")

    def test_dunder_setitem_slice_with_empty_slice_clears_list(self):
        orig = [1, 2, 3]
        orig[:] = ()
        self.assertEqual(orig, [])

    def test_dunder_setitem_slice_with_same_size(self):
        orig = [1, 2, 3, 4, 5]
        orig[1:4] = ["B", "C", "D"]
        self.assertEqual(orig, [1, "B", "C", "D", 5])

    def test_dunder_setitem_slice_with_short_stop_grows(self):
        orig = [1, 2, 3, 4, 5]
        orig[:1] = "abc"
        self.assertEqual(orig, ["a", "b", "c", 2, 3, 4, 5])

    def test_dunder_setitem_slice_with_larger_slice_grows(self):
        orig = [1, 2, 3, 4, 5]
        orig[1:4] = ["B", "C", "D", "MORE", "ELEMENTS"]
        self.assertEqual(orig, [1, "B", "C", "D", "MORE", "ELEMENTS", 5])

    def test_dunder_setitem_slice_with_smaller_slice_shrinks(self):
        orig = [1, 2, 3, 4, 5]
        orig[1:4] = ["FEWER", "ELEMENTS"]
        self.assertEqual(orig, [1, "FEWER", "ELEMENTS", 5])

    def test_dunder_setitem_slice_with_tuple_sets_slice(self):
        orig = [1, 2, 3, 4, 5]
        orig[1:4] = ("a", 6, None)
        self.assertEqual(orig, [1, "a", 6, None, 5])

    def test_dunder_setitem_slice_with_self_copies(self):
        orig = [1, 2, 3, 4, 5]
        orig[1:4] = orig
        self.assertEqual(orig, [1, 1, 2, 3, 4, 5, 5])

    def test_dunder_setitem_slice_with_reversed_bounds_inserts_at_start(self):
        orig = [1, 2, 3, 4, 5]
        orig[3:2] = "ab"
        self.assertEqual(orig, [1, 2, 3, "a", "b", 4, 5])

    def test_dunder_setitem_slice_with_step_assigns_slice(self):
        orig = [1, 2, 3, 4, 5, 6]
        orig[::2] = "abc"
        self.assertEqual(orig, ["a", 2, "b", 4, "c", 6])

    def test_dunder_setitem_slice_with_negative_step_assigns_backwards(self):
        orig = [1, 2, 3, 4, 5, 6]
        orig[::-2] = "abc"
        self.assertEqual(orig, [1, "c", 3, "b", 5, "a"])

    def test_dunder_setitem_slice_extended_short_raises_value_error(self):
        orig = [1, 2, 3, 4, 5]
        with self.assertRaises(ValueError) as context:
            orig[::2] = ()
        self.assertEqual(
            str(context.exception),
            "attempt to assign sequence of size 0 to extended slice of size 3",
        )

    def test_dunder_setitem_slice_extended_long_raises_value_error(self):
        orig = [1, 2, 3, 4, 5]
        with self.assertRaises(ValueError) as context:
            orig[::2] = (1, 2, 3, 4)
        self.assertEqual(
            str(context.exception),
            "attempt to assign sequence of size 4 to extended slice of size 3",
        )

    def test_dunder_setitem_slice_with_non_iterable_raises_type_error(self):
        orig = []
        with self.assertRaises(TypeError) as context:
            orig[:] = 1
        self.assertEqual(str(context.exception), "can only assign an iterable")

    def test_clear_with_empty_list_does_nothing(self):
        ls = []
        self.assertIsNone(ls.clear())
        self.assertEqual(len(ls), 0)

    def test_copy_with_non_list_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            list.copy(None)
        self.assertIn(
            "'copy' requires a 'list' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_count_with_non_list_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            list.count(None, 1)
        self.assertIn(
            "'count' requires a 'list' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_count_with_item_returns_int_count(self):
        ls = [1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1]
        self.assertEqual(ls.count(1), 8)
        self.assertEqual(ls.count(2), 4)
        self.assertEqual(ls.count(3), 2)
        self.assertEqual(ls.count(4), 1)
        self.assertEqual(ls.count(5), 0)

    def test_count_calls_dunder_eq(self):
        class AlwaysEqual:
            def __eq__(self, other):
                return True

        class NeverEqual:
            def __eq__(self, other):
                return False

        a = AlwaysEqual()
        n = NeverEqual()
        a_list = [a, a, a, a, a]
        n_list = [n, n, n]
        self.assertEqual(a_list.count(a), 5)
        self.assertEqual(a_list.count(n), 5)
        self.assertEqual(n_list.count(a), 0)
        self.assertEqual(n_list.count(n), 3)
        self.assertEqual(n_list.count(NeverEqual()), 0)

    def test_count_does_not_use_dunder_getitem_or_dunder_iter(self):
        class Foo(list):
            def __getitem__(self, idx):
                raise NotImplementedError("__getitem__")

            def __iter__(self):
                raise NotImplementedError("__iter__")

        a = Foo([1, 2, 1, 2, 1])
        self.assertEqual(a.count(0), 0)
        self.assertEqual(a.count(1), 3)
        self.assertEqual(a.count(2), 2)

    def test_dunder_hash_is_none(self):
        self.assertIs(list.__hash__, None)

    def test_extend_list_returns_none(self):
        original = [1, 2, 3]
        copy = []
        self.assertIsNone(copy.extend(original))
        self.assertFalse(copy is original)
        self.assertEqual(copy, original)

    def test_extend_with_iterator_that_raises_partway_through_has_sideeffect(self):
        class C:
            def __init__(self):
                self.n = 0

            def __iter__(self):
                return self

            def __next__(self):
                if self.n > 4:
                    raise UserWarning("foo")
                self.n += 1
                return self.n

        result = [0]
        with self.assertRaises(UserWarning):
            result.extend(C())
        self.assertEqual(result, [0, 1, 2, 3, 4, 5])

    def test_delitem_with_int_subclass_does_not_call_dunder_index(self):
        class C(int):
            def __index__(self):
                raise ValueError("foo")

        ls = list(range(5))
        del ls[C(0)]
        self.assertEqual(ls, [1, 2, 3, 4])

    def test_delitem_with_dunder_index_calls_dunder_index(self):
        class C:
            def __index__(self):
                return 2

        ls = list(range(5))
        del ls[C()]
        self.assertEqual(ls, [0, 1, 3, 4])

    def test_delslice_negative_indexes_removes_first_element(self):
        a = [0, 1]
        del a[-2:-1]
        self.assertEqual(a, [1])

    def test_delslice_negative_start_no_stop_removes_trailing_elements(self):
        a = [0, 1]
        del a[-1:]
        self.assertEqual(a, [0])

    def test_delslice_with_negative_step_deletes_every_other_element(self):
        a = [0, 1, 2, 3, 4]
        del a[::-2]
        self.assertEqual(a, [1, 3])

    def test_delslice_with_start_and_negative_step_deletes_every_other_element(self):
        a = [0, 1, 2, 3, 4]
        del a[1::-2]
        self.assertEqual(a, [0, 2, 3, 4])

    def test_delslice_with_large_step_deletes_last_element(self):
        a = [0, 1, 2, 3, 4]
        del a[4 :: 1 << 333]
        self.assertEqual(a, [0, 1, 2, 3])

    def test_delslice_with_large_stop_deletes_to_end(self):
        a = [0, 1, 2, 3, 4]
        del a[2 : 1 << 333]
        self.assertEqual(a, [0, 1])

    def test_delslice_with_negative_large_stop_deletes_nothing(self):
        a = [0, 1, 2, 3, 4]
        del a[2 : -(1 << 333)]
        self.assertEqual(a, [0, 1, 2, 3, 4])

    def test_delslice_with_true_stop_deletes_to_one(self):
        a = [0, 1, 2, 3, 4]
        del a[:True]
        self.assertEqual(a, [1, 2, 3, 4])

    def test_delslice_with_false_stop_deletes_to_zero(self):
        a = [0, 1, 2, 3, 4]
        del a[:False]
        self.assertEqual(a, [0, 1, 2, 3, 4])

    def test_getitem_with_int_subclass_does_not_call_dunder_index(self):
        class C(int):
            def __index__(self):
                raise ValueError("foo")

        ls = list(range(5))
        self.assertEqual(ls[C(3)], 3)

    def test_getitem_with_raising_descriptor_propagates_exception(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("foo")

        class C:
            __index__ = Desc()

        ls = list(range(5))
        with self.assertRaises(AttributeError) as context:
            ls[C()]
        self.assertEqual(str(context.exception), "foo")

    def test_getitem_with_string_raises_type_error(self):
        ls = list(range(5))
        with self.assertRaises(TypeError) as context:
            ls["3"]
        self.assertEqual(
            str(context.exception), "list indices must be integers or slices, not str"
        )

    def test_getitem_with_dunder_index_calls_dunder_index(self):
        class C:
            def __index__(self):
                return 2

        ls = list(range(5))
        self.assertEqual(ls[C()], 2)

    def test_getitem_returns_item(self):
        original = [1, 2, 3, 4, 5, 6]
        self.assertEqual(original[0], 1)
        self.assertEqual(original[3], 4)
        self.assertEqual(original[5], 6)
        original[0] = 6
        original[5] = 1
        self.assertEqual(original[0], 6)
        self.assertEqual(original[3], 4)
        self.assertEqual(original[5], 1)

    def test_getslice_with_valid_indices_returns_sublist(self):
        ls = list(range(5))
        self.assertEqual(ls[2:-1:1], [2, 3])

    def test_getslice_with_negative_start_returns_trailing(self):
        ls = list(range(5))
        self.assertEqual(ls[-2:], [3, 4])

    def test_getslice_with_positive_stop_returns_leading(self):
        ls = list(range(5))
        self.assertEqual(ls[:2], [0, 1])

    def test_getslice_with_negative_stop_returns_all_but_trailing(self):
        ls = list(range(5))
        self.assertEqual(ls[:-2], [0, 1, 2])

    def test_getslice_with_positive_step_returns_forwards_list(self):
        ls = list(range(5))
        self.assertEqual(ls[::2], [0, 2, 4])

    def test_getslice_with_negative_step_returns_backwards_list(self):
        ls = list(range(5))
        self.assertEqual(ls[::-2], [4, 2, 0])

    def test_getslice_with_large_negative_start_returns_copy(self):
        ls = list(range(5))
        self.assertEqual(ls[-10:], ls)

    def test_getslice_with_large_positive_start_returns_empty(self):
        ls = list(range(5))
        self.assertEqual(ls[10:], [])

    def test_getslice_with_large_negative_start_returns_empty(self):
        ls = list(range(5))
        self.assertEqual(ls[:-10], [])

    def test_getslice_with_large_positive_start_returns_copy(self):
        ls = list(range(5))
        self.assertEqual(ls[:10], ls)

    def test_getslice_with_identity_slice_returns_copy(self):
        ls = list(range(5))
        copy = ls[::]
        self.assertEqual(copy, ls)
        self.assertIsNot(copy, ls)

    def test_getslice_with_none_slice_copies_list(self):
        original = [1, 2, 3]
        copy = original[:]
        self.assertEqual(len(copy), 3)
        self.assertEqual(copy[0], 1)
        self.assertEqual(copy[1], 2)
        self.assertEqual(copy[2], 3)

    def test_getslice_with_start_stop_step_returns_list(self):
        original = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        sliced = original[1:2:3]
        self.assertEqual(sliced, [2])

    def test_getslice_with_start_step_returns_list(self):
        original = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        sliced = original[1::3]
        self.assertEqual(sliced, [2, 5, 8])

    def test_getslice_with_start_stop_negative_step_returns_list(self):
        original = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        sliced = original[8:2:-2]
        self.assertEqual(sliced, [9, 7, 5])

    def test_getslice_with_start_stop_step_returns_empty_list(self):
        original = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        sliced = original[8:2:2]
        self.assertEqual(sliced, [])

    def test_getslice_in_list_comprehension(self):
        original = [1, 2, 3]
        result = [item[:] for item in [original] * 2]
        self.assertIsNot(result, original)
        self.assertEqual(len(result), 2)
        r1, r2 = result
        self.assertEqual(len(r1), 3)
        r11, r12, r13 = r1
        self.assertEqual(r11, 1)
        self.assertEqual(r12, 2)
        self.assertEqual(r13, 3)

    def test_gt_with_elem_gt_and_same_size_returns_false(self):
        class SubSet(set):
            def __gt__(self, other):
                return True

        a = [SubSet([1])]
        b = [SubSet([1])]
        self.assertFalse(list.__gt__(a, b))

    def test_gt_with_elem_gt_and_diff_size_returns_true(self):
        class SubSet(set):
            def __gt__(self, other):
                return True

        a = [SubSet([1])]
        b = [SubSet([1, 2])]
        self.assertTrue(list.__gt__(a, b))

    def test_gt_with_elem_eq_gt_and_diff_size_returns_false(self):
        class SubSet(set):
            def __eq__(self, other):
                return True

            def __gt__(self, other):
                return True

        a = [SubSet([1])]
        b = [SubSet([1, 2])]
        self.assertFalse(list.__gt__(a, b))

    def test_gt_longer_rhs_with_elem_eq_gt_and_diff_size_returns_true(self):
        class SubSet(set):
            def __eq__(self, other):
                return False

            def __gt__(self, other):
                return True

        a = [SubSet([1, 2])]
        b = [SubSet([1]), 2]
        self.assertTrue(list.__gt__(a, b))

    def test_gt_with_bigger_list_returns_true(self):
        a = [1, 2, 4]
        b = [1, 2, 3]
        self.assertTrue(list.__gt__(a, b))

    def test_gt_with_equal_lists_returns_false(self):
        a = [1, 2, 3]
        b = [1, 2, 3]
        self.assertFalse(list.__gt__(a, b))

    def test_gt_with_identity_lists_returns_false(self):
        a = [1, 2, 3]
        self.assertFalse(list.__gt__(a, a))

    def test_gt_with_non_list_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            list.__gt__(False, [])
        self.assertIn(
            "'__gt__' requires a 'list' object but received a 'bool'",
            str(context.exception),
        )

    def test_gt_with_shorter_lhs_but_bigger_elem_returns_true(self):
        a = [1, 2, 3]
        b = [1, 1, 1, 1]
        self.assertTrue(list.__gt__(a, b))

    def test_gt_with_shorter_lhs_returns_false(self):
        a = [1, 2]
        b = [1, 2, 3]
        self.assertFalse(list.__gt__(a, b))

    def test_gt_with_shorter_rhs_but_bigger_elem_returns_false(self):
        a = [1, 1, 1, 1]
        b = [1, 2, 3]
        self.assertFalse(list.__gt__(a, b))

    def test_gt_with_shorter_rhs_returns_true(self):
        a = [1, 2, 3, 4]
        b = [1, 2, 3]
        self.assertTrue(list.__gt__(a, b))

    def test_gt_with_smaller_list_returns_false(self):
        a = [1, 2, 2]
        b = [1, 2, 3]
        self.assertFalse(list.__gt__(a, b))

    def test_index_with_non_list_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            list.index(1, 2)
        self.assertIn(
            "'index' requires a 'list' object but received a 'int'",
            str(context.exception),
        )

    def test_index_with_none_start_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            [].index(1, None)
        self.assertEqual(
            str(context.exception),
            "slice indices must be integers or have an __index__ method",
        )

    def test_index_with_none_stop_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            [].index(1, 2, None)
        self.assertEqual(
            str(context.exception),
            "slice indices must be integers or have an __index__ method",
        )

    def test_index_with_negative_searches_relative_to_end(self):
        ls = list(range(10))
        self.assertEqual(ls.index(4, -7, -3), 4)

    def test_index_searches_from_left(self):
        ls = [1, 2, 1, 2, 1, 2, 1]
        self.assertEqual(ls.index(1, 1, -1), 2)

    def test_index_outside_of_bounds_raises_value_error(self):
        ls = list(range(10))
        with self.assertRaises(ValueError) as context:
            self.assertEqual(ls.index(4, 5), 4)
        self.assertEqual(str(context.exception), "4 is not in list")

    def test_index_calls_dunder_eq(self):
        class AlwaysEqual:
            def __eq__(self, other):
                return True

        class NeverEqual:
            def __eq__(self, other):
                return False

        a = AlwaysEqual()
        n = NeverEqual()
        a_list = [a, a, a, a, a]
        n_list = [n, n, n]
        self.assertEqual(a_list.index(a), 0)
        self.assertEqual(a_list.index(n, 1), 1)
        self.assertEqual(n_list.index(n, 2, 3), 2)

    def test_ge_with_elem_ge_and_same_size_returns_true(self):
        class SubSet(set):
            def __ge__(self, other):
                return True

        a = [SubSet([1])]
        b = [SubSet([1])]
        self.assertTrue(list.__ge__(a, b))

    def test_ge_with_elem_ge_and_diff_size_returns_true(self):
        class SubSet(set):
            def __ge__(self, other):
                return True

        a = [SubSet([1, 2])]
        b = [SubSet([1])]
        self.assertTrue(list.__ge__(a, b))

    def test_ge_with_elem_eq_ge_and_diff_size_returns_true(self):
        class SubSet(set):
            def __eq__(self, other):
                return True

            def __ge__(self, other):
                return True

        a = [SubSet([1, 2])]
        b = [SubSet([1])]
        self.assertTrue(list.__ge__(a, b))

    def test_ge_longer_lhs_with_elem_eq_ge_and_diff_size_returns_true(self):
        class SubSet(set):
            def __eq__(self, other):
                return False

            def __ge__(self, other):
                return True

        a = [SubSet([1, 2]), 2]
        b = [SubSet([1])]
        self.assertTrue(list.__ge__(a, b))

    def test_ge_with_bigger_list_returns_true(self):
        a = [1, 2, 4]
        b = [1, 2, 3]
        self.assertTrue(list.__ge__(a, b))

    def test_ge_with_equal_lists_returns_true(self):
        a = [1, 2, 3]
        b = [1, 2, 3]
        self.assertTrue(list.__ge__(a, b))

    def test_ge_with_identity_lists_returns_true(self):
        a = [1, 2, 3]
        self.assertTrue(list.__ge__(a, a))

    def test_ge_with_longer_lhs_but_smaller_elem_returns_false(self):
        a = [1, 1, 1, 1]
        b = [1, 2, 3]
        self.assertFalse(list.__ge__(a, b))

    def test_ge_with_longer_lhs_returns_true(self):
        a = [1, 2, 3, 4]
        b = [1, 2, 3]
        self.assertTrue(list.__ge__(a, b))

    def test_ge_with_longer_rhs_but_smaller_elem_returns_true(self):
        a = [1, 2, 3]
        b = [1, 1, 1, 1]
        self.assertTrue(list.__ge__(a, b))

    def test_ge_with_longer_rhs_returns_false(self):
        a = [1, 2]
        b = [1, 2, 3]
        self.assertFalse(list.__ge__(a, b))

    def test_ge_with_non_list_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            list.__ge__(False, [])
        self.assertIn(
            "'__ge__' requires a 'list' object but received a 'bool'",
            str(context.exception),
        )

    def test_le_with_elem_le_and_same_size_returns_true(self):
        class SubSet(set):
            def __le__(self, other):
                return True

        a = [SubSet([1])]
        b = [SubSet([1])]
        self.assertTrue(list.__le__(a, b))

    def test_le_with_elem_le_and_diff_size_returns_true(self):
        class SubSet(set):
            def __le__(self, other):
                return True

        a = [SubSet([1, 2])]
        b = [SubSet([1])]
        self.assertTrue(list.__le__(a, b))

    def test_le_with_elem_eq_le_and_diff_size_returns_true(self):
        class SubSet(set):
            def __eq__(self, other):
                return True

            def __le__(self, other):
                return True

        a = [SubSet([1, 2])]
        b = [SubSet([1])]
        self.assertTrue(list.__le__(a, b))

    def test_le_longer_lhs_with_elem_eq_le_and_diff_size_returns_true(self):
        class SubSet(set):
            def __eq__(self, other):
                return False

            def __le__(self, other):
                return True

        a = [SubSet([1, 2]), 2]
        b = [SubSet([1])]
        self.assertTrue(list.__le__(a, b))

    def test_le_with_bigger_list_returns_false(self):
        a = [1, 2, 4]
        b = [1, 2, 3]
        self.assertFalse(list.__le__(a, b))

    def test_le_with_equal_lists_returns_true(self):
        a = [1, 2, 3]
        b = [1, 2, 3]
        self.assertTrue(list.__le__(a, b))

    def test_le_with_identity_lists_returns_true(self):
        a = [1, 2, 3]
        self.assertTrue(list.__le__(a, a))

    def test_le_with_longer_lhs_but_smaller_elem_returns_true(self):
        a = [1, 1, 1, 1]
        b = [1, 2, 3]
        self.assertTrue(list.__le__(a, b))

    def test_le_with_longer_lhs_returns_false(self):
        a = [1, 2, 3, 4]
        b = [1, 2, 3]
        self.assertFalse(list.__le__(a, b))

    def test_le_with_longer_rhs_but_smaller_elem_returns_false(self):
        a = [1, 2, 3]
        b = [1, 1, 1, 1]
        self.assertFalse(list.__le__(a, b))

    def test_le_with_longer_rhs_returns_true(self):
        a = [1, 2]
        b = [1, 2, 3]
        self.assertTrue(list.__le__(a, b))

    def test_le_with_non_list_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            list.__le__(False, [])
        self.assertIn(
            "'__le__' requires a 'list' object but received a 'bool'",
            str(context.exception),
        )

    def test_le_with_smaller_list_returns_true(self):
        a = [1, 2, 2]
        b = [1, 2, 3]
        self.assertTrue(list.__le__(a, b))

    def test_lt_with_elem_lt_and_same_size_returns_false(self):
        class SubSet(set):
            def __lt__(self, other):
                return True

        a = [SubSet([1])]
        b = [SubSet([1])]
        self.assertFalse(list.__lt__(a, b))

    def test_lt_with_elem_lt_and_diff_size_returns_true(self):
        class SubSet(set):
            def __lt__(self, other):
                return True

        a = [SubSet([1, 2])]
        b = [SubSet([1])]
        self.assertTrue(list.__lt__(a, b))

    def test_lt_with_elem_eq_lt_and_diff_size_returns_false(self):
        class SubSet(set):
            def __eq__(self, other):
                return True

            def __lt__(self, other):
                return True

        a = [SubSet([1, 2])]
        b = [SubSet([1])]
        self.assertFalse(list.__lt__(a, b))

    def test_lt_longer_lhs_with_elem_eq_lt_and_diff_size_returns_true(self):
        class SubSet(set):
            def __eq__(self, other):
                return False

            def __lt__(self, other):
                return True

        a = [SubSet([1, 2]), 2]
        b = [SubSet([1])]
        self.assertTrue(list.__lt__(a, b))

    def test_lt_with_bigger_list_returns_false(self):
        a = [1, 2, 4]
        b = [1, 2, 3]
        self.assertFalse(list.__lt__(a, b))

    def test_lt_with_equal_lists_returns_false(self):
        a = [1, 2, 3]
        b = [1, 2, 3]
        self.assertFalse(list.__lt__(a, b))

    def test_lt_with_identity_lists_returns_false(self):
        a = [1, 2, 3]
        self.assertFalse(list.__lt__(a, a))

    def test_lt_with_longer_lhs_but_smaller_elem_returns_true(self):
        a = [1, 1, 1, 1]
        b = [1, 2, 3]
        self.assertTrue(list.__lt__(a, b))

    def test_lt_with_longer_lhs_returns_false(self):
        a = [1, 2, 3, 4]
        b = [1, 2, 3]
        self.assertFalse(list.__lt__(a, b))

    def test_lt_with_longer_rhs_but_smaller_elem_returns_false(self):
        a = [1, 2, 3]
        b = [1, 1, 1, 1]
        self.assertFalse(list.__lt__(a, b))

    def test_lt_with_longer_rhs_returns_true(self):
        a = [1, 2]
        b = [1, 2, 3]
        self.assertTrue(list.__lt__(a, b))

    def test_lt_with_non_list_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            list.__lt__(False, [])
        self.assertIn(
            "'__lt__' requires a 'list' object but received a 'bool'",
            str(context.exception),
        )

    def test_lt_with_smaller_list_returns_true(self):
        a = [1, 2, 2]
        b = [1, 2, 3]
        self.assertTrue(list.__lt__(a, b))

    def test_pop_with_non_list_raises_type_error(self):
        self.assertRaises(TypeError, list.pop, None)

    def test_pop_with_non_index_index_raises_type_error(self):
        self.assertRaises(TypeError, list.pop, [], "idx")

    def test_pop_with_empty_list_raises_index_error(self):
        self.assertRaises(IndexError, list.pop, [])

    def test_pop_with_no_args_pops_from_end_of_list(self):
        original = [1, 2, 3]
        self.assertEqual(original.pop(), 3)
        self.assertEqual(original, [1, 2])

    def test_pop_with_positive_in_bounds_arg_pops_from_list(self):
        original = [1, 2, 3]
        self.assertEqual(original.pop(1), 2)
        self.assertEqual(original, [1, 3])

    def test_pop_with_positive_out_of_bounds_arg_raises_index_error(self):
        original = [1, 2, 3]
        self.assertRaises(IndexError, list.pop, original, 10)

    def test_pop_with_negative_in_bounds_arg_pops_from_list(self):
        original = [1, 2, 3]
        self.assertEqual(original.pop(-1), 3)
        self.assertEqual(original, [1, 2])

    def test_pop_with_negative_out_of_bounds_arg_raises_index_error(self):
        original = [1, 2, 3]
        self.assertRaises(IndexError, list.pop, original, -10)

    def test_repr_returns_string(self):
        self.assertEqual(list.__repr__([]), "[]")
        self.assertEqual(list.__repr__([42]), "[42]")
        self.assertEqual(list.__repr__([1, 2, "hello"]), "[1, 2, 'hello']")

        class M(type):
            def __repr__(cls):
                return "<M instance>"

        class C(metaclass=M):
            def __repr__(self):
                return "<C instance>"

        self.assertEqual(list.__repr__([C, C()]), "[<M instance>, <C instance>]")

    def test_repr_with_recursive_list_prints_ellipsis(self):
        ls = []
        ls.append(ls)
        self.assertEqual(list.__repr__(ls), "[[...]]")

    def test_setslice_with_empty_slice_grows_list(self):
        grows = []
        grows[:] = [1, 2, 3]
        self.assertEqual(grows, [1, 2, 3])

    def test_setslice_with_list_subclass_calls_dunder_iter(self):
        class C(list):
            def __iter__(self):
                return ["a", "b", "c"].__iter__()

        grows = []
        grows[:] = C()
        self.assertEqual(grows, ["a", "b", "c"])

    def test_sort_with_big_list(self):
        ls = list(range(0, 1000))
        reversed_ls = list(reversed(ls))
        reversed_ls.sort()
        self.assertEqual(ls, reversed_ls)

    def test_sort_with_non_list_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            list.sort(None)
        self.assertIn(
            "'sort' requires a 'list' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_sort_with_key(self):
        ls = [1, 2, 3, 4, 5, 6]
        ls.sort(key=lambda x: -x)
        self.assertEqual(ls, [6, 5, 4, 3, 2, 1])

    def test_sort_with_key_handles_duplicates(self):
        ls = [1, 1, 2, 2]
        ls.sort(key=lambda x: -x)
        self.assertEqual(ls, [2, 2, 1, 1])

    def test_sort_with_key_handles_noncomparable_item(self):
        class C:
            def __init__(self, val):
                self.val = val

            def __eq__(self, other):
                return self.val == other.val

        ls = [C(i % 2) for i in range(4)]
        ls.sort(key=lambda x: x.val)
        self.assertEqual(ls, [C(0), C(0), C(1), C(1)])


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

    def test_dunder_doc_returns_updated_value(self):
        p = property()
        self.assertIsNone(p.__doc__)

        document_message = "this is for testing"
        p.__doc__ = document_message
        self.assertIs(p.__doc__, document_message)

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
    def test_dunder_init_returns_simple_namespace(self):
        from types import SimpleNamespace

        s = SimpleNamespace(foo=42, bar="baz")
        self.assertEqual(s.foo, 42)
        self.assertEqual(s.bar, "baz")

    def test_dunder_repr_returns_str(self):
        from types import SimpleNamespace

        s = SimpleNamespace(foo=42, bar="baz")
        r = SimpleNamespace.__repr__(s)
        self.assertTrue(
            r == "namespace(foo=42, bar='baz')" or r == "namespace(bar='baz', foo=42)"
        )


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


class StrTests(unittest.TestCase):
    def test_binary_add_with_non_str_other_falls_back_on_other_dunder_radd(self):
        class C:
            def __radd__(self, other):
                return (self, other)

        instance = C()
        self.assertEqual("a" + instance, (instance, "a"))

    def test_binary_add_with_str_subclass_other_falls_back_on_other_dunder_radd(self):
        class C(str):
            def __radd__(self, other):
                return (self, other)

        instance = C("foo")
        self.assertEqual("a" + instance, (instance, "a"))

    def test_dunder_getitem_with_int_returns_code_point(self):
        s = "a\u05D0b\u05D1c\u05D2"
        self.assertEqual(s[0], "a")
        self.assertEqual(s[1], "\u05D0")
        self.assertEqual(s[2], "b")
        self.assertEqual(s[3], "\u05D1")
        self.assertEqual(s[4], "c")
        self.assertEqual(s[5], "\u05D2")

    def test_dunder_getitem_with_large_int_raises_index_error(self):
        s = "hello"
        with self.assertRaises(IndexError) as context:
            s[2 ** 63]
        self.assertEqual(
            str(context.exception), "cannot fit 'int' into an index-sized integer"
        )

    def test_dunder_getitem_with_negative_int_indexes_from_end(self):
        s = "a\u05D0b\u05D1c\u05D2"
        self.assertEqual(s[-6], "a")
        self.assertEqual(s[-5], "\u05D0")
        self.assertEqual(s[-4], "b")
        self.assertEqual(s[-3], "\u05D1")
        self.assertEqual(s[-2], "c")
        self.assertEqual(s[-1], "\u05D2")

    def test_dunder_getitem_negative_out_of_bounds_raises_index_error(self):
        s = "hello"
        with self.assertRaises(IndexError) as context:
            s[-6]
        self.assertEqual(str(context.exception), "string index out of range")

    def test_dunder_getitem_positive_out_of_bounds_raises_index_error(self):
        s = "hello"
        with self.assertRaises(IndexError) as context:
            s[5]
        self.assertEqual(str(context.exception), "string index out of range")

    def test_dunder_getitem_with_slice_returns_str(self):
        s = "hello world"
        self.assertEqual(s[6:], "world")
        self.assertEqual(s[:5], "hello")
        self.assertEqual(s[::2], "hlowrd")
        self.assertEqual(s[::-1], "dlrow olleh")
        self.assertEqual(s[1:8:2], "el o")
        self.assertEqual(s[-1:3:-3], "doo")

    def test_dunder_getitem_with_slice_indexes_by_code_point(self):
        s = "# \xc2\xa9 2018 Unicode\xc2\xae, Inc.\n"
        self.assertEqual(s[10:], "Unicode\xc2\xae, Inc.\n")
        self.assertEqual(s[:9], "# \xc2\xa9 2018")
        self.assertEqual(s[::2], "#\xc2 08Uioe\xae n.")
        self.assertEqual(s[1:8:2], " \xa921")
        self.assertEqual(s[-1:3:-3], "\nn,ecU1 ")

    def test_dunder_getitem_with_slice_uses_adjusted_bounds(self):
        s = "hello world"
        self.assertEqual(s[-20:5], "hello")
        self.assertEqual(s[6:42], "world")

    def test_expandtabs_with_non_int_raises_type_error(self):
        with self.assertRaises(TypeError):
            "expand\tme".expandtabs("not-int")

    def test_expandtabs_with_no_args_expands_to_eight_spaces(self):
        s = "expand\tme"
        self.assertEqual(s.expandtabs(), "expand  me")

    def test_expandtabs_with_zero_deletes_tabs(self):
        s = "expand\tme"
        self.assertEqual(s.expandtabs(0), "expandme")

    def test_expandtabs_expands_column_to_given_number_of_spaces(self):
        s = "expand\tme"
        self.assertEqual(s.expandtabs(3), "expand   me")

    def test_expandtabs_with_newlines_resets_col_position(self):
        s = "123\t12345\t\n1234\t1\r12\t1234\t123\t1"
        self.assertEqual(s.expandtabs(6), "123   12345 \n1234  1\r12    1234  123   1")

    def test_title_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.title(123)

    def test_title_with_empty_string_returns_itself(self):
        a = ""
        self.assertIs(a.title(), "")

    def test_title_with_non_empty_string_returns_new_string(self):
        a = "Hello World"
        result = a.title()
        self.assertEqual(a, result)
        self.assertIsNot(a, result)

    def test_title_with_single_lowercase_word_returns_titlecased(self):
        self.assertEqual("abc".title(), "Abc")

    def test_title_with_single_uppercase_word_returns_titlecased(self):
        self.assertEqual("ABC".title(), "Abc")

    def test_title_with_multiple_words_returns_titlecased(self):
        self.assertEqual("foo bar baz".title(), "Foo Bar Baz")

    def test_title_with_words_with_special_chars_eturns_titlecased(self):
        self.assertEqual(
            "they're bill's friends from the UK".title(),
            "They'Re Bill'S Friends From The Uk",
        )

    def test_translate_without_dict(self):
        with self.assertRaises(TypeError):
            "abc".translate(123)

    def test_translate_without_non_valid_dict_value(self):
        with self.assertRaises(TypeError):
            "abc".translate({97: 123.456}, "int.*, None or str")

    def test_translate_without_characters_in_dict_returns_same_str(self):
        original = "abc"
        expected = "abc"
        table = {120: 130, 121: 131, 122: 132}
        self.assertEqual(original.translate(table), expected)

    def test_translate_with_none_values_returns_substr(self):
        original = "abc"
        expected = "bc"
        table = {97: None}
        self.assertEqual(original.translate(table), expected)

    def test_translate_returns_new_str(self):
        original = "abc"
        expected = "dbc"
        table = {97: 100}
        self.assertEqual(original.translate(table), expected)

    def test_translate_returns_longer_str(self):
        original = "abc"
        expected = "defbc"
        table = {97: "def"}
        self.assertEqual(original.translate(table), expected)

    def test_translate_with_short_str_returns_same_str(self):
        original = "foobar"
        expected = "foobar"
        table = "baz"
        self.assertEqual(original.translate(table), expected)

    def test_translate_with_lowercase_ascii_can_lowercase(self):
        table = "".join(
            [chr(i).lower() if chr(i).isupper() else chr(i) for i in range(128)]
        )
        original = "FoOBaR"
        expected = "foobar"
        self.assertEqual(original.translate(table), expected)

    def test_translate_with_getitem_calls_it(self):
        class Sequence:
            def __getitem__(self, index):
                if index == ord("a"):
                    return "z"
                if index == ord("b"):
                    return None
                if index == ord("c"):
                    return 0xC6
                raise LookupError

        original = "abcd"
        expected = "z\u00c6d"
        self.assertEqual(original.translate(Sequence()), expected)

    def test_translate_with_getitem_calls_type_slot(self):
        class Sequence:
            def __getitem__(self, index):
                if index == ord("a"):
                    return "z"

        s = Sequence()
        s.__getitem__ = None
        self.assertEqual("a".translate(s), "z")

    def test_translate_with_invalid_getitem_raises_type_error(self):
        class Sequence:
            def __getitem__(self, index):
                if index == ord("a"):
                    return b"a"

        with self.assertRaises(TypeError):
            "invalid".translate(Sequence())

    def test_maketrans_dict_with_non_str_or_int_keys_raises_type_error(self):
        with self.assertRaisesRegex(TypeError, "translate table must be"):
            str.maketrans({123.456: 2})

    def test_maketrans_dict_with_int_key_returns_translation_table(self):
        result = str.maketrans({97: 2})
        expected = {97: 2}
        self.assertEqual(result, expected)

    def test_maketrans_dict_with_str_subtype_key_returns_translation_table(self):
        class substr(str):
            pass

        result = str.maketrans({substr("a"): 2})
        expected = {97: 2}
        self.assertEqual(result, expected)

    def test_maketrans_with_dict_returns_translation_table(self):
        result = str.maketrans({"a": 2})
        expected = {97: 2}
        self.assertEqual(result, expected)

    def test_maketrans_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.maketrans(1, "b")
        with self.assertRaises(TypeError):
            str.maketrans("a", 98)

    def test_maketrans_with_non_equal_len_str_raises_value_error(self):
        with self.assertRaisesRegex(ValueError, "equal len"):
            str.maketrans("a", "ab")

    def test_maketrans_with_str_returns_translation_table(self):
        result = str.maketrans("abc", "def")
        expected = {97: 100, 98: 101, 99: 102}
        self.assertEqual(result, expected)

    def test_maketrans_with_third_arg_non_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.maketrans("abc", "def", 123)

    def test_maketrans_with_third_arg_returns_translation_table_with_none(self):
        result = str.maketrans("abc", "def", "cd")
        expected = {97: 100, 98: 101, 99: None, 100: None}
        self.assertEqual(result, expected)

    def test_dunder_new_with_raising_dunder_str_propagates_exception(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("failed")

        class Foo:
            __str__ = Desc()

        foo = Foo()
        with self.assertRaises(AttributeError) as context:
            str(foo)
        self.assertEqual(str(context.exception), "failed")

    def test_capitalize_with_non_str_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.capitalize(1)
        self.assertIn(
            "'capitalize' requires a 'str' object but received a 'int'",
            str(context.exception),
        )

    def test_casefold_with_non_str_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.casefold(1)
        self.assertIn(
            "'casefold' requires a 'str' object but received a 'int'",
            str(context.exception),
        )

    def test_casefold_with_empty_string_returns_itself(self):
        self.assertEqual("".casefold(), "")

    def test_casefold_with_ascii_returns_folded_str(self):
        self.assertEqual("abc".casefold(), "abc")
        self.assertEqual("ABC".casefold(), "abc")
        self.assertEqual("aBcDe".casefold(), "abcde")
        self.assertEqual("aBc123".casefold(), "abc123")

    def test_casefold_with_nonascii_returns_folded_str(self):
        self.assertEqual("\u00c5\u00c6\u00c7".casefold(), "\u00e5\u00e6\u00e7")
        self.assertEqual("\u00c5A\u00c6B\u00c7".casefold(), "\u00e5a\u00e6b\u00e7")
        self.assertEqual("A\u00c5\u00c6\u00c7C".casefold(), "a\u00e5\u00e6\u00e7c")

    def test_casefold_with_lower_extended_case_returns_folded_str(self):
        self.assertEqual("der Flu\u00DF".casefold(), "der fluss")
        self.assertEqual("\u00DF".casefold(), "ss")

    def test_casefold_with_str_subclass_returns_folded_str(self):
        class C(str):
            pass

        s = C("der Flu\u00DF").casefold()
        self.assertIs(type(s), str)
        self.assertEqual(s, "der fluss")

    def test_center_with_non_int_index_width_calls_dunder_index(self):
        class W:
            def __index__(self):
                return 5

        self.assertEqual(str.center("abc", W()), " abc ")

    def test_center_returns_justified_string(self):
        self.assertEqual(str.center("abc", -1), "abc")
        self.assertEqual(str.center("abc", 0), "abc")
        self.assertEqual(str.center("abc", 1), "abc")
        self.assertEqual(str.center("abc", 2), "abc")
        self.assertEqual(str.center("abc", 3), "abc")
        self.assertEqual(str.center("abc", 4), "abc ")
        self.assertEqual(str.center("abc", 5), " abc ")
        self.assertEqual(str.center("abc", 6), " abc  ")
        self.assertEqual(str.center("abc", 7), "  abc  ")
        self.assertEqual(str.center("abc", 8), "  abc   ")
        self.assertEqual(str.center("", 4), "    ")
        self.assertEqual(str.center("\t \n", 6), " \t \n  ")

    def test_center_with_custom_fillchar_returns_str(self):
        self.assertEqual(str.center("ba", 7, "@"), "@@@ba@@")
        self.assertEqual(
            str.center("x\U0001f43bx", 6, "\U0001f40d"),
            "\U0001f40dx\U0001f43bx\U0001f40d\U0001f40d",
        )

    def test_center_returns_identity(self):
        s = "foo bar baz bam!"
        self.assertIs(str.center(s, -1), s)
        self.assertIs(str.center(s, 5), s)
        self.assertIs(str.center(s, len(s)), s)

    def test_center_with_fillchar_not_char_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, "The fill character must be exactly one character long"
        ):
            str.center("", 2, "")
        with self.assertRaisesRegex(
            TypeError, "The fill character must be exactly one character long"
        ):
            str.center("", 2, ",,")

    def test_count_with_non_str_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.count(None, "foo")

    def test_count_with_non_str_other_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.count("foo", None)

    def test_count_calls_dunder_index_on_start(self):
        class C:
            def __index__(self):
                raise UserWarning("foo")

        c = C()
        with self.assertRaises(UserWarning):
            str.count("foo", "bar", c, 100)

    def test_count_calls_dunder_index_on_end(self):
        class C:
            def __index__(self):
                raise UserWarning("foo")

        c = C()
        with self.assertRaises(UserWarning):
            str.count("foo", "bar", 0, c)

    def test_count_returns_number_of_occurrences(self):
        self.assertEqual("foo".count("o"), 2)

    def test_encode_idna_returns_ascii_encoded_str(self):
        self.assertEqual("foo".encode("idna"), b"foo")

    def test_endswith_with_non_str_or_tuple_prefix_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            "".endswith([])
        self.assertEqual(
            str(context.exception),
            "endswith first arg must be str or a tuple of str, not list",
        )

    def test_endswith_with_non_str_or_tuple_prefix_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            "hello".endswith(("e", b"", "h"))
        self.assertEqual(
            str(context.exception),
            "tuple for endswith must only contain str, not bytes",
        )

    def test_endswith_with_empty_string_returns_true_for_valid_bounds(self):
        self.assertTrue("".endswith(""))
        self.assertTrue("".endswith("", -1))
        self.assertTrue("".endswith("", 0, 10))
        self.assertTrue("hello".endswith(""))
        self.assertTrue("hello".endswith("", 5))
        self.assertTrue("hello".endswith("", 2, -1))

    def test_endswith_with_empty_string_returns_false_for_invalid_bounds(self):
        self.assertFalse("".endswith("", 1))
        self.assertFalse("hello".endswith("", 6))
        self.assertFalse("hello".endswith("", -1, 1))

    def test_endswith_with_suffix_returns_true(self):
        self.assertTrue("hello".endswith("o"))
        self.assertTrue("hello".endswith("lo"))
        self.assertTrue("hello".endswith("llo"))
        self.assertTrue("hello".endswith("ello"))
        self.assertTrue("hello".endswith("hello"))

    def test_endswith_with_nonsuffix_returns_false(self):
        self.assertFalse("hello".endswith("l"))
        self.assertFalse("hello".endswith("allo"))
        self.assertFalse("hello".endswith("he"))
        self.assertFalse("hello".endswith("elo"))
        self.assertFalse("hello".endswith("foo"))

    def test_endswith_with_substr_at_end_returns_true(self):
        self.assertTrue("hello".endswith("e", 0, 2))
        self.assertTrue("hello".endswith("ll", 1, 4))
        self.assertTrue("hello".endswith("lo", 2, 7))
        self.assertTrue("hello".endswith("o", 0, 100))

    def test_endswith_outside_bounds_returns_false(self):
        self.assertFalse("hello".endswith("hello", 0, 4))
        self.assertFalse("hello".endswith("llo", 1, 4))
        self.assertFalse("hello".endswith("hell", 0, 2))
        self.assertFalse("hello".endswith("o", 2, 4))

    def test_endswith_with_negative_bounds_relative_to_end(self):
        self.assertTrue("hello".endswith("ll", 1, -1))
        self.assertTrue("hello".endswith("e", 0, -3))
        self.assertFalse("hello".endswith("ello", -10, -1))

    def test_endswith_with_tuple(self):
        self.assertTrue("hello".endswith(("l", "o", "l")))
        self.assertFalse("hello".endswith(("foo", "bar")))

    def test_format_single_open_curly_brace_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.format("{")
        self.assertEqual(
            str(context.exception), "Single '{' encountered in format string"
        )

    def test_format_single_close_curly_brace_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.format("}")
        self.assertEqual(
            str(context.exception), "Single '}' encountered in format string"
        )

    def test_format_index_out_of_args_raises_index_error(self):
        with self.assertRaises(IndexError) as context:
            str.format("{1}", 4)
        self.assertIn("index out of range", str(context.exception))

    def test_format_not_existing_key_in_kwargs_raises_key_error(self):
        with self.assertRaises(KeyError) as context:
            str.format("{a}", b=4)
        self.assertEqual(str(context.exception), "'a'")

    def test_format_not_index_field_not_in_kwargs_raises_key_error(self):
        with self.assertRaises(KeyError) as context:
            str.format("{-1}", b=4)
        self.assertEqual(str(context.exception), "'-1'")

    def test_format_str_without_field_returns_itself(self):
        result = str.format("no field added")
        self.assertEqual(result, "no field added")

    def test_format_double_open_curly_braces_returns_single(self):
        result = str.format("{{")
        self.assertEqual(result, "{")

    def test_format_double_close_open_curly_braces_returns_single(self):
        result = str.format("}}")
        self.assertEqual(result, "}")

    def test_format_auto_index_field_returns_str_replaced_for_matching_args(self):
        result = str.format("a{}b{}c", 0, 1)
        self.assertEqual(result, "a0b1c")

    def test_format_auto_index_field_with_explicit_index_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.format("a{}b{0}c", 0)
        self.assertIn("cannot switch", str(context.exception))
        self.assertIn("automatic field numbering", str(context.exception))
        self.assertIn("manual field specification", str(context.exception))

    def test_format_auto_index_field_with_keyword_returns_formatted_str(self):
        result = str.format("a{}b{keyword}c{}d", 0, 1, keyword=888)
        self.assertEqual(result, "a0b888c1d")

    def test_format_explicit_index_fields_returns_formatted_str(self):
        result = str.format("a{2}b{1}c{0}d{1}e", 0, 1, 2)
        self.assertEqual(result, "a2b1c0d1e")

    def test_format_keyword_fields_returns_formatted_str(self):
        result = str.format("1{a}2{b}3{c}4{b}5", a="a", b="b", c="c")
        self.assertEqual(result, "1a2b3c4b5")

    def test_format_with_unnamed_subscript(self):
        result = str.format("{[1]}", (1, 2, 3))
        self.assertEqual(result, "2")

    def test_format_with_numbered_subscript(self):
        result = str.format("{1[1]}", (1, 2, 3), (4, 5, 6))
        self.assertEqual(result, "5")

    def test_format_with_named_subscript(self):
        result = str.format("{b[1]}", a=(1, 2, 3), b=(4, 5, 6))
        self.assertEqual(result, "5")

    def test_format_with_multiple_subscripts(self):
        result = str.format("{b[1][2]}", a=(1, 2, 3), b=(4, (5, 6, 7), 6))
        self.assertEqual(result, "7")

    def test_isalnum_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isalnum())) for x in range(128)),
            "0000000000000000000000000000000000000000000000001111111111000000"
            "0111111111111111111111111110000001111111111111111111111111100000",
        )

    def test_isalnum_with_empty_string_returns_false(self):
        self.assertFalse("".isalnum())

    def test_isalnum_with_multichar_string_returns_bool(self):
        self.assertTrue("1a5b".isalnum())
        self.assertFalse("1b)".isalnum())

    def test_isalnum_with_non_ascii_returns_bool(self):
        self.assertTrue("\u0e50ab2\u00e9".isalnum())
        self.assertFalse("\u0e50 \u00e9".isalnum())

    def test_isalnum_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isalnum(None)
        self.assertIn("'isalnum' requires a 'str' object", str(context.exception))

    def test_isalpha_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isalpha())) for x in range(128)),
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0111111111111111111111111110000001111111111111111111111111100000",
        )

    def test_isalpha_with_empty_string_returns_false(self):
        self.assertFalse("".isalpha())

    def test_isalpha_with_multichar_string_returns_bool(self):
        self.assertTrue("hElLo".isalpha())
        self.assertFalse("x8".isalpha())

    def test_isalpha_with_non_ascii_returns_bool(self):
        self.assertTrue("resum\u00e9".isalpha())
        self.assertTrue("\u00c9tude".isalpha())
        self.assertTrue("\u01c8udevit".isalpha())

        self.assertFalse("23\u00e9".isalpha())
        self.assertFalse("\u01c8 ".isalpha())

    def test_isalpha_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isalpha(None)
        self.assertIn("'isalpha' requires a 'str' object", str(context.exception))

    @unittest.skipUnless(hasattr(str, "isascii"), "Added in 3.7")
    def test_isascii_with_empty_string_returns_true(self):
        self.assertTrue("".isascii())

    @unittest.skipUnless(hasattr(str, "isascii"), "Added in 3.7")
    def test_isascii_with_ascii_values_returns_true(self):
        self.assertTrue("howdy".isascii())
        self.assertTrue("\x00".isascii())
        self.assertTrue("\x7f".isascii())
        self.assertTrue("\x00\x7f".isascii())

    @unittest.skipUnless(hasattr(str, "isascii"), "Added in 3.7")
    def test_isascii_with_nonascii_values_return_false(self):
        self.assertFalse("\x80".isascii())
        self.assertFalse("\xe9".isascii())

    def test_isdecimal_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isdecimal())) for x in range(128)),
            "0000000000000000000000000000000000000000000000001111111111000000"
            "0000000000000000000000000000000000000000000000000000000000000000",
        )

    def test_isdecimal_with_empty_string_returns_false(self):
        self.assertFalse("".isdecimal())

    def test_isdecimal_with_multichar_string_returns_bool(self):
        self.assertTrue("8725".isdecimal())
        self.assertFalse("8-4".isdecimal())

    def test_isdecimal_with_non_ascii_returns_bool(self):
        self.assertTrue("\u0e50".isdecimal())  # THAI DIGIT ZERO
        self.assertFalse("\u00b2".isdecimal())  # SUPERSCRIPT TWO
        self.assertFalse("\u00bd".isdecimal())  # VULGAR FRACTION ONE HALF

    def test_isdecimal_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isdecimal(None)
        self.assertIn("'isdecimal' requires a 'str' object", str(context.exception))

    def test_isdigit_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isdigit())) for x in range(128)),
            "0000000000000000000000000000000000000000000000001111111111000000"
            "0000000000000000000000000000000000000000000000000000000000000000",
        )

    def test_isdigit_with_empty_string_returns_false(self):
        self.assertFalse("".isdigit())

    def test_isdigit_with_multichar_string_returns_bool(self):
        self.assertTrue("8725".isdigit())
        self.assertFalse("8-4".isdigit())

    def test_isdigit_with_non_ascii_returns_bool(self):
        self.assertTrue("\u0e50".isdigit())  # THAI DIGIT ZERO
        self.assertTrue("\u00b2".isdigit())  # SUPERSCRIPT TWO
        self.assertFalse("\u00bd".isdigit())  # VULGAR FRACTION ONE HALF

    def test_isdigit_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isdigit(None)
        self.assertIn("'isdigit' requires a 'str' object", str(context.exception))

    def test_isidentifier_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isidentifier())) for x in range(128)),
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0111111111111111111111111110000101111111111111111111111111100000",
        )
        self.assertEqual(
            "".join(str(int(("_" + chr(x)).isidentifier())) for x in range(128)),
            "0000000000000000000000000000000000000000000000001111111111000000"
            "0111111111111111111111111110000101111111111111111111111111100000",
        )

    def test_isidentifier_with_empty_string_returns_false(self):
        self.assertFalse("".isidentifier())

    def test_isidentifier_with_multichar_string_returns_bool(self):
        self.assertTrue("foo_8".isidentifier())
        self.assertTrue("Foo_8".isidentifier())
        self.assertTrue("_FOO_8".isidentifier())
        self.assertFalse("foo bar".isidentifier())

    def test_isidentifier_with_non_ascii_string_returns_bool(self):
        self.assertTrue("resum\u00e9".isidentifier())
        self.assertTrue("\u00e9tude".isidentifier())
        self.assertFalse("\U0001F643".isidentifier())

    def test_isidentifier_with_invalid_start_returns_false(self):
        self.assertFalse("8foo".isidentifier())
        self.assertFalse(".foo".isidentifier())

    def test_isidentifier_with_symbol_returns_false(self):
        self.assertFalse("foo!".isidentifier())
        self.assertFalse("foo.bar".isidentifier())

    def test_isidentifier_allows_underscore_start(self):
        self.assertTrue("_hello".isidentifier())
        self.assertFalse("_world!".isidentifier())

    def test_isidentifier_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isidentifier(None)
        self.assertIn("'isidentifier' requires a 'str' object", str(context.exception))

    def test_islower_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).islower())) for x in range(128)),
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0000000000000000000000000000000001111111111111111111111111100000",
        )

    def test_islower_with_empty_string_returns_false(self):
        self.assertFalse("".islower())

    def test_islower_with_multichar_string_returns_bool(self):
        self.assertTrue("hello".islower())
        self.assertTrue("...a...".islower())
        self.assertFalse("hEllo".islower())
        self.assertFalse("...A...".islower())
        self.assertFalse("......".islower())

    def test_islower_with_non_ascii_returns_bool(self):
        self.assertTrue("resum\u00e9".islower())
        self.assertFalse("\u00c9tude".islower())  # uppercase
        self.assertFalse("\u01c8udevit".islower())  # titlecase

    def test_islower_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.islower(None)
        self.assertIn("'islower' requires a 'str' object", str(context.exception))

    def test_isnumeric_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isnumeric())) for x in range(128)),
            "0000000000000000000000000000000000000000000000001111111111000000"
            "0000000000000000000000000000000000000000000000000000000000000000",
        )

    def test_isnumeric_with_empty_string_returns_false(self):
        self.assertFalse("".isnumeric())

    def test_isnumeric_with_multichar_string_returns_bool(self):
        self.assertTrue("28741".isnumeric())
        self.assertFalse("5e4".isnumeric())

    def test_isnumeric_with_non_ascii_returns_bool(self):
        self.assertTrue("\u0e50".isnumeric())  # THAI DIGIT ZERO
        self.assertTrue("\u00b2".isnumeric())  # SUPERSCRIPT TWO
        self.assertTrue("\u00bd".isnumeric())  # VULGAR FRACTION ONE HALF

    def test_isnumeric_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isnumeric(None)
        self.assertIn("'isnumeric' requires a 'str' object", str(context.exception))

    def test_isprintable_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isprintable())) for x in range(128)),
            "0000000000000000000000000000000011111111111111111111111111111111"
            "1111111111111111111111111111111111111111111111111111111111111110",
        )

    def test_isprintable_with_empty_string_returns_true(self):
        self.assertTrue("".isprintable())

    def test_isprintable_with_multichar_string_returns_bool(self):
        self.assertTrue("Hello World!".isprintable())
        self.assertFalse("Hello\tWorld!".isprintable())

    def test_isprintable_with_non_ascii_returns_bool(self):
        self.assertTrue("\u0e50 foo \u00e3!".isprintable())
        self.assertFalse("\u0e50 \t \u00e3!".isprintable())

    def test_isprintable_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isprintable(None)
        self.assertIn("'isprintable' requires a 'str' object", str(context.exception))

    def test_isspace_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isspace())) for x in range(128)),
            "0000000001111100000000000000111110000000000000000000000000000000"
            "0000000000000000000000000000000000000000000000000000000000000000",
        )

    def test_isspace_with_empty_string_returns_false(self):
        self.assertFalse("".isspace())

    def test_isspace_with_multichar_string_returns_bool(self):
        self.assertTrue(" \t\r\n".isspace())
        self.assertFalse(" _".isspace())

    def test_isspace_with_multichar_unicode_spaces_returns_true(self):
        self.assertTrue(" \t\u3000\n\u202f".isspace())

    def test_isspace_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isspace(None)
        self.assertIn("'isspace' requires a 'str' object", str(context.exception))

    def test_istitle_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).istitle())) for x in range(128)),
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0111111111111111111111111110000000000000000000000000000000000000",
        )
        self.assertEqual(
            "".join(str(int(("A" + chr(x)).istitle())) for x in range(128)),
            "1111111111111111111111111111111111111111111111111111111111111111"
            "1000000000000000000000000001111111111111111111111111111111111111",
        )

    def test_istitle_with_empty_string_returns_false(self):
        self.assertFalse("".istitle())

    def test_istitle_with_multichar_string_returns_bool(self):
        self.assertTrue("Hello\t!".istitle())
        self.assertTrue("...A...".istitle())
        self.assertTrue("Aa Bbb Cccc".istitle())
        self.assertFalse("HeLlo".istitle())
        self.assertFalse("...a...".istitle())
        self.assertFalse("...".istitle())

    def test_istitle_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.istitle(None)
        self.assertIn("'istitle' requires a 'str' object", str(context.exception))

    def test_istitle_with_non_ascii_returns_bool(self):
        self.assertTrue("Resum\u00e9".istitle())
        self.assertTrue("\u00c9tude".istitle())
        self.assertTrue("\u01c8udevit".istitle())

        self.assertFalse("resum\u00e9".istitle())
        self.assertFalse("\u00c9tudE".istitle())
        self.assertFalse("L\u01c8udevit".istitle())

    def test_isupper_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isupper())) for x in range(128)),
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0111111111111111111111111110000000000000000000000000000000000000",
        )

    def test_isupper_with_empty_string_returns_false(self):
        self.assertFalse("".isupper())

    def test_isupper_with_multichar_string_returns_bool(self):
        self.assertTrue("HELLO".isupper())
        self.assertTrue("...A...".isupper())
        self.assertFalse("HElLLO".isupper())
        self.assertFalse("...a...".isupper())
        self.assertFalse("......".isupper())
        self.assertFalse("12345".isupper())
        self.assertTrue("ABC12345".isupper())
        self.assertFalse("ABC12345abc".isupper())

    def test_isupper_with_nonascii_string_returns_bool(self):
        self.assertFalse("RESUM\u00e9".isupper())  # lowercase
        self.assertTrue("\u00c9TUDE".isupper())  # uppercase
        self.assertFalse("\u01c8UDEVIT".isupper())  # titlecase

    def test_isupper_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isupper(None)
        self.assertIn("'isupper' requires a 'str' object", str(context.exception))

    def test_join_with_raising_descriptor_dunder_iter_raises_type_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise UserWarning("foo")

        class C:
            __iter__ = Desc()

        with self.assertRaises(TypeError):
            str.join("", C())

    def test_join_with_non_string_in_items_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.join(",", ["hello", 1])
        self.assertEqual(
            str(context.exception), "sequence item 1: expected str instance, int found"
        )

    def test_join_with_non_string_separator_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.join(None, ["hello", "world"])
        self.assertTrue(
            str(context.exception).endswith(
                "'join' requires a 'str' object but received a 'NoneType'"
            )
        )

    def test_join_with_empty_items_returns_empty_string(self):
        result = str.join(",", ())
        self.assertEqual(result, "")

    def test_join_with_one_item_returns_item(self):
        result = str.join(",", ("foo",))
        self.assertEqual(result, "foo")

    def test_join_with_empty_separator_uses_empty_string(self):
        result = str.join("", ("1", "2", "3"))
        self.assertEqual(result, "123")

    def test_join_with_tuple_joins_elements(self):
        result = str.join(",", ("1", "2", "3"))
        self.assertEqual(result, "1,2,3")

    def test_join_with_tuple_subclass_calls_dunder_iter(self):
        class C(tuple):
            def __iter__(self):
                return ("a", "b", "c").__iter__()

        elements = C(("p", "q", "r"))
        result = "".join(elements)
        self.assertEqual(result, "abc")

    def test_ljust_returns_justified_string(self):
        self.assertEqual(str.ljust("abc", -1), "abc")
        self.assertEqual(str.ljust("abc", 0), "abc")
        self.assertEqual(str.ljust("abc", 1), "abc")
        self.assertEqual(str.ljust("abc", 2), "abc")
        self.assertEqual(str.ljust("abc", 3), "abc")
        self.assertEqual(str.ljust("abc", 4), "abc ")
        self.assertEqual(str.ljust("abc", 5), "abc  ")
        self.assertEqual(str.ljust("abc", 6), "abc   ")
        self.assertEqual(str.ljust("", 4), "    ")
        self.assertEqual(str.ljust("\t \n", 5), "\t \n  ")

    def test_ljust_with_custom_fillchar_returns_str(self):
        self.assertEqual(str.ljust("ba", 8, "@"), "ba@@@@@@")
        self.assertEqual(
            str.ljust("x\U0001f43bx", 5, "\U0001f40d"),
            "x\U0001f43bx\U0001f40d\U0001f40d",
        )

    def test_ljust_returns_identity(self):
        s = "foo bar baz bam!"
        self.assertIs(str.ljust(s, -1), s)
        self.assertIs(str.ljust(s, 5), s)
        self.assertIs(str.ljust(s, len(s)), s)

    def test_ljust_with_non_int_index_width_calls_dunder_index(self):
        class W:
            def __index__(self):
                return 5

        self.assertEqual(str.ljust("abc", W()), "abc  ")

    def test_ljust_with_fillchar_not_char_raises_type_error(self):
        self.assertRaises(TypeError, str.ljust, "", 2, "")
        self.assertRaises(TypeError, str.ljust, "", 2, ",,")
        self.assertRaises(TypeError, str.ljust, "", 2, 3)

    def test_lower_returns_lowercased_string(self):
        self.assertEqual(str.lower("HELLO"), "hello")
        self.assertEqual(str.lower("HeLLo_WoRlD"), "hello_world")
        self.assertEqual(str.lower("hellO World!"), "hello world!")
        self.assertEqual(str.lower(""), "")
        self.assertEqual(str.lower("1234"), "1234")
        self.assertEqual(str.lower("$%^*("), "$%^*(")

    def test_lower_with_non_ascii_returns_lowercased_string(self):
        self.assertEqual(str.lower("foo\u01BCBAR"), "foo\u01BDbar")
        # uppercase without lowercase
        self.assertEqual(str.lower("FOO\U0001D581bar"), "foo\U0001D581bar")
        # one to many lowercasing
        self.assertEqual(str.lower("foo\u0130bar"), "foo\x69\u0307bar")

    def test_lower_with_str_subclass_returns_str(self):
        class C(str):
            pass

        self.assertEqual(str.lower(C("HeLLo")), "hello")
        self.assertIs(type(str.lower(C("HeLLo"))), str)
        self.assertIs(type(str.lower(C(""))), str)
        self.assertIs(type(str.lower(C("lower"))), str)

    def test_replace(self):
        test = "mississippi"
        self.assertEqual(test.replace("i", "a"), "massassappa")
        self.assertEqual(test.replace("i", "vv"), "mvvssvvssvvppvv")
        self.assertEqual(test.replace("ss", "x"), "mixixippi")

    def test_replace_with_count(self):
        test = "mississippi"
        self.assertEqual(test.replace("i", "a", 0), "mississippi")
        self.assertEqual(test.replace("i", "a", 2), "massassippi")

    def test_replace_int_error(self):
        test = "a b"
        self.assertRaises(TypeError, test.replace, 32, "")

    def test_rindex_with_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.rindex(b"", "")

    def test_rindex_with_subsequence_returns_last_in_range(self):
        haystack = "-a-a--a--"
        needle = "a"
        self.assertEqual(haystack.rindex(needle, 1, 6), 3)

    def test_rindex_with_missing_raises_value_error(self):
        haystack = "abc"
        needle = "d"
        with self.assertRaises(ValueError) as context:
            haystack.rindex(needle)
        self.assertEqual(str(context.exception), "substring not found")

    def test_rindex_outside_of_bounds_raises_value_error(self):
        haystack = "abc"
        needle = "c"
        with self.assertRaises(ValueError) as context:
            haystack.rindex(needle, 0, 2)
        self.assertEqual(str(context.exception), "substring not found")

    def test_rjust_returns_justified_string(self):
        self.assertEqual(str.rjust("abc", -1), "abc")
        self.assertEqual(str.rjust("abc", 0), "abc")
        self.assertEqual(str.rjust("abc", 1), "abc")
        self.assertEqual(str.rjust("abc", 2), "abc")
        self.assertEqual(str.rjust("abc", 3), "abc")
        self.assertEqual(str.rjust("abc", 4), " abc")
        self.assertEqual(str.rjust("abc", 5), "  abc")
        self.assertEqual(str.rjust("abc", 6), "   abc")
        self.assertEqual(str.rjust("", 4), "    ")
        self.assertEqual(str.rjust("\t \n", 5), "  \t \n")

    def test_rjust_with_custom_fillchar_returns_str(self):
        self.assertEqual(str.rjust("ba", 8, "@"), "@@@@@@ba")
        self.assertEqual(
            str.rjust("x\U0001f43bx", 5, "\U0001f40d"),
            "\U0001f40d\U0001f40dx\U0001f43bx",
        )

    def test_rjust_returns_identity(self):
        s = "foo bar baz bam!"
        self.assertIs(str.rjust(s, -1), s)
        self.assertIs(str.rjust(s, 5), s)
        self.assertIs(str.rjust(s, len(s)), s)

    def test_rjust_with_non_int_index_width_calls_dunder_index(self):
        class W:
            def __index__(self):
                return 5

        self.assertEqual(str.rjust("abc", W()), "  abc")

    def test_rjust_with_fillchar_not_char_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, "The fill character must be exactly one character long"
        ):
            str.rjust("", 2, "")
        with self.assertRaisesRegex(
            TypeError, "The fill character must be exactly one character long"
        ):
            str.rjust("", 2, ",,")

    def test_rpartition_with_non_str_self_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError, "requires a 'str'", str.rpartition, None, "hello"
        )

    def test_rpartition_with_non_str_sep_raises_type_error(self):
        self.assertRaises(TypeError, str.rpartition, "hello", None)

    def test_rpartition_partitions_str(self):
        self.assertEqual("hello".rpartition("l"), ("hel", "l", "o"))

    def test_split_with_empty_separator_list_raises_type_error(self):
        self.assertRaisesRegex(TypeError, "must be str or None", str.split, "abc", [])

    def test_split_with_empty_separator_list_and_bad_maxsplit_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "cannot be interpreted as an integer",
            str.split,
            "abc",
            [],
            maxsplit="bad",
        )

    def test_split_with_empty_separator_raises_value_error(self):
        self.assertRaisesRegex(ValueError, "empty separator", str.split, "abc", "")

    def test_split_with_repeated_sep_consolidates_spaces(self):
        self.assertEqual("hello".split("l"), ["he", "", "o"])

    def test_split_with_none_separator_splits_on_whitespace(self):
        self.assertEqual("a\t\n b c".split(), ["a", "b", "c"])

    def test_split_with_adjacent_separators_coalesces_spaces(self):
        self.assertEqual("hello".split("l"), ["he", "", "o"])

    def test_split_with_negative_maxsplit_splits_all(self):
        self.assertEqual("a b c".split(" ", maxsplit=-10), ["a", "b", "c"])

    def test_split_with_non_int_maxsplit_calls_dunder_index(self):
        class C:
            __index__ = Mock(name="__index__", return_value=1)

        result = "a b c".split(" ", maxsplit=C())
        self.assertEqual(result, ["a", "b c"])
        C.__index__.assert_called_once()

    def test_split_with_int_subclass_maxsplit_does_not_call_dunder_index(self):
        class C(int):
            __index__ = Mock(name="__index__", return_value=1)

        result = "a b c".split(" ", maxsplit=C(5))
        self.assertEqual(result, ["a", "b", "c"])
        C.__index__.assert_not_called()

    def test_splitlines_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.splitlines(None)

    def test_splitlines_with_float_keepends_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.splitlines("hello", 0.4)

    def test_splitlines_with_non_int_calls_dunder_int_non_bool(self):
        class C:
            def __int__(self):
                return 100

        self.assertEqual(str.splitlines("\n", C()), ["\n"])

    def test_splitlines_with_non_int_calls_dunder_int_true(self):
        class C:
            def __int__(self):
                return 1

        self.assertEqual(str.splitlines("\n", C()), ["\n"])

    def test_splitlines_with_non_int_calls_dunder_int_false(self):
        class C:
            def __int__(self):
                return 0

        self.assertEqual(str.splitlines("\n", C()), [""])

    def test_startswith_with_non_str_or_tuple_prefix_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            "".startswith([])
        self.assertEqual(
            str(context.exception),
            "startswith first arg must be str or a tuple of str, not list",
        )

    def test_startswith_with_non_str_or_tuple_prefix_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            "hello".startswith(("e", b"", "h"))
        self.assertEqual(
            str(context.exception),
            "tuple for startswith must only contain str, not bytes",
        )

    def test_startswith_with_empty_string_returns_true_for_valid_start(self):
        self.assertTrue("".startswith(""))
        self.assertTrue("".startswith("", -1))
        self.assertTrue("hello".startswith("", 5))

    def test_startswith_with_empty_string_returns_false_for_invalid_start(self):
        self.assertFalse("".startswith("", 1))
        self.assertFalse("hello".startswith("", 6))

    def test_startswith_with_prefix_returns_true(self):
        self.assertTrue("hello".startswith("h"))
        self.assertTrue("hello".startswith("he"))
        self.assertTrue("hello".startswith("hel"))
        self.assertTrue("hello".startswith("hell"))
        self.assertTrue("hello".startswith("hello"))

    def test_startswith_with_nonprefix_returns_false(self):
        self.assertFalse("hello".startswith("e"))
        self.assertFalse("hello".startswith("l"))
        self.assertFalse("hello".startswith("el"))
        self.assertFalse("hello".startswith("llo"))
        self.assertFalse("hello".startswith("hi"))
        self.assertFalse("hello".startswith("hello there"))

    def test_startswith_with_substr_at_start_returns_true(self):
        self.assertTrue("hello".startswith("e", 1))
        self.assertTrue("hello".startswith("ll", 2))
        self.assertTrue("hello".startswith("lo", 3))
        self.assertTrue("hello".startswith("o", 4))

    def test_startswith_outside_bounds_returns_false(self):
        self.assertFalse("hello".startswith("hello", 0, 4))
        self.assertFalse("hello".startswith("llo", 2, 3))
        self.assertFalse("hello".startswith("lo", 3, 4))
        self.assertFalse("hello".startswith("o", 4, 4))

    def test_startswith_with_negative_bounds_relative_to_end(self):
        self.assertTrue("hello".startswith("he", 0, -1))
        self.assertTrue("hello".startswith("llo", -3))
        self.assertFalse("hello".startswith("ello", -4, -2))

    def test_startswith_with_tuple(self):
        self.assertTrue("hello".startswith(("r", "h", "s")))
        self.assertFalse("hello".startswith(("foo", "bar")))

    def test_str_new_with_bytes_and_no_encoding_returns_str(self):
        decoded = str(b"abc")
        self.assertEqual(decoded, "b'abc'")

    def test_str_new_with_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            str("", encoding="utf_8")

    def test_str_new_with_non_bytes_raises_type_error(self):
        with self.assertRaises(TypeError):
            str(1, encoding="utf_8")

    def test_str_new_with_bytes_and_encoding_returns_decoded_str(self):
        decoded = str(b"abc", encoding="ascii")
        self.assertEqual(decoded, "abc")

    def test_str_new_with_no_object_and_encoding_returns_empty_string(self):
        self.assertEqual(str(encoding="ascii"), "")

    def test_str_new_with_class_without_dunder_str_returns_str(self):
        class A:
            def __repr__(self):
                return "test"

        self.assertEqual(str(A()), "test")

    def test_str_new_with_class_with_faulty_dunder_str_raises_type_error(self):
        with self.assertRaises(TypeError):

            class A:
                def __str__(self):
                    return 1

            str(A())

    def test_str_new_with_class_with_proper_duner_str_returns_str(self):
        class A:
            def __str__(self):
                return "test"

        self.assertEqual(str(A()), "test")

    def test_swapcase_with_int_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.swapcase(4)
        self.assertIn(
            "'swapcase' requires a 'str' object but received a 'int'",
            str(context.exception),
        )

    def test_swapcase_returns_inverted_string(self):
        self.assertEqual("hEllO CoMPuTErS".swapcase(), "HeLLo cOmpUteRs")
        self.assertEqual("TTT".swapcase(), "ttt")
        self.assertEqual("ttt".swapcase(), "TTT")
        self.assertEqual("T".swapcase(), "t")
        self.assertEqual(" ".swapcase(), " ")
        self.assertEqual("".swapcase(), "")

    def test_swapcase_with_unicode_returns_inverted_string(self):
        self.assertEqual("\U0001044F".swapcase(), "\U00010427")
        self.assertEqual("\U00010427".swapcase(), "\U0001044F")
        self.assertEqual("\U0001044F\U0001044F".swapcase(), "\U00010427\U00010427")
        self.assertEqual("\U00010427\U0001044F".swapcase(), "\U0001044F\U00010427")
        self.assertEqual("\U0001044F\U00010427".swapcase(), "\U00010427\U0001044F")
        self.assertEqual("X\U00010427x\U0001044F".swapcase(), "x\U0001044FX\U00010427")
        self.assertEqual("ﬁ".swapcase(), "FI")
        self.assertEqual("\u0130".swapcase(), "\u0069\u0307")
        self.assertEqual("\u00e9TuDe".swapcase(), "\u00c9tUdE")

    def test_title_with_returns_titlecased_string(self):
        self.assertEqual("".title(), "")
        self.assertEqual("1234!@#$".title(), "1234!@#$")
        self.assertEqual("hello world".title(), "Hello World")
        self.assertEqual("HELLO_WORLD".title(), "Hello_World")

        self.assertEqual("resum\u00e9".title(), "Resum\u00e9")
        self.assertEqual("\u00e9TuDe".title(), "\u00c9tude")
        self.assertEqual("\u01c7uDeViT".title(), "\u01c8udevit")

    def test_upper_returns_uppercased_string(self):
        self.assertEqual(str.upper("hello"), "HELLO")
        self.assertEqual(str.upper("HeLLo_WoRlD"), "HELLO_WORLD")
        self.assertEqual(str.upper("hellO World!"), "HELLO WORLD!")
        self.assertEqual(str.upper(""), "")
        self.assertEqual(str.upper("1234"), "1234")
        self.assertEqual(str.upper("$%^*("), "$%^*(")

    def test_upper_with_str_subclass_returns_str(self):
        class C(str):
            pass

        self.assertEqual(str.upper(C("HeLLo")), "HELLO")
        self.assertIs(type(str.upper(C("HeLLo"))), str)
        self.assertIs(type(str.upper(C(""))), str)
        self.assertIs(type(str.upper(C("upper"))), str)

    def test_upper_with_non_ascii_returns_uppercased_string(self):
        self.assertEqual(str.upper("foo\u01BDBAR"), "FOO\u01BCBAR")
        # lowercase without uppercase
        self.assertEqual(str.upper("FOO\u0237bar"), "FOO\u0237BAR")
        # one to many uppercasing
        self.assertEqual(str.upper("foo\xDFbar"), "FOO\x53\x53BAR")


class StrDunderFormatTests(unittest.TestCase):
    def test_empty_format_returns_string(self):
        self.assertEqual(str.__format__("hello", ""), "hello")
        self.assertEqual(str.__format__("", ""), "")
        self.assertEqual(str.__format__("\U0001f40d", ""), "\U0001f40d")

    def test_format_with_width_returns_string(self):
        self.assertEqual(str.__format__("hello", "1"), "hello")
        self.assertEqual(str.__format__("hello", "5"), "hello")
        self.assertEqual(str.__format__("hello", "8"), "hello   ")

        self.assertEqual(str.__format__("hello", "<"), "hello")
        self.assertEqual(str.__format__("hello", "<0"), "hello")
        self.assertEqual(str.__format__("hello", "<5"), "hello")
        self.assertEqual(str.__format__("hello", "<8"), "hello   ")

        self.assertEqual(str.__format__("hello", ">"), "hello")
        self.assertEqual(str.__format__("hello", ">0"), "hello")
        self.assertEqual(str.__format__("hello", ">5"), "hello")
        self.assertEqual(str.__format__("hello", ">8"), "   hello")

        self.assertEqual(str.__format__("hello", "^"), "hello")
        self.assertEqual(str.__format__("hello", "^0"), "hello")
        self.assertEqual(str.__format__("hello", "^5"), "hello")
        self.assertEqual(str.__format__("hello", "^8"), " hello  ")
        self.assertEqual(str.__format__("hello", "^9"), "  hello  ")

    def test_format_with_fill_char_returns_string(self):
        self.assertEqual(str.__format__("hello", ".^9"), "..hello..")
        self.assertEqual(str.__format__("hello", "*>9"), "****hello")
        self.assertEqual(str.__format__("hello", "^<9"), "hello^^^^")
        self.assertEqual(
            str.__format__("hello", "\U0001f40d>7"), "\U0001f40d\U0001f40dhello"
        )

    def test_format_with_precision_returns_string(self):
        self.assertEqual(str.__format__("hello", ".0"), "")
        self.assertEqual(str.__format__("hello", ".2"), "he")
        self.assertEqual(str.__format__("hello", ".5"), "hello")
        self.assertEqual(str.__format__("hello", ".7"), "hello")

    def test_format_with_precision_and_width_returns_string(self):
        self.assertEqual(str.__format__("hello", "10.8"), "hello     ")
        self.assertEqual(str.__format__("hello", "^5.2"), " he  ")

    def test_works_with_str_subclass(self):
        class C(str):
            pass

        self.assertEqual(str.__format__("hello", C("7")), "hello  ")
        self.assertEqual(str.__format__(C("hello"), "7"), "hello  ")
        self.assertIs(type(str.__format__(C("hello"), "7")), str)
        self.assertEqual(str.__format__(C("hello"), ""), "hello")
        self.assertIs(type(str.__format__(C("hello"), "")), str)

    def test_format_zero_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", "0")
        self.assertEqual(
            str(context.exception),
            "'=' alignment not allowed in string format specifier",
        )

    def test_format_with_equal_alignment_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", "=")
        self.assertEqual(
            str(context.exception),
            "'=' alignment not allowed in string format specifier",
        )

    def test_non_s_format_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", "d")
        self.assertEqual(
            str(context.exception), "Unknown format code 'd' for object of type 'str'"
        )

        class C(str):
            pass

        with self.assertRaises(ValueError) as context:
            str.__format__(C(""), "\U0001f40d")
        self.assertEqual(
            str(context.exception),
            "Unknown format code '\\x1f40d' for object of type 'C'",
        )

    def test_big_width_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", "999999999999999999999")
        self.assertEqual(
            str(context.exception), "Too many decimal digits in format string"
        )

    def test_missing_precision_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", ".")
        self.assertEqual(str(context.exception), "Format specifier missing precision")

    def test_big_precision_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", ".999999999999999999999")
        self.assertEqual(
            str(context.exception), "Too many decimal digits in format string"
        )

    def test_extra_chars_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", "sX")
        self.assertEqual(str(context.exception), "Invalid format specifier")

    def test_comma_underscore_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", ",_")
        self.assertEqual(str(context.exception), "Cannot specify both ',' and '_'.")

    def test_comma_comma_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", ",,")
        self.assertEqual(str(context.exception), "Cannot specify both ',' and '_'.")

    def test_underscore_underscore_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", "__")
        self.assertEqual(str(context.exception), "Cannot specify '_' with '_'.")

    def test_unexpected_thousands_separator_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", "_")
        self.assertEqual(str(context.exception), "Cannot specify '_' with 's'.")

    def test_zfill_with_empty_string_input_returns_padding_only(self):
        self.assertEqual("000", "".zfill(3))

    def test_zfill_with_positive_prefix_input_returns_prepended_padding(self):
        self.assertEqual("+0123", "+123".zfill(5))
        self.assertEqual("+00", "+".zfill(3))

    def test_zfill_with_negative_prefix_input_returns_prepended_padding(self):
        self.assertEqual("-0123", "-123".zfill(5))
        self.assertEqual("-00", "-".zfill(3))

    def test_zfill_without_sign_prefix_input_returns_prepended_padding(self):
        self.assertEqual("-0123", "-123".zfill(5))
        self.assertEqual("0123", "123".zfill(4))
        self.assertEqual("0034", "34".zfill(4))

    def test_zfill_without_sign_prefix_input_returns_self_when_width_lte_len(self):
        test_str = "123"
        for width in range(len(test_str) + 1):
            result = test_str.zfill(width)
            self.assertIs(test_str, result)

    def test_zfill_with_positive_prefix_input_returns_self_when_width_lte_len(self):
        test_str = "+123"
        for width in range(len(test_str) + 1):
            result = test_str.zfill(width)
            self.assertIs(test_str, result)

    def test_zfill_with_negative_prefix_input_returns_self_when_width_lte_len(self):
        test_str = "-123"
        for width in range(len(test_str) + 1):
            result = test_str.zfill(width)
            self.assertIs(test_str, result)

    def test_zfill_with_float_width_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            "123".zfill(3.2)
        self.assertEqual(str(context.exception), "integer argument expected, got float")


class StrModTests(unittest.TestCase):
    def test_empty_format_returns_empty_string(self):
        self.assertEqual(str.__mod__("", ()), "")

    def test_simple_string_returns_string(self):
        self.assertEqual(str.__mod__("foo bar (}", ()), "foo bar (}")

    def test_with_non_tuple_args_returns_string(self):
        self.assertEqual(str.__mod__("%s", "foo"), "foo")
        self.assertEqual(str.__mod__("%d", 42), "42")
        self.assertEqual(str.__mod__("%s", {"foo": "bar"}), "{'foo': 'bar'}")

    def test_with_named_args_returns_string(self):
        self.assertEqual(
            str.__mod__("%(foo)s %(bar)d", {"foo": "ho", "bar": 42}), "ho 42"
        )
        self.assertEqual(str.__mod__("%()x", {"": 123}), "7b")
        self.assertEqual(str.__mod__(")%(((()) ()))s(", {"((()) ())": 99}), ")99(")
        self.assertEqual(str.__mod__("%(%s)s", {"%s": -5}), "-5")

    def test_with_custom_mapping_returns_string(self):
        class C:
            def __getitem__(self, key):
                return "getitem called with " + key

        self.assertEqual(str.__mod__("%(foo)s", C()), "getitem called with foo")

    def test_with_custom_mapping_propagates_errors(self):
        with self.assertRaises(KeyError) as context:
            str.__mod__("%(foo)s", {})
        self.assertEqual(str(context.exception), "'foo'")

        class C:
            def __getitem__(self, key):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            str.__mod__("%(foo)s", C())

    def test_without_mapping_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%(foo)s", None)
        self.assertEqual(str(context.exception), "format requires a mapping")
        with self.assertRaises(TypeError) as context:
            str.__mod__("%(foo)s", "foobar")
        self.assertEqual(str(context.exception), "format requires a mapping")
        with self.assertRaises(TypeError) as context:
            str.__mod__("%(foo)s", ("foobar",))
        self.assertEqual(str(context.exception), "format requires a mapping")

    def test_with_mapping_does_not_raise_type_error(self):
        # The following must not raise
        # "not all arguments converted during string formatting".
        self.assertEqual(str.__mod__("foo", {"bar": 42}), "foo")

    def test_positional_after_named_arg_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%(foo)s %s", {"foo": "bar"})
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )

    def test_mix_named_and_tuple_args_returns_string(self):
        self.assertEqual(str.__mod__("%s %(a)s", {"a": 77}), "{'a': 77} 77")

    def test_mapping_in_tuple_returns_string(self):
        self.assertEqual(str.__mod__("%s", ({"foo": "bar"},)), "{'foo': 'bar'}")

    def test_c_format_returns_string(self):
        self.assertEqual(str.__mod__("%c", ("x",)), "x")
        self.assertEqual(str.__mod__("%c", ("\U0001f44d",)), "\U0001f44d")
        self.assertEqual(str.__mod__("%c", (76,)), "L")
        self.assertEqual(str.__mod__("%c", (0x1F40D,)), "\U0001f40d")

    def test_c_format_with_non_int_returns_string(self):
        class C:
            def __index__(self):
                return 42

        self.assertEqual(str.__mod__("%c", (C(),)), "*")

    def test_c_format_raises_overflow_error(self):
        import sys

        maxunicode_range = str.__mod__("range(0x%x)", (sys.maxunicode + 1))
        with self.assertRaises(OverflowError) as context:
            str.__mod__("%c", (sys.maxunicode + 1,))
        self.assertEqual(str(context.exception), f"%c arg not in {maxunicode_range}")
        with self.assertRaises(OverflowError) as context:
            str.__mod__("%c", (-1,))
        self.assertEqual(str(context.exception), f"%c arg not in {maxunicode_range}")

    def test_c_format_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%c", (None,))
        self.assertEqual(str(context.exception), "%c requires int or char")
        with self.assertRaises(TypeError) as context:
            str.__mod__("%c", ("ab",))
        self.assertEqual(str(context.exception), "%c requires int or char")
        with self.assertRaises(TypeError) as context:
            str.__mod__("%c", (123456789012345678901234567890,))
        self.assertEqual(str(context.exception), "%c requires int or char")

        class C:
            def __index__(self):
                raise UserWarning()

        with self.assertRaises(TypeError) as context:
            str.__mod__("%c", (C(),))
        self.assertEqual(str(context.exception), "%c requires int or char")

    def test_s_format_returns_string(self):
        self.assertEqual(str.__mod__("%s", ("foo",)), "foo")

        class C:
            __repr__ = None

            def __str__(self):
                return "str called"

        self.assertEqual(str.__mod__("%s", (C(),)), "str called")

    def test_s_format_propagates_errors(self):
        class C:
            def __str__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            str.__mod__("%s", (C(),))

    def test_r_format_returns_string(self):
        self.assertEqual(str.__mod__("%r", (42,)), "42")
        self.assertEqual(str.__mod__("%r", ("foo",)), "'foo'")
        self.assertEqual(
            str.__mod__("%r", ({"foo": "\U0001d4eb\U0001d4ea\U0001d4fb"},)),
            "{'foo': '\U0001d4eb\U0001d4ea\U0001d4fb'}",
        )

        class C:
            def __repr__(self):
                return "repr called"

            __str__ = None

        self.assertEqual(str.__mod__("%r", (C(),)), "repr called")

    def test_r_format_propagates_errors(self):
        class C:
            def __repr__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            str.__mod__("%r", (C(),))

    def test_a_format_returns_string(self):
        self.assertEqual(str.__mod__("%a", (42,)), "42")
        self.assertEqual(str.__mod__("%a", ("foo",)), "'foo'")

        class C:
            def __repr__(self):
                return "repr called"

            __str__ = None

        self.assertEqual(str.__mod__("%a", (C(),)), "repr called")
        # TODO(T39861344, T38702699): We should have a test with some non-ascii
        # characters here proving that they are escaped. Unfortunately
        # builtins.ascii() does not work in that case yet.

    def test_a_format_propagates_errors(self):
        class C:
            def __repr__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            str.__mod__("%a", (C(),))

    def test_diu_format_returns_string(self):
        self.assertEqual(str.__mod__("%d", (0,)), "0")
        self.assertEqual(str.__mod__("%d", (-1,)), "-1")
        self.assertEqual(str.__mod__("%d", (42,)), "42")
        self.assertEqual(str.__mod__("%i", (0,)), "0")
        self.assertEqual(str.__mod__("%i", (-1,)), "-1")
        self.assertEqual(str.__mod__("%i", (42,)), "42")
        self.assertEqual(str.__mod__("%u", (0,)), "0")
        self.assertEqual(str.__mod__("%u", (-1,)), "-1")
        self.assertEqual(str.__mod__("%u", (42,)), "42")

    def test_diu_format_with_largeint_returns_string(self):
        self.assertEqual(
            str.__mod__("%d", (-123456789012345678901234567890,)),
            "-123456789012345678901234567890",
        )
        self.assertEqual(
            str.__mod__("%i", (-123456789012345678901234567890,)),
            "-123456789012345678901234567890",
        )
        self.assertEqual(
            str.__mod__("%u", (-123456789012345678901234567890,)),
            "-123456789012345678901234567890",
        )

    def test_diu_format_with_non_int_returns_string(self):
        class C:
            def __int__(self):
                return 42

            def __index__(self):
                raise UserWarning()

        self.assertEqual(str.__mod__("%d", (C(),)), "42")
        self.assertEqual(str.__mod__("%i", (C(),)), "42")
        self.assertEqual(str.__mod__("%u", (C(),)), "42")

    def test_diu_format_raises_typeerrors(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%d", (None,))
        self.assertEqual(
            str(context.exception), "%d format: a number is required, not NoneType"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%i", (None,))
        self.assertEqual(
            str(context.exception), "%i format: a number is required, not NoneType"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%u", (None,))
        self.assertEqual(
            str(context.exception), "%u format: a number is required, not NoneType"
        )

    def test_diu_format_propagates_errors(self):
        class C:
            def __int__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            str.__mod__("%d", (C(),))
        with self.assertRaises(UserWarning):
            str.__mod__("%i", (C(),))
        with self.assertRaises(UserWarning):
            str.__mod__("%u", (C(),))

    def test_diu_format_reraises_typerrors(self):
        class C:
            def __int__(self):
                raise TypeError("foobar")

        with self.assertRaises(TypeError) as context:
            str.__mod__("%d", (C(),))
        self.assertEqual(
            str(context.exception), "%d format: a number is required, not C"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%i", (C(),))
        self.assertEqual(
            str(context.exception), "%i format: a number is required, not C"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%u", (C(),))
        self.assertEqual(
            str(context.exception), "%u format: a number is required, not C"
        )

    def test_xX_format_returns_string(self):
        self.assertEqual(str.__mod__("%x", (0,)), "0")
        self.assertEqual(str.__mod__("%x", (-123,)), "-7b")
        self.assertEqual(str.__mod__("%x", (42,)), "2a")
        self.assertEqual(str.__mod__("%X", (0,)), "0")
        self.assertEqual(str.__mod__("%X", (-123,)), "-7B")
        self.assertEqual(str.__mod__("%X", (42,)), "2A")

    def test_xX_format_with_largeint_returns_string(self):
        self.assertEqual(
            str.__mod__("%x", (-123456789012345678901234567890,)),
            "-18ee90ff6c373e0ee4e3f0ad2",
        )
        self.assertEqual(
            str.__mod__("%X", (-123456789012345678901234567890,)),
            "-18EE90FF6C373E0EE4E3F0AD2",
        )

    def test_xX_format_with_non_int_returns_string(self):
        class C:
            def __float__(self):
                return 3.3

            def __index__(self):
                return 77

        self.assertEqual(str.__mod__("%x", (C(),)), "4d")
        self.assertEqual(str.__mod__("%X", (C(),)), "4D")

    def test_xX_format_raises_typeerrors(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%x", (None,))
        self.assertEqual(
            str(context.exception), "%x format: an integer is required, not NoneType"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%X", (None,))
        self.assertEqual(
            str(context.exception), "%X format: an integer is required, not NoneType"
        )

    def test_xX_format_propagates_errors(self):
        class C:
            def __int__(self):
                return 42

            def __index__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            str.__mod__("%x", (C(),))
        with self.assertRaises(UserWarning):
            str.__mod__("%X", (C(),))

    def test_xX_format_reraises_typerrors(self):
        class C:
            def __int__(self):
                return 42

            def __index__(self):
                raise TypeError("foobar")

        with self.assertRaises(TypeError) as context:
            str.__mod__("%x", (C(),))
        self.assertEqual(
            str(context.exception), "%x format: an integer is required, not C"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%X", (C(),))
        self.assertEqual(
            str(context.exception), "%X format: an integer is required, not C"
        )

    def test_o_format_returns_string(self):
        self.assertEqual(str.__mod__("%o", (0,)), "0")
        self.assertEqual(str.__mod__("%o", (-123,)), "-173")
        self.assertEqual(str.__mod__("%o", (42,)), "52")

    def test_o_format_with_largeint_returns_string(self):
        self.assertEqual(
            str.__mod__("%o", (-123456789012345678901234567890)),
            "-143564417755415637016711617605322",
        )

    def test_o_format_with_non_int_returns_string(self):
        class C:
            def __float__(self):
                return 3.3

            def __index__(self):
                return 77

        self.assertEqual(str.__mod__("%o", (C(),)), "115")

    def test_o_format_raises_typeerrors(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%o", (None,))
        self.assertEqual(
            str(context.exception), "%o format: an integer is required, not NoneType"
        )

    def test_o_format_propagates_errors(self):
        class C:
            def __int__(self):
                return 42

            def __index__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            str.__mod__("%o", (C(),))

    def test_o_format_reraises_typerrors(self):
        class C:
            def __int__(self):
                return 42

            def __index__(self):
                raise TypeError("foobar")

        with self.assertRaises(TypeError) as context:
            str.__mod__("%o", (C(),))
        self.assertEqual(
            str(context.exception), "%o format: an integer is required, not C"
        )

    def test_f_format_returns_string(self):
        self.assertEqual(str.__mod__("%f", (0.0,)), "0.000000")
        self.assertEqual(str.__mod__("%f", (-0.0,)), "-0.000000")
        self.assertEqual(str.__mod__("%f", (1.0,)), "1.000000")
        self.assertEqual(str.__mod__("%f", (-1.0,)), "-1.000000")
        self.assertEqual(str.__mod__("%f", (42.125,)), "42.125000")

        self.assertEqual(str.__mod__("%f", (1e3,)), "1000.000000")
        self.assertEqual(str.__mod__("%f", (1e6,)), "1000000.000000")
        self.assertEqual(
            str.__mod__("%f", (1e40,)),
            "10000000000000000303786028427003666890752.000000",
        )

    def test_F_format_returns_string(self):
        self.assertEqual(str.__mod__("%F", (42.125,)), "42.125000")

    def test_e_format_returns_string(self):
        self.assertEqual(str.__mod__("%e", (0.0,)), "0.000000e+00")
        self.assertEqual(str.__mod__("%e", (-0.0,)), "-0.000000e+00")
        self.assertEqual(str.__mod__("%e", (1.0,)), "1.000000e+00")
        self.assertEqual(str.__mod__("%e", (-1.0,)), "-1.000000e+00")
        self.assertEqual(str.__mod__("%e", (42.125,)), "4.212500e+01")

        self.assertEqual(str.__mod__("%e", (1e3,)), "1.000000e+03")
        self.assertEqual(str.__mod__("%e", (1e6,)), "1.000000e+06")
        self.assertEqual(str.__mod__("%e", (1e40,)), "1.000000e+40")

    def test_E_format_returns_string(self):
        self.assertEqual(str.__mod__("%E", (1.0,)), "1.000000E+00")

    def test_g_format_returns_string(self):
        self.assertEqual(str.__mod__("%g", (0.0,)), "0")
        self.assertEqual(str.__mod__("%g", (-1.0,)), "-1")
        self.assertEqual(str.__mod__("%g", (0.125,)), "0.125")
        self.assertEqual(str.__mod__("%g", (3.5,)), "3.5")

    def test_eEfFgG_format_with_inf_returns_string(self):
        self.assertEqual(str.__mod__("%e", (float("inf"),)), "inf")
        self.assertEqual(str.__mod__("%E", (float("inf"),)), "INF")
        self.assertEqual(str.__mod__("%f", (float("inf"),)), "inf")
        self.assertEqual(str.__mod__("%F", (float("inf"),)), "INF")
        self.assertEqual(str.__mod__("%g", (float("inf"),)), "inf")
        self.assertEqual(str.__mod__("%G", (float("inf"),)), "INF")

        self.assertEqual(str.__mod__("%e", (-float("inf"),)), "-inf")
        self.assertEqual(str.__mod__("%E", (-float("inf"),)), "-INF")
        self.assertEqual(str.__mod__("%f", (-float("inf"),)), "-inf")
        self.assertEqual(str.__mod__("%F", (-float("inf"),)), "-INF")
        self.assertEqual(str.__mod__("%g", (-float("inf"),)), "-inf")
        self.assertEqual(str.__mod__("%G", (-float("inf"),)), "-INF")

    def test_eEfFgG_format_with_nan_returns_string(self):
        self.assertEqual(str.__mod__("%e", (float("nan"),)), "nan")
        self.assertEqual(str.__mod__("%E", (float("nan"),)), "NAN")
        self.assertEqual(str.__mod__("%f", (float("nan"),)), "nan")
        self.assertEqual(str.__mod__("%F", (float("nan"),)), "NAN")
        self.assertEqual(str.__mod__("%g", (float("nan"),)), "nan")
        self.assertEqual(str.__mod__("%G", (float("nan"),)), "NAN")

        self.assertEqual(str.__mod__("%e", (float("-nan"),)), "nan")
        self.assertEqual(str.__mod__("%E", (float("-nan"),)), "NAN")
        self.assertEqual(str.__mod__("%f", (float("-nan"),)), "nan")
        self.assertEqual(str.__mod__("%F", (float("-nan"),)), "NAN")
        self.assertEqual(str.__mod__("%g", (float("-nan"),)), "nan")
        self.assertEqual(str.__mod__("%G", (float("-nan"),)), "NAN")

    def test_f_format_with_precision_returns_string(self):
        number = 1.23456789123456789
        self.assertEqual(str.__mod__("%.0f", number), "1")
        self.assertEqual(str.__mod__("%.1f", number), "1.2")
        self.assertEqual(str.__mod__("%.2f", number), "1.23")
        self.assertEqual(str.__mod__("%.3f", number), "1.235")
        self.assertEqual(str.__mod__("%.4f", number), "1.2346")
        self.assertEqual(str.__mod__("%.5f", number), "1.23457")
        self.assertEqual(str.__mod__("%.6f", number), "1.234568")
        self.assertEqual(str.__mod__("%f", number), "1.234568")

        self.assertEqual(str.__mod__("%.17f", number), "1.23456789123456789")
        self.assertEqual(str.__mod__("%.25f", number), "1.2345678912345678934769921")
        self.assertEqual(
            str.__mod__("%.60f", number),
            "1.234567891234567893476992139767389744520187377929687500000000",
        )

    def test_eEfFgG_format_with_precision_returns_string(self):
        number = 1.23456789123456789
        self.assertEqual(str.__mod__("%.0e", number), "1e+00")
        self.assertEqual(str.__mod__("%.0E", number), "1E+00")
        self.assertEqual(str.__mod__("%.0f", number), "1")
        self.assertEqual(str.__mod__("%.0F", number), "1")
        self.assertEqual(str.__mod__("%.0g", number), "1")
        self.assertEqual(str.__mod__("%.0G", number), "1")
        self.assertEqual(str.__mod__("%.4e", number), "1.2346e+00")
        self.assertEqual(str.__mod__("%.4E", number), "1.2346E+00")
        self.assertEqual(str.__mod__("%.4f", number), "1.2346")
        self.assertEqual(str.__mod__("%.4F", number), "1.2346")
        self.assertEqual(str.__mod__("%.4g", number), "1.235")
        self.assertEqual(str.__mod__("%.4G", number), "1.235")
        self.assertEqual(str.__mod__("%e", number), "1.234568e+00")
        self.assertEqual(str.__mod__("%E", number), "1.234568E+00")
        self.assertEqual(str.__mod__("%f", number), "1.234568")
        self.assertEqual(str.__mod__("%F", number), "1.234568")
        self.assertEqual(str.__mod__("%g", number), "1.23457")
        self.assertEqual(str.__mod__("%G", number), "1.23457")

    def test_g_format_with_flags_and_width_returns_string(self):
        self.assertEqual(str.__mod__("%5g", 7.0), "    7")
        self.assertEqual(str.__mod__("%5g", 7.2), "  7.2")
        self.assertEqual(str.__mod__("% 5g", 7.2), "  7.2")
        self.assertEqual(str.__mod__("%+5g", 7.2), " +7.2")
        self.assertEqual(str.__mod__("%5g", -7.2), " -7.2")
        self.assertEqual(str.__mod__("% 5g", -7.2), " -7.2")
        self.assertEqual(str.__mod__("%+5g", -7.2), " -7.2")

        self.assertEqual(str.__mod__("%-5g", 7.0), "7    ")
        self.assertEqual(str.__mod__("%-5g", 7.2), "7.2  ")
        self.assertEqual(str.__mod__("%- 5g", 7.2), " 7.2 ")
        self.assertEqual(str.__mod__("%-+5g", 7.2), "+7.2 ")
        self.assertEqual(str.__mod__("%-5g", -7.2), "-7.2 ")
        self.assertEqual(str.__mod__("%- 5g", -7.2), "-7.2 ")
        self.assertEqual(str.__mod__("%-+5g", -7.2), "-7.2 ")

        self.assertEqual(str.__mod__("%#g", 7.0), "7.00000")

        self.assertEqual(str.__mod__("%#- 7.2g", float("-nan")), " nan   ")
        self.assertEqual(str.__mod__("%#- 7.2g", float("inf")), " inf   ")
        self.assertEqual(str.__mod__("%#- 7.2g", float("-inf")), "-inf   ")

    def test_eEfFgG_format_with_flags_and_width_returns_string(self):
        number = 1.23456789123456789
        self.assertEqual(str.__mod__("% -#12.3e", number), " 1.235e+00  ")
        self.assertEqual(str.__mod__("% -#12.3E", number), " 1.235E+00  ")
        self.assertEqual(str.__mod__("% -#12.3f", number), " 1.235      ")
        self.assertEqual(str.__mod__("% -#12.3F", number), " 1.235      ")
        self.assertEqual(str.__mod__("% -#12.3g", number), " 1.23       ")
        self.assertEqual(str.__mod__("% -#12.3G", number), " 1.23       ")

    def test_ef_format_with_non_float_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%e", (None,))
        self.assertEqual(str(context.exception), "must be real number, not NoneType")

        class C:
            def __float__(self):
                return "not a float"

        with self.assertRaises(TypeError) as context:
            str.__mod__("%f", (C(),))
        self.assertEqual(
            str(context.exception), "C.__float__ returned non-float (type str)"
        )

    def test_g_format_propogates_errors(self):
        class C:
            def __float__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            str.__mod__("%g", (C(),))

    def test_efg_format_with_non_float_returns_string(self):
        class A(float):
            pass

        self.assertEqual(str.__mod__("%e", (A(9.625),)), str.__mod__("%e", (9.625,)))

        class C:
            def __float__(self):
                return 3.5

        self.assertEqual(str.__mod__("%f", (C(),)), str.__mod__("%f", (3.5,)))

        class D:
            def __float__(self):
                return A(-12.75)

        warnings.filterwarnings(
            action="ignore",
            category=DeprecationWarning,
            message=".*__float__ returned non-float.*",
            module=__name__,
        )
        self.assertEqual(str.__mod__("%g", (D(),)), str.__mod__("%g", (-12.75,)))

    def test_percent_format_returns_percent(self):
        self.assertEqual(str.__mod__("%%", ()), "%")

    # TODO(T65863013): Make str.__mod__ behavior match CPython and remove this
    # skipIf
    @unittest.skipIf(
        sys.implementation.name == "cpython" and sys.version_info >= (3, 7),
        "behavior changes in CPython 3.7",
    )
    def test_percent_with_flags_percision_and_width_returns_percent(self):
        self.assertEqual(str.__mod__("%0.0%", ()), "%")
        self.assertEqual(str.__mod__("%*.%", (42,)), "%")
        self.assertEqual(str.__mod__("%.*%", (88,)), "%")
        self.assertEqual(str.__mod__("%0#*.42%", (1234,)), "%")

    def test_flags_get_accepted(self):
        self.assertEqual(str.__mod__("%-s", ""), "")
        self.assertEqual(str.__mod__("%+s", ""), "")
        self.assertEqual(str.__mod__("% s", ""), "")
        self.assertEqual(str.__mod__("%#s", ""), "")
        self.assertEqual(str.__mod__("%0s", ""), "")
        self.assertEqual(str.__mod__("%#-#0+ -s", ""), "")

    def test_string_format_with_width_returns_string(self):
        self.assertEqual(str.__mod__("%5s", "oh"), "   oh")
        self.assertEqual(str.__mod__("%-5s", "ah"), "ah   ")
        self.assertEqual(str.__mod__("%05s", "uh"), "   uh")
        self.assertEqual(str.__mod__("%-# 5s", "eh"), "eh   ")

        self.assertEqual(str.__mod__("%0s", "foo"), "foo")
        self.assertEqual(str.__mod__("%-0s", "foo"), "foo")
        self.assertEqual(str.__mod__("%10s", "hello world"), "hello world")
        self.assertEqual(str.__mod__("%-10s", "hello world"), "hello world")

    def test_string_format_with_width_star_returns_string(self):
        self.assertEqual(str.__mod__("%*s", (7, "foo")), "    foo")
        self.assertEqual(str.__mod__("%*s", (-7, "bar")), "bar    ")
        self.assertEqual(str.__mod__("%-*s", (7, "baz")), "baz    ")
        self.assertEqual(str.__mod__("%-*s", (-7, "bam")), "bam    ")

    def test_string_format_with_precision_returns_string(self):
        self.assertEqual(str.__mod__("%.3s", "python"), "pyt")
        self.assertEqual(str.__mod__("%.0s", "python"), "")
        self.assertEqual(str.__mod__("%.10s", "python"), "python")

    def test_string_format_with_precision_star_returns_string(self):
        self.assertEqual(str.__mod__("%.*s", (3, "monty")), "mon")
        self.assertEqual(str.__mod__("%.*s", (0, "monty")), "")
        self.assertEqual(str.__mod__("%.*s", (-4, "monty")), "")

    def test_string_format_with_width_and_precision_returns_string(self):
        self.assertEqual(str.__mod__("%8.3s", ("foobar",)), "     foo")
        self.assertEqual(str.__mod__("%-8.3s", ("foobar",)), "foo     ")
        self.assertEqual(str.__mod__("%*.3s", (8, "foobar")), "     foo")
        self.assertEqual(str.__mod__("%*.3s", (-8, "foobar")), "foo     ")
        self.assertEqual(str.__mod__("%8.*s", (3, "foobar")), "     foo")
        self.assertEqual(str.__mod__("%-8.*s", (3, "foobar")), "foo     ")
        self.assertEqual(str.__mod__("%*.*s", (8, 3, "foobar")), "     foo")
        self.assertEqual(str.__mod__("%-*.*s", (8, 3, "foobar")), "foo     ")

    def test_s_r_a_c_formats_accept_flags_width_precision_return_strings(self):
        self.assertEqual(str.__mod__("%-*.3s", (8, "foobar")), "foo     ")
        self.assertEqual(str.__mod__("%-*.3r", (8, "foobar")), "'fo     ")
        self.assertEqual(str.__mod__("%-*.3a", (8, "foobar")), "'fo     ")
        self.assertEqual(str.__mod__("%-*.3c", (8, 94)), "^       ")

    def test_number_format_with_sign_flag_returns_string(self):
        self.assertEqual(str.__mod__("%+d", (42,)), "+42")
        self.assertEqual(str.__mod__("%+d", (-42,)), "-42")
        self.assertEqual(str.__mod__("% d", (17,)), " 17")
        self.assertEqual(str.__mod__("% d", (-17,)), "-17")
        self.assertEqual(str.__mod__("%+ d", (42,)), "+42")
        self.assertEqual(str.__mod__("%+ d", (-42,)), "-42")
        self.assertEqual(str.__mod__("% +d", (17,)), "+17")
        self.assertEqual(str.__mod__("% +d", (-17,)), "-17")

    def test_number_format_alt_flag_returns_string(self):
        self.assertEqual(str.__mod__("%#d", (23,)), "23")
        self.assertEqual(str.__mod__("%#x", (23,)), "0x17")
        self.assertEqual(str.__mod__("%#X", (23,)), "0X17")
        self.assertEqual(str.__mod__("%#o", (23,)), "0o27")

    def test_number_format_with_width_returns_string(self):
        self.assertEqual(str.__mod__("%5d", (123,)), "  123")
        self.assertEqual(str.__mod__("%5d", (-8,)), "   -8")
        self.assertEqual(str.__mod__("%-5d", (123,)), "123  ")
        self.assertEqual(str.__mod__("%-5d", (-8,)), "-8   ")

        self.assertEqual(str.__mod__("%05d", (123,)), "00123")
        self.assertEqual(str.__mod__("%05d", (-8,)), "-0008")
        self.assertEqual(str.__mod__("%-05d", (123,)), "123  ")
        self.assertEqual(str.__mod__("%0-5d", (-8,)), "-8   ")

        self.assertEqual(str.__mod__("%#7x", (42,)), "   0x2a")
        self.assertEqual(str.__mod__("%#7x", (-42,)), "  -0x2a")

        self.assertEqual(str.__mod__("%5d", (123456,)), "123456")
        self.assertEqual(str.__mod__("%-5d", (-123456,)), "-123456")

    def test_number_format_with_precision_returns_string(self):
        self.assertEqual(str.__mod__("%.5d", (123,)), "00123")
        self.assertEqual(str.__mod__("%.5d", (-123,)), "-00123")
        self.assertEqual(str.__mod__("%.5d", (1234567,)), "1234567")
        self.assertEqual(str.__mod__("%#.5x", (99,)), "0x00063")

    def test_number_format_with_width_precision_flags_returns_string(self):
        self.assertEqual(str.__mod__("%8.3d", (12,)), "     012")
        self.assertEqual(str.__mod__("%8.3d", (-7,)), "    -007")
        self.assertEqual(str.__mod__("%05.3d", (12,)), "00012")
        self.assertEqual(str.__mod__("%+05.3d", (12,)), "+0012")
        self.assertEqual(str.__mod__("% 05.3d", (12,)), " 0012")
        self.assertEqual(str.__mod__("% 05.3x", (19,)), " 0013")

        self.assertEqual(str.__mod__("%-8.3d", (12,)), "012     ")
        self.assertEqual(str.__mod__("%-8.3d", (-7,)), "-007    ")
        self.assertEqual(str.__mod__("%- 8.3d", (66,)), " 066    ")

    def test_width_and_precision_star_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%*d", (42,))
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%.*d", (42,))
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%*.*d", (42,))
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%*.*d", (1, 2))
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )

    def test_negative_precision_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__mod__("%.-2s", "foo")
        self.assertEqual(
            str(context.exception), "unsupported format character '-' (0x2d) at index 2"
        )

    def test_two_specifiers_returns_string(self):
        self.assertEqual(str.__mod__("%s%s", ("foo", "bar")), "foobar")
        self.assertEqual(str.__mod__(",%s%s", ("foo", "bar")), ",foobar")
        self.assertEqual(str.__mod__("%s,%s", ("foo", "bar")), "foo,bar")
        self.assertEqual(str.__mod__("%s%s,", ("foo", "bar")), "foobar,")
        self.assertEqual(str.__mod__(",%s..%s---", ("foo", "bar")), ",foo..bar---")
        self.assertEqual(str.__mod__(",%s...%s--", ("foo", "bar")), ",foo...bar--")
        self.assertEqual(str.__mod__(",,%s.%s---", ("foo", "bar")), ",,foo.bar---")
        self.assertEqual(str.__mod__(",,%s...%s-", ("foo", "bar")), ",,foo...bar-")
        self.assertEqual(str.__mod__(",,,%s..%s-", ("foo", "bar")), ",,,foo..bar-")
        self.assertEqual(str.__mod__(",,,%s.%s--", ("foo", "bar")), ",,,foo.bar--")

    def test_mixed_specifiers_with_percents_returns_string(self):
        self.assertEqual(str.__mod__("%%%s%%%s%%", ("foo", "bar")), "%foo%bar%")

    def test_mixed_specifiers_returns_string(self):
        self.assertEqual(
            str.__mod__("a %d %g %s", (123, 3.14, "baz")), "a 123 3.14 baz"
        )

    def test_specifier_missing_format_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__mod__("%", ())
        self.assertEqual(str(context.exception), "incomplete format")
        with self.assertRaises(ValueError) as context:
            str.__mod__("%(foo)", {"foo": None})
        self.assertEqual(str(context.exception), "incomplete format")

    def test_unknown_specifier_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__mod__("try %Y", (42,))
        self.assertEqual(
            str(context.exception), "unsupported format character 'Y' (0x59) at index 5"
        )

    def test_too_few_args_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%s%s", ("foo",))
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )

    def test_too_many_args_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("hello", 42)
        self.assertEqual(
            str(context.exception),
            "not all arguments converted during string formatting",
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%d%s", (1, "foo", 3))
        self.assertEqual(
            str(context.exception),
            "not all arguments converted during string formatting",
        )


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

    Py_TPFLAGS_HEAPTYPE = 1 << 9
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

    def test_dunder_flags_with_managed_type_is_heap_type(self):
        class C:
            pass

        self.assertTrue(C.__flags__ & TypeTests.Py_TPFLAGS_HEAPTYPE)

    def test_dunder_flags_with_managed_type_is_ready(self):
        class C:
            pass

        self.assertTrue(C.__flags__ & TypeTests.Py_TPFLAGS_READY)
        self.assertFalse(C.__flags__ & TypeTests.Py_TPFLAGS_READYING)

    def test_dunder_flags_without_dunder_abstractmethods_returns_false(self):
        class C:
            pass

        with self.assertRaises(AttributeError):
            C.__abstractmethods__

        self.assertFalse(C.__flags__ & TypeTests.Py_TPFLAGS_IS_ABSTRACT)

    def test_dunder_flags_with_empty_dunder_abstractmethods_returns_false(self):
        import abc

        class C(metaclass=abc.ABCMeta):
            pass

        self.assertEqual(len(C.__abstractmethods__), 0)
        self.assertFalse(C.__flags__ & TypeTests.Py_TPFLAGS_IS_ABSTRACT)

    def test_dunder_flags_with_non_empty_dunder_abstractmethods_returns_true(self):
        import abc

        class C(metaclass=abc.ABCMeta):
            @abc.abstractmethod
            def foo(self):
                pass

        self.assertEqual(len(C.__abstractmethods__), 1)
        self.assertTrue(C.__flags__ & TypeTests.Py_TPFLAGS_IS_ABSTRACT)

    def test_dunder_flags_sets_long_subclass_if_int_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & TypeTests.Py_TPFLAGS_LONG_SUBCLASS)

        class D(int):
            pass

        self.assertTrue(int.__flags__ & TypeTests.Py_TPFLAGS_LONG_SUBCLASS)
        self.assertTrue(D.__flags__ & TypeTests.Py_TPFLAGS_LONG_SUBCLASS)

    def test_dunder_flags_sets_list_subclass_if_list_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & TypeTests.Py_TPFLAGS_LIST_SUBCLASS)

        class D(list):
            pass

        self.assertTrue(list.__flags__ & TypeTests.Py_TPFLAGS_LIST_SUBCLASS)
        self.assertTrue(D.__flags__ & TypeTests.Py_TPFLAGS_LIST_SUBCLASS)

    def test_dunder_flags_sets_tuple_subclass_if_tuple_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & TypeTests.Py_TPFLAGS_TUPLE_SUBCLASS)

        class D(tuple):
            pass

        self.assertTrue(tuple.__flags__ & TypeTests.Py_TPFLAGS_TUPLE_SUBCLASS)
        self.assertTrue(D.__flags__ & TypeTests.Py_TPFLAGS_TUPLE_SUBCLASS)

    def test_dunder_flags_sets_bytes_subclass_if_bytes_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & TypeTests.Py_TPFLAGS_BYTES_SUBCLASS)

        class D(bytes):
            pass

        self.assertTrue(bytes.__flags__ & TypeTests.Py_TPFLAGS_BYTES_SUBCLASS)
        self.assertTrue(D.__flags__ & TypeTests.Py_TPFLAGS_BYTES_SUBCLASS)

    def test_dunder_flags_sets_unicode_subclass_if_str_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & TypeTests.Py_TPFLAGS_UNICODE_SUBCLASS)

        class D(str):
            pass

        self.assertTrue(str.__flags__ & TypeTests.Py_TPFLAGS_UNICODE_SUBCLASS)
        self.assertTrue(D.__flags__ & TypeTests.Py_TPFLAGS_UNICODE_SUBCLASS)

    def test_dunder_flags_sets_dict_subclass_if_dict_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & TypeTests.Py_TPFLAGS_DICT_SUBCLASS)

        class D(dict):
            pass

        self.assertTrue(dict.__flags__ & TypeTests.Py_TPFLAGS_DICT_SUBCLASS)
        self.assertTrue(D.__flags__ & TypeTests.Py_TPFLAGS_DICT_SUBCLASS)

    def test_dunder_flags_sets_base_exc_subclass_if_base_exception_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & TypeTests.Py_TPFLAGS_BASE_EXC_SUBCLASS)

        class D(BaseException):
            pass

        self.assertTrue(
            BaseException.__flags__ & TypeTests.Py_TPFLAGS_BASE_EXC_SUBCLASS
        )
        self.assertTrue(D.__flags__ & TypeTests.Py_TPFLAGS_BASE_EXC_SUBCLASS)
        self.assertTrue(MemoryError.__flags__ & TypeTests.Py_TPFLAGS_BASE_EXC_SUBCLASS)

    def test_dunder_flags_sets_type_subclass_if_type_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & TypeTests.Py_TPFLAGS_TYPE_SUBCLASS)

        class D(type):
            pass

        self.assertTrue(type.__flags__ & TypeTests.Py_TPFLAGS_TYPE_SUBCLASS)
        self.assertTrue(D.__flags__ & TypeTests.Py_TPFLAGS_TYPE_SUBCLASS)

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

    def test_dunder_new_with_non_dict_type_dict_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__new__(type, "X", (object,), 1)

    def test_dunder_new_returns_type_instance(self):
        X = type.__new__(type, "X", (object,), {})
        self.assertIsInstance(X, type)
        self.assertEqual(X.__name__, "X")
        self.assertEqual(X.__qualname__, "X")

    def test_dunder_new_adds_to_base_dunder_subclasses(self):
        A = type.__new__(type, "A", (object,), {})
        B = type.__new__(type, "B", (object,), {})
        C = type.__new__(type, "C", (A, B), {})
        D = type.__new__(type, "D", (A,), {})
        self.assertEqual(A.__subclasses__(), [C, D])
        self.assertEqual(B.__subclasses__(), [C])
        self.assertEqual(C.__subclasses__(), [])
        self.assertEqual(D.__subclasses__(), [])

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

    def test_mro_with_custom_method_propagates_exception(self):
        class Meta(type):
            def mro(cls):
                raise KeyError

        with self.assertRaises(KeyError):

            class Foo(metaclass=Meta):
                pass

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
