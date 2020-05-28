#!/usr/bin/env python3
import contextvars
import unittest


class ContextTests(unittest.TestCase):
    def test_dunder_contains_with_invalid_self(self):
        with self.assertRaises(TypeError):
            contextvars.Context.__contains__(None, None)

    def test_dunder_eq_with_invalid_self(self):
        with self.assertRaises(TypeError):
            contextvars.Context.__eq__(None, None)

    def test_dunder_getitem_with_invalid_self(self):
        with self.assertRaises(TypeError):
            contextvars.Context.__getitem__(None, None)

    def test_dunder_iter_with_invalid_self(self):
        with self.assertRaises(TypeError):
            contextvars.Context.__iter__(None)

    def test_dunder_len_with_invalid_self(self):
        with self.assertRaises(TypeError):
            contextvars.Context.__len__(None)

    def test_dunder_new_with_invalid_cls(self):
        with self.assertRaises(TypeError):
            contextvars.Context.__new__(None)

    def test_copy_with_invalid_self(self):
        with self.assertRaises(TypeError):
            contextvars.Context.copy(None)

    def test_get_with_invalid_self(self):
        with self.assertRaises(TypeError):
            contextvars.Context.get(None, None)

    def test_items_with_invalid_self(self):
        with self.assertRaises(TypeError):
            contextvars.Context.items(None)

    def test_keys_with_invalid_self(self):
        with self.assertRaises(TypeError):
            contextvars.Context.keys(None)

    def test_run_with_invalid_self(self):
        with self.assertRaises(TypeError):
            contextvars.Context.keys(None, None)

    def test_values_with_invalid_self(self):
        with self.assertRaises(TypeError):
            contextvars.Context.values(None)

    def test_dunder_contains_only_allows_context_var_types(self):
        with self.assertRaises(TypeError):
            contextvars.Context().__contains__(None)

    def test_dunder_contains_finds_var_from_context(self):
        ctx = contextvars.Context()

        def f_with_ctx():
            var = contextvars.ContextVar("foo")
            var.set(1)
            self.assertTrue(ctx.__contains__(var))

        ctx.run(f_with_ctx)

    def test_dunder_contains_does_not_find_var_from_another_context(self):
        var = contextvars.ContextVar("foo")
        var.set(1)
        ctx = contextvars.Context()

        def f_with_ctx():
            self.assertFalse(ctx.__contains__(var))

        ctx.run(f_with_ctx)

    def test_dunder_eq_matches_with_same_data(self):
        ctx1 = contextvars.Context()
        var = None

        def f_with_ctx1():
            nonlocal var
            var = contextvars.ContextVar("foo")
            var.set(1)

        ctx1.run(f_with_ctx1)

        ctx2 = contextvars.Context()

        def f_with_ctx2():
            nonlocal var
            var.set(1)

        ctx2.run(f_with_ctx2)
        self.assertTrue(ctx1.__eq__(ctx2))

    def test_dunder_eq_does_not_match_with_different_data(self):
        ctx1 = contextvars.Context()
        var = None

        def f_with_ctx1():
            nonlocal var
            var = contextvars.ContextVar("foo")
            var.set(1)

        ctx1.run(f_with_ctx1)
        ctx2 = contextvars.Context()

        def f_with_ctx2():
            nonlocal var
            var.set(2)

        ctx2.run(f_with_ctx2)
        self.assertFalse(ctx1.__eq__(ctx2))

    def test_dunder_eq_returns_not_implemented_on_mismatched_other_type(self):
        self.assertEqual(contextvars.Context().__eq__(None), NotImplemented)

    def test_dunder_getitem_only_allows_context_var_types(self):
        with self.assertRaises(TypeError):
            contextvars.Context().__getitem__(None)

    def test_dunder_getitem_returns_var_from_context(self):
        ctx = contextvars.Context()

        def f_with_ctx():
            nonlocal ctx
            var = contextvars.ContextVar("foo")
            var.set(1)
            self.assertEqual(ctx.__getitem__(var), 1)

        ctx.run(f_with_ctx)

    def test_dunder_getitem_does_not_return_var_from_another_context(self):
        var = contextvars.ContextVar("foo")
        var.set(1)
        ctx = contextvars.Context()

        def f_with_ctx():
            nonlocal var
            with self.assertRaises(KeyError):
                contextvars.Context().__getitem__(var)

        ctx.run(f_with_ctx)

    def test_dunder_hash_is_not_set(self):
        self.assertIsNone(contextvars.Context.__hash__)

    def test_dunder_len_with_0_items(self):
        ctx = contextvars.Context()
        self.assertEqual(ctx.__len__(), 0)

    def test_dunder_len_with_1_items(self):
        ctx = contextvars.Context()

        def f_with_ctx():
            var = contextvars.ContextVar("foo")
            var.set(1)

        ctx.run(f_with_ctx)
        self.assertEqual(ctx.__len__(), 1)

    def test_dunder_len_with_2_items(self):
        ctx = contextvars.Context()

        def f_with_ctx():
            var1 = contextvars.ContextVar("foo")
            var1.set(1)
            var2 = contextvars.ContextVar("bar")
            var2.set(2)

        ctx.run(f_with_ctx)
        self.assertEqual(ctx.__len__(), 2)

    def test_dunder_iter_with_0_items(self):
        ctx = contextvars.Context()
        self.assertEqual(list(ctx), [])

    def test_dunder_iter_with_1_items(self):
        ctx = contextvars.Context()

        def f_with_ctx():
            nonlocal ctx
            var = contextvars.ContextVar("foo")
            var.set(1)
            self.assertEqual(list(ctx), [var])

        ctx.run(f_with_ctx)

    def test_dunder_iter_with_2_items(self):
        ctx = contextvars.Context()

        def f_with_ctx():
            nonlocal ctx
            var1 = contextvars.ContextVar("foo")
            var1.set(1)
            var2 = contextvars.ContextVar("bar")
            var2.set(2)
            self.assertEqual(set(ctx), {var1, var2})

        ctx.run(f_with_ctx)

    def test_copy_produces_context_with_same_data(self):
        ctx = contextvars.Context()

        def f_with_ctx():
            var1 = contextvars.ContextVar("foo")
            var1.set(1)
            var2 = contextvars.ContextVar("bar")
            var2.set(2)

        ctx.run(f_with_ctx)
        ctx_copy = ctx.copy()
        self.assertEqual(list(ctx_copy.items()), list(ctx.items()))

    def test_copy_produces_new_context(self):
        ctx = contextvars.Context()
        ctx_copy = ctx.copy()
        self.assertIsNot(ctx, ctx_copy)

    def test_copy_result_has_no_prev_context(self):
        ctx = contextvars.Context()

        def f():
            def g():
                pass

            ctx_copy = ctx.copy()
            # This would raise a RuntimeError if 'ctx' were used
            ctx_copy.run(g)

    def test_copy_context_mutation_does_not_affect_source_context(self):
        ctx = contextvars.Context()
        ctx_copy = ctx.copy()

        def f_with_ctx_copy():
            nonlocal ctx
            var = contextvars.ContextVar("foo")
            var.set(1)
            self.assertFalse(ctx.__contains__(var))

        ctx_copy.run(f_with_ctx_copy)

    def test_copy_source_mutation_does_not_affect_copy_context(self):
        ctx = contextvars.Context()
        ctx_copy = ctx.copy()

        def f_with_ctx():
            nonlocal ctx_copy
            var = contextvars.ContextVar("foo")
            var.set(1)
            self.assertFalse(ctx_copy.__contains__(var))

        ctx.run(f_with_ctx)

    def test_get_with_invalid_key_type_raises(self):
        ctx = contextvars.Context()
        with self.assertRaises(TypeError):
            ctx.get(None)

    def test_get_valid_key_returns_value(self):
        ctx = contextvars.Context()

        def f_with_ctx():
            var = contextvars.ContextVar("foo")
            var.set(1)
            self.assertEqual(ctx.get(var), 1)

        ctx.run(f_with_ctx)

    def test_get_missing_key_returns_specified_default(self):
        var = contextvars.ContextVar("foo")
        var.set(1)
        ctx = contextvars.Context()
        self.assertEqual(ctx.get(var, 2), 2)

    def test_get_missing_key_is_none_with_no_specified_default(self):
        var = contextvars.ContextVar("foo")
        var.set(1)
        ctx = contextvars.Context()
        self.assertIsNone(ctx.get(var))

    def test_items(self):
        ctx = contextvars.Context()

        def f_with_ctx():
            nonlocal ctx
            var1 = contextvars.ContextVar("foo")
            var1.set(1)
            var2 = contextvars.ContextVar("bar")
            var2.set(2)
            self.assertEqual(set(ctx.items()), {(var1, 1), (var2, 2)})

        ctx.run(f_with_ctx)

    def test_keys(self):
        ctx = contextvars.Context()

        def f_with_ctx():
            nonlocal ctx
            var1 = contextvars.ContextVar("foo")
            var1.set(1)
            var2 = contextvars.ContextVar("bar")
            var2.set(2)
            self.assertEqual(set(ctx.keys()), {var1, var2})

        ctx.run(f_with_ctx)

    def test_values(self):
        ctx = contextvars.Context()

        def f_with_ctx():
            nonlocal ctx
            var1 = contextvars.ContextVar("foo")
            var1.set(1)
            var2 = contextvars.ContextVar("bar")
            var2.set(2)
            self.assertEqual(set(ctx.values()), {1, 2})

        ctx.run(f_with_ctx)

    def test_run_passes_through_args(self):
        def f(a, *, b=None):
            a.assertEqual(b, 1)

        contextvars.Context().run(f, self, b=1)

    def test_run_cannot_have_nested_calls_to_same_context(self):
        ctx = contextvars.Context()

        def f():
            def g():
                pass

            ctx.run(g)

        with self.assertRaises(RuntimeError):
            ctx.run(f)

    def test_run_can_be_called_multiple_times_on_context(self):
        ctx = contextvars.Context()

        def f():
            pass

        ctx.run(f)
        ctx.run(f)

    def test_run_can_have_nested_calls_to_copied_context(self):
        ctx = contextvars.Context()
        ctx_copy = ctx.copy()

        def f():
            def g():
                pass

            ctx_copy.run(g)

        ctx.run(f)

    def test_run_protects_previous_var_values(self):
        def f_with_empty_context():
            var = contextvars.ContextVar("foo")
            var.set(1)
            ctx = contextvars.Context()
            ctx.run(lambda: var.set(2))
            self.assertEqual(var.get(), 1)

        contextvars.Context().run(f_with_empty_context)


class ContextVarTests(unittest.TestCase):
    def test_dunder_new_with_invalid_cls(self):
        with self.assertRaises(TypeError):
            contextvars.ContextVar.__new__(None, None)

    def test_dunder_repr_with_invalid_self(self):
        with self.assertRaises(TypeError):
            contextvars.ContextVar.__repr__(None)

    def test_get_with_invalid_self(self):
        with self.assertRaises(TypeError):
            contextvars.ContextVar.get(None)

    def test_set_with_invalid_self(self):
        with self.assertRaises(TypeError):
            contextvars.ContextVar.set(None, None)

    def test_reset_with_invalid_self(self):
        with self.assertRaises(TypeError):
            contextvars.ContextVar.reset(None, None)

    def test_dunder_repr_with_default(self):
        c = contextvars.ContextVar("foo", default=None)
        self.assertRegex(
            c.__repr__(), r"<ContextVar name='foo' default=None at 0x[0-9a-f]+>"
        )

    def test_dunder_new_with_non_string_name(self):
        with self.assertRaises(TypeError):
            contextvars.ContextVar.__new__(contextvars.ContextVar, None)

    def test_dunder_repr_without_default(self):
        var = contextvars.ContextVar("foo")
        self.assertRegex(var.__repr__(), r"<ContextVar name='foo' at 0x[0-9a-f]+>")

    def test_dunder_repr_recursive(self):
        backref = []
        var = contextvars.ContextVar("foo", default=backref)
        backref.append(var)
        self.assertIn("...", var.__repr__())

    def test_name_property(self):
        var = contextvars.ContextVar("foo")
        self.assertEqual(var.name, "foo")

    def test_get_unset_value_with_no_defaults(self):
        def f_with_empty_context():
            var = contextvars.ContextVar("foo")
            with self.assertRaises(LookupError):
                var.get()

        contextvars.Context().run(f_with_empty_context)

    def test_get_value_set_in_current_context(self):
        var = contextvars.ContextVar("foo")
        var.set(1)
        self.assertEqual(var.get(), 1)

    def test_get_value_set_in_current_context_overrides_defaults(self):
        var = contextvars.ContextVar("foo", default=1)
        var.set(2)
        self.assertEqual(var.get(3), 2)

    def test_get_with_default_takes_priority_over_context_var_default(self):
        def f_with_empty_context():
            var = contextvars.ContextVar("foo", default=1)
            self.assertEqual(var.get(2), 2)

        contextvars.Context().run(f_with_empty_context)

    def test_get_with_unset_and_no_default_returns_context_var_default(self):
        def f_with_empty_context():
            var = contextvars.ContextVar("foo", default=1)
            self.assertEqual(var.get(), 1)

        contextvars.Context().run(f_with_empty_context)

    def test_set_with_no_previous_value_returns_token_with_missing_old_value(self):
        def f_with_empty_context():
            var = contextvars.ContextVar("foo")
            token = var.set(None)
            self.assertIs(token.old_value, contextvars.Token.MISSING)

        contextvars.Context().run(f_with_empty_context)

    def test_set_with_previous_value_returns_token_with_old_value(self):
        def f_with_empty_context():
            var = contextvars.ContextVar("foo")
            var.set(1)
            token = var.set(2)
            self.assertEqual(token.old_value, 1)

        contextvars.Context().run(f_with_empty_context)

    def test_set_is_available_in_subsequent_get(self):
        var = contextvars.ContextVar("foo")
        var.set(1)
        self.assertEqual(var.get(), 1)

    def test_set_does_not_affect_other_contexts(self):
        ctx1 = contextvars.Context()
        var = None

        def f1_with_ctx1():
            nonlocal var
            var = contextvars.ContextVar("foo")
            var.set(1)

        ctx1.run(f1_with_ctx1)

        ctx2 = contextvars.Context()

        def f_with_ctx2():
            nonlocal var
            var.set(2)

        ctx2.run(f_with_ctx2)

        def f2_with_ctx1():
            self.assertEqual(var.get(), 1)

        ctx1.run(f2_with_ctx1)

    def test_set_returns_token_referring_to_self(self):
        var = contextvars.ContextVar("foo")
        token = var.set(1)
        self.assertIs(token.var, var)

    def test_reset_with_used_token_raises_runtime_error(self):
        var = contextvars.ContextVar("foo")
        token = var.set(1)
        var.reset(token)
        with self.assertRaises(RuntimeError):
            var.reset(token)

    def test_reset_with_token_from_another_context_var_raises_value_error(self):
        var1 = contextvars.ContextVar("foo")
        token = var1.set(1)
        var2 = contextvars.ContextVar("foo")
        with self.assertRaises(ValueError):
            var2.reset(token)

    def test_reset_with_token_from_another_context_raises_value_error(self):
        ctx1 = contextvars.Context()
        var = None
        token = None

        def f_with_ctx1():
            nonlocal var, token
            var = contextvars.ContextVar("foo")
            token = var.set(1)

        ctx1.run(f_with_ctx1)

        ctx2 = contextvars.Context()

        def f_with_ctx2():
            with self.assertRaises(ValueError):
                var.reset(token)

        ctx2.run(f_with_ctx2)

    def test_reset_with_no_previous_value_removes_value_from_context(self):
        def f_with_empty_context():
            var = contextvars.ContextVar("foo")
            token = var.set(1)
            var.reset(token)
            with self.assertRaises(LookupError):
                var.get()

        contextvars.Context().run(f_with_empty_context)

    def test_reset_restores_previous_value_to_context(self):
        var = contextvars.ContextVar("foo")
        var.set(1)
        token = var.set(2)
        var.reset(token)
        self.assertEqual(var.get(), 1)


class CopyContextTests(unittest.TestCase):
    def test_mutating_copied_context_does_not_affect_current_context(self):
        def f_with_empty_context():
            ctx_copy = contextvars.copy_context()
            var = contextvars.ContextVar("foo")
            var.set(1)
            self.assertFalse(ctx_copy.__contains__(var))

        contextvars.Context().run(f_with_empty_context)


class TokenTests(unittest.TestCase):
    def test_dunder_repr_with_invalid_self(self):
        with self.assertRaises(TypeError):
            contextvars.Token.__repr__(None)

    def test_dunder_new_raises_runtime_error(self):
        with self.assertRaises(RuntimeError):
            contextvars.Token.__new__(contextvars.Token, None, None, None)

    def test_missing_value_exists(self):
        self.assertTrue(hasattr(contextvars.Token, "MISSING"))

    def test_dunder_repr_unused(self):
        var = contextvars.ContextVar("foo")
        token = var.set(1)
        self.assertRegex(
            token.__repr__(),
            r"<Token var=<ContextVar name='foo' at 0x[0-9a-f]+> at 0x[0-9a-f]+>",
        )

    def test_dunder_repr_used(self):
        var = contextvars.ContextVar("foo")
        token = var.set(1)
        var.reset(token)
        self.assertRegex(
            token.__repr__(),
            r"<Token used var=<ContextVar name='foo' at 0x[0-9a-f]+> at 0x[0-9a-f]+>",
        )

    def test_dunder_repr_recursive(self):
        backref = []
        var = contextvars.ContextVar("foo", default=backref)
        token = var.set(1)
        backref.append(token)
        self.assertIn("...", token.__repr__())

    def test_var_property(self):
        var = contextvars.ContextVar("foo")
        token = var.set(1)
        self.assertIs(token.var, var)

    def test_old_value_property_when_there_is_an_old_value(self):
        value = object()
        var = contextvars.ContextVar("foo")
        var.set(value)
        token = var.set(object())
        self.assertIs(token.old_value, value)

    def test_old_value_property_when_missing(self):
        var = contextvars.ContextVar("foo")
        token = var.set(None)
        self.assertIs(token.old_value, contextvars.Token.MISSING)


if __name__ == "__main__":
    unittest.main()
