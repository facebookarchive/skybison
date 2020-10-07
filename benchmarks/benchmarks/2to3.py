#!/usr/bin/env python3
import glob
import io
import logging
import os.path
import sys
from lib2to3 import refactor
from lib2to3.main import main as main_2to3


BENCHMARKDIR = os.path.dirname(os.path.abspath(__file__))
FIXER_PKG = "lib2to3.fixes"


# Import all fixers upfront, to make sure bytecode is compiled
for fixer in refactor.get_fixers_from_package(FIXER_PKG):
    __import__(fixer)


expected_log = """\
INFO:RefactoringTool:Skipping optional fixer: buffer
INFO:RefactoringTool:Skipping optional fixer: idioms
INFO:RefactoringTool:Skipping optional fixer: set_literal
INFO:RefactoringTool:Skipping optional fixer: ws_comma
INFO:RefactoringTool:Refactored data/2to3/context_processors.py.txt
INFO:RefactoringTool:No changes to data/2to3/exceptions.py.txt
INFO:RefactoringTool:Refactored data/2to3/mail.py.txt
INFO:RefactoringTool:Refactored data/2to3/paginator.py.txt
INFO:RefactoringTool:No changes to data/2to3/signals.py.txt
INFO:RefactoringTool:Refactored data/2to3/urlresolvers.py.txt
INFO:RefactoringTool:No changes to data/2to3/xheaders.py.txt
INFO:RefactoringTool:Files that need to be modified:
INFO:RefactoringTool:data/2to3/context_processors.py.txt
INFO:RefactoringTool:data/2to3/exceptions.py.txt
INFO:RefactoringTool:data/2to3/mail.py.txt
INFO:RefactoringTool:data/2to3/paginator.py.txt
INFO:RefactoringTool:data/2to3/signals.py.txt
INFO:RefactoringTool:data/2to3/urlresolvers.py.txt
INFO:RefactoringTool:data/2to3/xheaders.py.txt
"""


expected_output = '''\
--- data/2to3/context_processors.py.txt	(original)
+++ data/2to3/context_processors.py.txt	(refactored)
@@ -82,7 +82,7 @@
     def __getitem__(self, perm_name):
         return self.user.has_perm("%s.%s" % (self.module_name, perm_name))
 
-    def __nonzero__(self):
+    def __bool__(self):
         return self.user.has_module_perms(self.module_name)
 
 class PermWrapper(object):
--- data/2to3/mail.py.txt	(original)
+++ data/2to3/mail.py.txt	(refactored)
@@ -222,12 +222,12 @@
         conversions.
         """
         if to:
-            assert not isinstance(to, basestring), '"to" argument must be a list or tuple'
+            assert not isinstance(to, str), '"to" argument must be a list or tuple'
             self.to = list(to)
         else:
             self.to = []
         if bcc:
-            assert not isinstance(bcc, basestring), '"bcc" argument must be a list or tuple'
+            assert not isinstance(bcc, str), '"bcc" argument must be a list or tuple'
             self.bcc = list(bcc)
         else:
             self.bcc = []
@@ -259,7 +259,7 @@
             msg['Date'] = formatdate()
         if 'message-id' not in header_names:
             msg['Message-ID'] = make_msgid()
-        for name, value in self.extra_headers.items():
+        for name, value in list(self.extra_headers.items()):
             if name.lower() == 'from':
                 continue
             msg[name] = value
--- data/2to3/paginator.py.txt	(original)
+++ data/2to3/paginator.py.txt	(refactored)
@@ -70,7 +70,7 @@
         Returns a 1-based range of pages for iterating through within
         a template for loop.
         """
-        return range(1, self.num_pages + 1)
+        return list(range(1, self.num_pages + 1))
     page_range = property(_get_page_range)
 
 QuerySetPaginator = Paginator # For backwards-compatibility.
--- data/2to3/urlresolvers.py.txt	(original)
+++ data/2to3/urlresolvers.py.txt	(refactored)
@@ -133,12 +133,12 @@
             return self._callback
         try:
             self._callback = get_callable(self._callback_str)
-        except ImportError, e:
+        except ImportError as e:
             mod_name, _ = get_mod_func(self._callback_str)
-            raise ViewDoesNotExist, "Could not import %s. Error was: %s" % (mod_name, str(e))
-        except AttributeError, e:
+            raise ViewDoesNotExist("Could not import %s. Error was: %s" % (mod_name, str(e)))
+        except AttributeError as e:
             mod_name, func_name = get_mod_func(self._callback_str)
-            raise ViewDoesNotExist, "Tried %s in module %s. Error was: %s" % (func_name, mod_name, str(e))
+            raise ViewDoesNotExist("Tried %s in module %s. Error was: %s" % (func_name, mod_name, str(e)))
         return self._callback
     callback = property(_get_callback)
 
@@ -148,7 +148,7 @@
         # urlconf_name is a string representing the module containing URLconfs.
         self.regex = re.compile(regex, re.UNICODE)
         self.urlconf_name = urlconf_name
-        if not isinstance(urlconf_name, basestring):
+        if not isinstance(urlconf_name, str):
             self._urlconf_module = self.urlconf_name
         self.callback = None
         self.default_kwargs = default_kwargs or {}
@@ -182,9 +182,9 @@
                             for piece, p_args in parent:
                                 new_matches.extend([(piece + suffix, p_args + args) for (suffix, args) in matches])
                             lookups.appendlist(name, (new_matches, p_pattern + pat))
-                    for namespace, (prefix, sub_pattern) in pattern.namespace_dict.items():
+                    for namespace, (prefix, sub_pattern) in list(pattern.namespace_dict.items()):
                         namespaces[namespace] = (p_pattern + prefix, sub_pattern)
-                    for app_name, namespace_list in pattern.app_dict.items():
+                    for app_name, namespace_list in list(pattern.app_dict.items()):
                         apps.setdefault(app_name, []).extend(namespace_list)
             else:
                 bits = normalize(p_pattern)
@@ -220,7 +220,7 @@
             for pattern in self.url_patterns:
                 try:
                     sub_match = pattern.resolve(new_path)
-                except Resolver404, e:
+                except Resolver404 as e:
                     sub_tried = e.args[0].get('tried')
                     if sub_tried is not None:
                         tried.extend([(pattern.regex.pattern + '   ' + t) for t in sub_tried])
@@ -228,14 +228,14 @@
                         tried.append(pattern.regex.pattern)
                 else:
                     if sub_match:
-                        sub_match_dict = dict([(smart_str(k), v) for k, v in match.groupdict().items()])
+                        sub_match_dict = dict([(smart_str(k), v) for k, v in list(match.groupdict().items())])
                         sub_match_dict.update(self.default_kwargs)
-                        for k, v in sub_match[2].iteritems():
+                        for k, v in sub_match[2].items():
                             sub_match_dict[smart_str(k)] = v
                         return sub_match[0], sub_match[1], sub_match_dict
                     tried.append(pattern.regex.pattern)
-            raise Resolver404, {'tried': tried, 'path': new_path}
-        raise Resolver404, {'path' : path}
+            raise Resolver404({'tried': tried, 'path': new_path})
+        raise Resolver404({'path' : path})
 
     def _get_urlconf_module(self):
         try:
@@ -260,8 +260,8 @@
         mod_name, func_name = get_mod_func(callback)
         try:
             return getattr(import_module(mod_name), func_name), {}
-        except (ImportError, AttributeError), e:
-            raise ViewDoesNotExist, "Tried %s. Error was: %s" % (callback, str(e))
+        except (ImportError, AttributeError) as e:
+            raise ViewDoesNotExist("Tried %s. Error was: %s" % (callback, str(e)))
 
     def resolve404(self):
         return self._resolve_special('404')
@@ -274,7 +274,7 @@
             raise ValueError("Don't mix *args and **kwargs in call to reverse()!")
         try:
             lookup_view = get_callable(lookup_view, True)
-        except (ImportError, AttributeError), e:
+        except (ImportError, AttributeError) as e:
             raise NoReverseMatch("Error importing '%s': %s." % (lookup_view, e))
         possibilities = self.reverse_dict.getlist(lookup_view)
         for possibility, pattern in possibilities:
@@ -283,13 +283,13 @@
                     if len(args) != len(params):
                         continue
                     unicode_args = [force_unicode(val) for val in args]
-                    candidate =  result % dict(zip(params, unicode_args))
+                    candidate =  result % dict(list(zip(params, unicode_args)))
                 else:
                     if set(kwargs.keys()) != set(params):
                         continue
-                    unicode_kwargs = dict([(k, force_unicode(v)) for (k, v) in kwargs.items()])
+                    unicode_kwargs = dict([(k, force_unicode(v)) for (k, v) in list(kwargs.items())])
                     candidate = result % unicode_kwargs
-                if re.search(u'^%s' % pattern, candidate, re.UNICODE):
+                if re.search('^%s' % pattern, candidate, re.UNICODE):
                     return candidate
         # lookup_view can be URL label, or dotted path, or callable, Any of
         # these can be passed in at the top, but callables are not friendly in
@@ -318,7 +318,7 @@
     if prefix is None:
         prefix = get_script_prefix()
 
-    if not isinstance(viewname, basestring):
+    if not isinstance(viewname, str):
         view = viewname
     else:
         parts = viewname.split(':')
@@ -348,13 +348,13 @@
                 extra, resolver = resolver.namespace_dict[ns]
                 resolved_path.append(ns)
                 prefix = prefix + extra
-            except KeyError, key:
+            except KeyError as key:
                 if resolved_path:
                     raise NoReverseMatch("%s is not a registered namespace inside '%s'" % (key, ':'.join(resolved_path)))
                 else:
                     raise NoReverseMatch("%s is not a registered namespace" % key)
 
-    return iri_to_uri(u'%s%s' % (prefix, resolver.reverse(view,
+    return iri_to_uri('%s%s' % (prefix, resolver.reverse(view,
             *args, **kwargs)))
 
 def clear_url_caches():
@@ -377,7 +377,7 @@
     wishes to construct their own URLs manually (although accessing the request
     instance is normally going to be a lot cleaner).
     """
-    return _prefixes.get(currentThread(), u'/')
+    return _prefixes.get(currentThread(), '/')
 
 def set_urlconf(urlconf_name):
     """
\
'''


pyfiles = """\
data/2to3/__init__.py.txt
data/2to3/context_processors.py.txt
data/2to3/exceptions.py.txt
data/2to3/mail.py.txt
data/2to3/paginator.py.txt
data/2to3/signals.py.txt
data/2to3/template_loader.py.txt
data/2to3/urlresolvers.py.txt
data/2to3/xheaders.py.txt
""".splitlines()


def bench_2to3():
    os.chdir(BENCHMARKDIR)
    argv_original = sys.argv
    sys.argv = [sys.argv[0], "-f", "all"] + pyfiles
    log = io.StringIO()
    logging.basicConfig(stream=log, level=logging.INFO)
    sys.stdout = io.StringIO()
    main_2to3(FIXER_PKG)
    sys.argv = argv_original
    output = sys.stdout.getvalue()
    sys.stdout = sys.__stdout__
    assert log.getvalue() == expected_log
    assert output == expected_output
    logging.root.handlers.clear()


def run():
    bench_2to3()


if __name__ == "__main__":
    bench_2to3()
