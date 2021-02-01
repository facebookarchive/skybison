from _builtins import _builtin


class JSONDecodeError(ValueError):
    """Subclass of ValueError with the following additional properties:

    msg: The unformatted error message
    doc: The JSON document being parsed
    pos: The start index of doc where parsing failed
    lineno: The line corresponding to pos
    colno: The column corresponding to pos

    """

    # Note that this exception is used from _json
    def __init__(self, msg, doc, pos):
        lineno = doc.count("\n", 0, pos) + 1
        colno = pos - doc.rfind("\n", 0, pos)
        errmsg = f"{msg}: line {lineno} column {colno} (char {pos})"
        super().__init__(errmsg)
        self.msg = msg
        self.doc = doc
        self.pos = pos
        self.lineno = lineno
        self.colno = colno

    def __reduce__(self):
        return self.__class__, (self.msg, self.doc, self.pos)


def _decode_with_cls(
    s, cls, object_hook, parse_float, parse_int, parse_constant, object_pairs_hook, **kw
):
    if cls is None:
        cls = _JSONDecoder
    if object_hook is not None:
        kw["object_hook"] = object_hook
    if object_pairs_hook is not None:
        kw["object_pairs_hook"] = object_pairs_hook
    if parse_float is not None:
        kw["parse_float"] = parse_float
    if parse_int is not None:
        kw["parse_int"] = parse_int
    if parse_constant is not None:
        kw["parse_constant"] = parse_constant
    return cls(**kw).decode(s)


def loads(
    s,
    *,
    encoding=None,
    cls=None,
    object_hook=None,
    parse_float=None,
    parse_int=None,
    parse_constant=None,
    object_pairs_hook=None,
    **kw,
):
    _builtin()


from json.decoder import JSONDecoder as _JSONDecoder
