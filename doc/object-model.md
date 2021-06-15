# Object model

The Object Model is the main difference that allows Skybison to be faster than
CPython. In CPython, every object is a heap-allocated `PyObject*`  -- and to
speed this up, they use [free lists](https://en.wikipedia.org/wiki/Free_list)
to avoid frequent `malloc`/`free` and have some static objects for small integers.
(You can and should read more about the [CPython Data
Model](https://docs.python.org/3/reference/datamodel.html) when you get a
chance).

Skybison is making this more efficient by having immediate types. All managed
objects are represented by `RawObject`, which is  a piece of memory of word
length. It can either directly include the actual data (*immediate objects*),
or point somewhere on the heap where the data is located (*heap-allocated
objects*).

> NOTE: This document was written during early project planning stages. Parts
> of it may be out of date.

## Object tagging

The least significant bits of `uword` RawObject are used as a tag to determines
the value type. At the conceptual level, we have two stages for resolving tags:
a 3-bit low tag and a 5-bit high tag. This separation is only conceptual; in
the code, we check against the full 5-bit tags using prefix masks.

### 3-bit Tags

* `**0` -- Small integers -- all bits except the last are used for the actual
  value
* `001` -- Heap-allocated objects -- the value of RawObject minus 1 is the actual
  address
* `011` -- Header (a special internal type, see below)
* `101` -- Small sequence types and Error -- escape into the 8-bit tag
* `111` -- Immediate objects -- escape into the 8-bit tag

### 5-bit Tags (101)

* `00101` -- Small bytes -- the 3 next least significant bits are used for the
  length -- the remaining bytes of the value are used to store the actual bytes
  in little endian order
* `01101` -- Small string --  the 3 next least significant bits are used for the
  length -- the remaining bytes of the value are used to store the actual string
  in little endian order
* `10101` -- Error -- the 3 next least significant bits store a tag indicating
  the kind of Error. See `ErrorKind` in `runtime/objects.h`. Remaining bits are
  all `0`.

### 5-bit Tags (111)

* `00111` -- Bool -- the next least significant bit is the value, true/false --
  remaining bits are all `0`
* `01111` -- NotImplemented -- remaining bits are all `0`
* `10111` -- Unbound -- remaining bits are all `0`
* `11111` -- None -- remaining bits are all `1` -- this is so we can
  `memset(obj.address(), -1, sizeof(obj))` to fill a heap-allocated object with
  `None`

With this design, many frequent objects are stored as immediate values and many
operations don't need the extra indirection.

Why this tagging scheme? Having small integers tagged with an extra zero at the
end makes it possible to use many arithmetic instructions without additional
work. To sum up two small integers, you just add the two RawObjects and the
result is automatically the right small integer -- you just need to check for
overflow. For heap-allocated objects, memory address always ends with `00`
because of default memory alignment, so we don't lose any bits by adding this
tag. Dereferencing does not have any extra overhead because all modern CPUs can
support dereferencing with offset.

## Heap-allocated objects

The first word of every heap-allocated object contains the *Header* - a special
structure with metadata about the object. Headers are tagged in the same way as
other objects, which makes it easier for the GC to identify the start of an
object when scanning the heap. The metadata includes type identification
(layout id) and object hash.

The rest of the allocated memory are attributes. A few object have immediate
attributes (`Float` has an attribute of type `double`, `LargeStr` contains a
`byte` array), but most attributes are simply other `RawObject`s.

While small integers and small strings can be stored as immediate objects,
heap-allocated counterparts are needed for larger values. `LargeStr` is
basically a byte array with a header; `LargeInt` is an array of digits of word
length, representing the number in two's-complement form (again, with a
header). Special abstract types `Int` and `Str` are needed to hide this
implementation detail -- managed code needs to work with normal `int` and `str`
types.

## Attribute access

TODO: document

## Subtypes

TODO: document
