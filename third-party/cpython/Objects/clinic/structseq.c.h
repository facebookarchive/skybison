/*[clinic input]
preserve
[clinic start generated code]*/

static PyObject *
structseq_new_impl(PyTypeObject *type, PyObject *arg, PyObject *dict);

static PyObject *
structseq_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"sequence", "dict", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "structseq", 0};
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *arg;
    PyObject *dict = NULL;

    Py_ssize_t copy_len = Py_MIN(nargs, 2);
    for (Py_ssize_t i = 0; i < copy_len; i++) {
        argsbuf[i] = PyTuple_GET_ITEM(args, i);
    }
    fastargs = _PyArg_UnpackKeywords(argsbuf, nargs, kwargs, NULL, &_parser, 1, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    arg = fastargs[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    dict = fastargs[1];
skip_optional_pos:
    return_value = structseq_new_impl(type, arg, dict);

exit:
    return return_value;
}
/*[clinic end generated code: output=265f83ab0ecd9637 input=a9049054013a1b77]*/
