# Heap profiling

Skybison comes with a "heap profiler" that emits heap dumps in  the Java HProf
format.

## Quickstart

### Dumping
* in python: `_builtins._heap_dump("/tmp/my_dump.hprof")`
* in C++: `heapDump(Thread::current(), "/tmp/my_dump.hprof");`

### Run jhat

```shell
jhat /tmp/my_dump.hprof
```

and open in localhost:7000 in your browser.

## More information

### Motivation

You're probably here because you have a leak, or you're wondering what's
allocating so much memory, or something else entirely. Regardless, we have some
APIs bundled into the runtime that can help you track down the issues.

### Get the data

There's two ways to get the data, depending on what kind of granularity you
need. In Python, there's `_builtins._heap_dump(filename: str) -> NoneType`,
which writes a heap dump to disk at the given filename. In C++, there's
`RawObject heapDump(Thread* thread, const char* filename)`, which is found in
`heap-profiler.h`. Note that this can raise an exception, so handle that case.
The file should appear in your working directory with the name you gave it.

The `.hprof` extension is fairly standard, so a reasonable name might be
`heap.hprof`.

### Get the tool

JHAT is the currently supported tool for browsing heap dumps, as it will work
out-of-the-box without modifications. If you do not have `jhat`, install it
from your preferred package manager.

The following patch will search for Python `weakref:_ref__referent` instead of
`java.lang.ref.WeakReference:referent`:

```diff
--- a/jdk/src/share/classes/com/sun/tools/hat/internal/model/Snapshot.java
+++ b/jdk/src/share/classes/com/sun/tools/hat/internal/model/Snapshot.java
@@ -274,14 +274,14 @@ public class Snapshot {
         heapObjects.putAll(fakeClasses);
         fakeClasses.clear();

-        weakReferenceClass = findClass("java.lang.ref.Reference");
+        weakReferenceClass = findClass("weakref");
         if (weakReferenceClass == null)  {      // JDK 1.1.x
             weakReferenceClass = findClass("sun.misc.Ref");
             referentFieldIndex = 0;
         } else {
             JavaField[] fields = weakReferenceClass.getFieldsForInstance();
             for (int i = 0; i < fields.length; i++) {
-                if ("referent".equals(fields[i].getName())) {
+                if ("_ref__referent".equals(fields[i].getName())) {
                     referentFieldIndex = i;
                     break;
                 }
```

JHAT known issues:

* "Exclude weak references" does not work without above patch

MAT is also partially supported, but needs a patch to be able to load a Skybison
heap dump:

```diff
--- a/plugins/org.eclipse.mat.hprof/src/org/eclipse/mat/hprof/Pass1Parser.java
+++ b/plugins/org.eclipse.mat.hprof/src/org/eclipse/mat/hprof/Pass1Parser.java
@@ -721,13 +721,19 @@ public class Pass1Parser extends AbstractParser

         in.skipBytes((long) size * idSize);
         previousArrayStart = address;
-        previousArrayUncompressedEnd = address + 16 + (long)size * 8;
+        previousArrayUncompressedEnd = address + (long)size * 8;
```

MAT known issues:

* Above patch is required to load any Skybison dump
* You must manually specify `weakref:_ref__referent` in any "Exclude weak
  references" search

### Load up the tool

Run `jhat heap.hprof`. It should output some diagnostics to the console:

```
maple% jhat heap.hprof
Reading from heap.hprof...
Dump file created Mon Apr 27 16:47:11 PDT 2020
Snapshot read, resolving...
Resolving 70397 objects...
Chasing references, expect 14 dots..............
Eliminating duplicate references..............
Snapshot resolved.
Started HTTP server on port 7000
Server is ready.
```

If it outputs anything else that does not roughly fit this pattern (errors,
warnings, etc), this is probably a bug in Skybison.

Visit the webserver at [localhost:7000](http://localhost:7000]). You should see
a landing page with the following options:

#### All classes including platform

This page has links to all classes in the layout table. We don't currently
distinguish between platform and non-platform classes, so this will print every
class that has been created (and not garbage collected) up to the heap dump.

#### Show all members of the rootset

This page has links to all of the object roots, using Java terms. A mapping:

| Java Term                 | Skybison Term            |
|---------------------------|--------------------------|
| Native Stack              | Handles (HandleScope)    |
| Java Local/Java Frame     | Python stack             |
| Thread Block              | Object on Thread object  |
| JNI Global                | ApiHandle*/PyObject*     |
| Thread Object             | Thread                   |
| System Class/Sticky Class | Layout                   |
| Unknown                   | Object on Runtime object |
| JNI Local                 | N/A                      |
| Monitor Used              | N/A                      |

You can use this page to manually traverse the live objects starting from the
rootset.

#### Show instance counts for all classes (including platform)

TODO: document

#### Show instance counts for all classes (excluding platform)

TODO: document

#### Show heap histogram

TODO: document

#### Show finalizer summary

TODO: document

#### Execute Object Query Language (OQL) query

TODO: document
