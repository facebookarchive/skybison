#!/usr/bin/env python3
import _frozen_importlib
import argparse
import importlib
import inspect
import modulefinder
import sys


class FunctionEvent:
    BUILTIN = "builtin/extension"
    INTERPRETED = "interpreted"

    def __init__(self, kind, function):
        self.module_name = "[unknown module]"
        self.function_name = "[unknown function]"
        self.kind = kind
        if function is not None:
            if function.__module__ is not None and function.__module__ != "":
                self.module_name = function.__module__
            if function.__qualname__ is not None:
                self.function_name = function.__qualname__


class Call(FunctionEvent):
    pass


class Return(FunctionEvent):
    pass


def _has_code(func, code):
    while func is not None:
        func_code = getattr(func, "__code__", None)
        if func_code is code:
            return func
        # Attempt to find the decorated function
        func = getattr(func, "__wrapped__", None)
    return None


def get_func_in_mro(obj, code):
    """Find a function with the supplied code object by looking in the mro of obj"""
    # FunctionType is incompatible with Callable
    # https://github.com/python/typeshed/issues/1378
    val = getattr(obj, code.co_name, None)
    if val is None:
        return None
    if isinstance(val, (classmethod, staticmethod)):
        cand = val.__func__
    elif (
        isinstance(val, property) and (val.fset is None) and (val.fdel is None)
    ):
        cand = val.fget
    else:
        cand = val
    return _has_code(cand, code)


def get_func(frame):
    """Return the function whose code object corresponds to the supplied stack frame."""
    code = frame.f_code
    if code.co_name is None:
        return None
    # First, try to find the function in globals
    cand = frame.f_globals.get(code.co_name, None)
    func = _has_code(cand, code)
    # If that failed, as will be the case with class and instance methods, try
    # to look up the function from the first argument. In the case of class/instance
    # methods, this should be the class (or an instance of the class) on which our
    # method is defined.
    if func is None and code.co_argcount >= 1:
        first_arg = frame.f_locals.get(code.co_varnames[0])
        func = get_func_in_mro(first_arg, code)
    # If we still can't find the function, as will be the case with static methods,
    # try looking at classes in global scope.
    if func is None:
        for v in frame.f_globals.values():
            if not isinstance(v, type):
                continue
            func = get_func_in_mro(v, code)
            if func is not None:
                break
    return func


class CallRecorder:
    C_EVENTS = {"c_call", "c_return"}

    EVENT_CLASS = {
        "c_call": Call,
        "c_exception": Return,
        "c_return": Return,
        "call": Call,
        "return": Return,
    }

    EVENT_TYP = {
        "c_call": FunctionEvent.BUILTIN,
        "c_exception": FunctionEvent.BUILTIN,
        "c_return": FunctionEvent.BUILTIN,
        "call": FunctionEvent.INTERPRETED,
        "return": FunctionEvent.INTERPRETED,
    }

    def __init__(self, event_log):
        self.event_log = event_log

    def get_func(self, frame, event, arg):
        if event in self.C_EVENTS:
            return arg
        try:
            func = get_func(frame)
        except Exception as exc:
            print(f"ERROR: Exception while looking up function: {exc}")
            func = None
        return func

    def __call__(self, frame, event, arg):
        func = self.get_func(frame, event, arg)
        self.event_log.append(
            self.EVENT_CLASS[event](self.EVENT_TYP[event], func)
        )


class Node:
    def __init__(self, name, attrs=None, children=None):
        self.name = name
        self.attrs = attrs or {}
        self.children = children or []
        self.opening_tag = f"<{self.name}>"
        if attrs:
            attrstr = " ".join(
                f'{name}="{val}"' for name, val in self.attrs.items()
            )
            self.opening_tag = f"<{self.name} {attrstr}>"
        self.closing_tag = f"</{self.name}>"

    def append(self, node):
        self.children.append(node)

    def pop(self):
        return self.children.pop()

    def __repr__(self):
        return f'Node("{self.name}", {repr(self.attrs)}, {repr(self.children)})'


class Text:
    def __init__(self, content):
        self.content = content

    def __repr__(self):
        return f'Text("{self.content}")'


class Document:
    HEADER = """
<html>
  <head>
    <link href="https://thomasf.github.io/solarized-css/solarized-light.min.css" rel="stylesheet"></link>
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js"></script>
    <style>
      .fn {
        font-weight: bolder;
      }
      .mn {
        font-weight: bolder;
      }
      .fk {
        font-weight: bolder;
      }
      .hidden {
        display: none;
      }
      .indent {
        padding-left: 16px;
      }
    </style>
  </head>
"""

    def __init__(self):
        self.level = 0
        self.lines = []

    def append(self, s):
        self.lines.append("  " * self.level + s)

    def process(self, node):
        if isinstance(node, Node):
            if len(node.children) == 1 and isinstance(node.children[0], Text):
                self.append(
                    node.opening_tag
                    + node.children[0].content
                    + node.closing_tag
                )
            else:
                self.append(node.opening_tag)
                self.level += 1
                for child in node.children:
                    self.process(child)
                self.level -= 1
                self.append(node.closing_tag)
        elif isinstance(node, Text):
            self.append(node.content)
        else:
            raise TypeError(f"Cannot process node: {node}")
        return self

    def render(self):
        return self.HEADER + "\n".join(self.lines) + "</html>"


def toggle(node):
    node_id = node.attrs["id"]
    return Node(
        "a",
        attrs={
            "href": "#",
            "onClick": f"$('#{node_id}').toggle(); return false;",
        },
        children=[Text("+/-")],
    )


def toggle_bar(node, title):
    return Node("div", children=[toggle(node), title])


CONTAINER_ID_CTR = 0


def toggle_bar_and_container(title):
    global CONTAINER_ID_CTR
    node_id = f"c{CONTAINER_ID_CTR}"
    CONTAINER_ID_CTR += 1
    container = Node("div", attrs={"id": node_id, "class": "hidden indent"})
    return toggle_bar(container, title), container


def compute_call_map(event_log, kind):
    funcs = {}
    for event in event_log:
        if isinstance(event, Call) and event.kind == kind:
            if event.module_name not in funcs:
                funcs[event.module_name] = set()
            funcs[event.module_name].add(event.function_name)
    return funcs


def called_function_section(called_funcs, kind):
    total_names = 0
    for names in called_funcs.values():
        total_names += len(names)
    all_bar, all_funcs = toggle_bar_and_container(
        Text(f"Called {total_names} {kind} functions")
    )
    for mod, funcs in sorted(
        called_funcs.items(), key=lambda kv: len(kv[1]), reverse=True
    ):
        mod_bar, mod_funcs = toggle_bar_and_container(
            Text(f"{mod} ({len(funcs)})")
        )
        for func in sorted(funcs):
            mod_funcs.append(Node("div", children=[Text(func)]))
        all_funcs.append(mod_bar)
        all_funcs.append(mod_funcs)
    return Node("div", children=[all_bar, all_funcs])


def call_node(event):
    node = Node(
        "span",
        children=[
            Node(
                "span",
                attrs={"class": "fn"},
                children=[Text(event.function_name)],
            ),
            Text(" in "),
            Node(
                "span",
                attrs={"class": "mn"},
                children=[Text(event.module_name)],
            ),
        ],
    )
    if event.kind == FunctionEvent.BUILTIN:
        node.append(
            Node("span", attrs={"class": "fk"}, children=[Text(event.kind)])
        )
    return node


def execution_trace_section(event_log):
    section = Node("div")
    stack = [section]
    for event in event_log:
        if isinstance(event, Call):
            bar, callees = toggle_bar_and_container(call_node(event))
            stack[-1].append(bar)
            stack[-1].append(callees)
            stack.append(callees)
        elif isinstance(event, Return):
            callees = stack.pop()
            if not callees.children:
                # If there are no callees, remove the callee container div and
                # toggle bar. Replace it with a simple text node.
                stack[-1].children.pop()
                bar = stack[-1].children.pop()
                stack[-1].append(Node("div", children=[bar.children[-1]]))
    return section


class ImportTrace:
    def __init__(self, module):
        self.module = module
        self.imports = []


class ImportTracer(modulefinder.ModuleFinder):
    def __init__(self, *args, **kwargs):
        self.seen = set()
        self.traces = []
        self.stack = []
        super().__init__(*args, **kwargs)

    def import_module(self, partname, fqname, parent):
        if fqname in self.modules:
            return self.modules[fqname]
        trace = ImportTrace(fqname)
        if self.stack:
            self.stack[-1].imports.append(trace)
        self.stack.append(trace)
        result = super().import_module(partname, fqname, parent)
        self.stack.pop()
        if fqname not in self.modules and self.stack:
            self.stack[-1].imports.pop()
        if not self.stack:
            self.traces.append(trace)
        return result

    def compute_imports(self, path):
        self.run_script(path)
        return self.modules.keys(), self.traces

    def scan_code(self, co, m):
        code = co.co_code
        scanner = self.scan_opcodes
        for what, args in scanner(co):
            if what == "store":
                name, = args
                m.globalnames[name] = 1
            elif what == "absolute_import":
                fromlist, name = args
                have_star = 0
                if fromlist is not None:
                    if "*" in fromlist:
                        have_star = 1
                    fromlist = [f for f in fromlist if f != "*"]
                self._safe_import_hook(name, m, fromlist, level=0)
                if have_star:
                    # We've encountered an "import *". If it is a Python module,
                    # the code has already been parsed and we can suck out the
                    # global names.
                    mm = None
                    if m.__path__:
                        # At this point we don't know whether 'name' is a
                        # submodule of 'm' or a global module. Let's just try
                        # the full name first.
                        mm = self.modules.get(m.__name__ + "." + name)
                    if mm is None:
                        mm = self.modules.get(name)
                    if mm is not None:
                        m.globalnames.update(mm.globalnames)
                        m.starimports.update(mm.starimports)
                        if mm.__code__ is None:
                            m.starimports[name] = 1
                    else:
                        m.starimports[name] = 1
            elif what == "relative_import":
                level, fromlist, name = args
                if name:
                    self._safe_import_hook(name, m, fromlist, level=level)
                else:
                    parent = self.determine_parent(m, level=level)
                    self._safe_import_hook(
                        parent.__name__, None, fromlist, level=0
                    )
            else:
                # We don't expect anything else from the generator.
                raise RuntimeError(what)


def imported_modules_section(imports):
    bar, mod_list = toggle_bar_and_container(
        Text(f"Imported {len(imports)} modules")
    )
    for name in sorted(imports):
        mod_list.append(Node("div", children=[Text(name)]))
    return Node("div", children=[bar, mod_list])


def build_import_trace(trace, called_builtins, indent=False):
    attrs = {}
    if indent:
        attrs = {"class": "indent"}
    msg = trace.module
    try:
        module = importlib.import_module(trace.module)
        if module.__loader__ is _frozen_importlib.BuiltinImporter:
            msg += " (builtin)"
    except:
        msg += f" (failed importing module)"
    if trace.module in called_builtins:
        msg += f" ({len(called_builtins[trace.module])} builtin/extension functions called)"
    container = Node("div", children=[Text(msg)], attrs=attrs)
    if trace.imports:
        for child in trace.imports:
            container.append(build_import_trace(child, called_builtins, True))
    return container


def import_trace_section(traces, called_builtins):
    bar, container = toggle_bar_and_container(Text("Import Trace:"))
    for trace in traces:
        container.append(build_import_trace(trace, called_builtins))
    return Node("div", children=[bar, container])


def render_summary(path, source, event_log):
    imported_modules, traces = ImportTracer().compute_imports(path)
    called_interpreted = compute_call_map(event_log, FunctionEvent.INTERPRETED)
    called_builtins = compute_call_map(event_log, FunctionEvent.BUILTIN)
    body = Node(
        "body",
        children=[
            Node("h1", children=[Text(f"Tracing summary for {path}")]),
            Node("h2", children=[Text("Source:")]),
            Node("pre", children=[Text(source)]),
            Node("h2", children=[Text("Summary stats:")]),
            imported_modules_section(imported_modules),
            called_function_section(called_builtins, FunctionEvent.BUILTIN),
            called_function_section(
                called_interpreted, FunctionEvent.INTERPRETED
            ),
            Node("h2", children=[Text("Import Trace:")]),
            import_trace_section(traces, called_builtins),
            Node("h2", children=[Text("Call Trace:")]),
            execution_trace_section(event_log),
        ],
    )
    doc = Document()
    doc.process(body)
    return doc.render()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        description="""
Determine what modules are imported and what functions are called by a script.

This script does the following:

  1. Executes the script at [script_path] and records what functions are called.
  2. Statically analyzes the script to determine what modules are imported. Note
     that this analysis does not consider inline imports and therefore may underestimate
     the set of modules that are imported during execution.

It then writes a report to [output_path] with the following information:

  - A list of the modules that are imported unconditionally.
  - A list of all the functions that were called, grouped by whether or not they
    are interpreted or native functions.
  - A call trace.
  - An import trace. Each imported module will appear exactly once in the trace,
    the first time it is executed.
""",
    )
    parser.add_argument("script_path", help="The path to the script to execute")
    parser.add_argument("output_path", help="Where to write the report")
    args = parser.parse_args()

    # Compile the script
    with open(args.script_path, "r") as f:
        source = f.read()
    code = compile(source, args.script_path, "exec")

    event_log = []
    sys.setprofile(CallRecorder(event_log))
    try:
        exec(code)
    finally:
        sys.setprofile(None)

    with open(args.output_path, "w+") as f:
        f.write(render_summary(args.script_path, source, event_log))
