
/* Errno module */

#include "Python.h"

/* Windows socket errors (WSA*)  */
#ifdef MS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
/* The following constants were added to errno.h in VS2010 but have
   preferred WSA equivalents. */
#undef EADDRINUSE
#undef EADDRNOTAVAIL
#undef EAFNOSUPPORT
#undef EALREADY
#undef ECONNABORTED
#undef ECONNREFUSED
#undef ECONNRESET
#undef EDESTADDRREQ
#undef EHOSTUNREACH
#undef EINPROGRESS
#undef EISCONN
#undef ELOOP
#undef EMSGSIZE
#undef ENETDOWN
#undef ENETRESET
#undef ENETUNREACH
#undef ENOBUFS
#undef ENOPROTOOPT
#undef ENOTCONN
#undef ENOTSOCK
#undef EOPNOTSUPP
#undef EPROTONOSUPPORT
#undef EPROTOTYPE
#undef ETIMEDOUT
#undef EWOULDBLOCK
#endif

/*
 * Pull in the system error definitions
 */

static PyMethodDef errno_methods[] = {
    {NULL,              NULL}
};

/* Helper function doing the dictionary inserting */

static void
_inscode(PyObject *m, PyObject *de, const char *name, int code)
{
    PyObject *u = PyUnicode_FromString(name);
    PyObject *v = PyLong_FromLong((long) code);

    /* Don't bother checking for errors; they'll be caught at the end
     * of the module initialization function by the caller of
     * initerrno().
     */
    if (u && v) {
        /* insert in modules dict */
        Py_INCREF(v);
        PyModule_AddObject(m, name, v);
        /* insert in errorcode dict */
        PyDict_SetItem(de, v, u);
    }
    Py_XDECREF(u);
    Py_XDECREF(v);
}

PyDoc_STRVAR(errno__doc__,
"This module makes available standard errno system symbols.\n\
\n\
The value of each symbol is the corresponding integer value,\n\
e.g., on most systems, errno.ENOENT equals the integer 2.\n\
\n\
The dictionary errno.errorcode maps numeric codes to symbol names,\n\
e.g., errno.errorcode[2] could be the string 'ENOENT'.\n\
\n\
Symbols that are not relevant to the underlying system are not defined.\n\
\n\
To map error codes to error messages, use the function os.strerror(),\n\
e.g. os.strerror(2) could return 'No such file or directory'.");

static struct PyModuleDef errnomodule = {
    PyModuleDef_HEAD_INIT,
    "errno",
    errno__doc__,
    -1,
    errno_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_errno(void)
{
    PyObject *m, *de;

    m = PyState_FindModule(&errnomodule);
    if (m != NULL) {
        Py_INCREF(m);
        return m;
    }

    m = PyModule_Create(&errnomodule);
    if (m == NULL)
        return NULL;
    de = PyDict_New();
    if (!de)
        return NULL;
    Py_INCREF(de);
    if (PyModule_AddObject(m, "errorcode", de) < 0)
        return NULL;

/* Macro so I don't have to edit each and every line below... */
#define inscode(m, ds, de, name, code, comment) _inscode(m, de, name, code)

    /*
     * The names and comments are borrowed from linux/include/errno.h,
     * which should be pretty all-inclusive.  However, the Solaris specific
     * names and comments are borrowed from sys/errno.h in Solaris.
     * MacOSX specific names and comments are borrowed from sys/errno.h in
     * MacOSX.
     */

#ifdef ENODEV
    inscode(m, ds, de, "ENODEV", ENODEV, "No such device");
#endif
#ifdef ENOCSI
    inscode(m, ds, de, "ENOCSI", ENOCSI, "No CSI structure available");
#endif
#ifdef EHOSTUNREACH
    inscode(m, ds, de, "EHOSTUNREACH", EHOSTUNREACH, "No route to host");
#else
#ifdef WSAEHOSTUNREACH
    inscode(m, ds, de, "EHOSTUNREACH", WSAEHOSTUNREACH, "No route to host");
#endif
#endif
#ifdef ENOMSG
    inscode(m, ds, de, "ENOMSG", ENOMSG, "No message of desired type");
#endif
#ifdef EUCLEAN
    inscode(m, ds, de, "EUCLEAN", EUCLEAN, "Structure needs cleaning");
#endif
#ifdef EL2NSYNC
    inscode(m, ds, de, "EL2NSYNC", EL2NSYNC, "Level 2 not synchronized");
#endif
#ifdef EL2HLT
    inscode(m, ds, de, "EL2HLT", EL2HLT, "Level 2 halted");
#endif
#ifdef ENODATA
    inscode(m, ds, de, "ENODATA", ENODATA, "No data available");
#endif
#ifdef ENOTBLK
    inscode(m, ds, de, "ENOTBLK", ENOTBLK, "Block device required");
#endif
#ifdef ENOSYS
    inscode(m, ds, de, "ENOSYS", ENOSYS, "Function not implemented");
#endif
#ifdef EPIPE
    inscode(m, ds, de, "EPIPE", EPIPE, "Broken pipe");
#endif
#ifdef EINVAL
    inscode(m, ds, de, "EINVAL", EINVAL, "Invalid argument");
#else
#ifdef WSAEINVAL
    inscode(m, ds, de, "EINVAL", WSAEINVAL, "Invalid argument");
#endif
#endif
#ifdef EOVERFLOW
    inscode(m, ds, de, "EOVERFLOW", EOVERFLOW, "Value too large for defined data type");
#endif
#ifdef EADV
    inscode(m, ds, de, "EADV", EADV, "Advertise error");
#endif
#ifdef EINTR
    inscode(m, ds, de, "EINTR", EINTR, "Interrupted system call");
#else
#ifdef WSAEINTR
    inscode(m, ds, de, "EINTR", WSAEINTR, "Interrupted system call");
#endif
#endif
#ifdef EUSERS
    inscode(m, ds, de, "EUSERS", EUSERS, "Too many users");
#else
#ifdef WSAEUSERS
    inscode(m, ds, de, "EUSERS", WSAEUSERS, "Too many users");
#endif
#endif
#ifdef ENOTEMPTY
    inscode(m, ds, de, "ENOTEMPTY", ENOTEMPTY, "Directory not empty");
#else
#ifdef WSAENOTEMPTY
    inscode(m, ds, de, "ENOTEMPTY", WSAENOTEMPTY, "Directory not empty");
#endif
#endif
#ifdef ENOBUFS
    inscode(m, ds, de, "ENOBUFS", ENOBUFS, "No buffer space available");
#else
#ifdef WSAENOBUFS
    inscode(m, ds, de, "ENOBUFS", WSAENOBUFS, "No buffer space available");
#endif
#endif
#ifdef EPROTO
    inscode(m, ds, de, "EPROTO", EPROTO, "Protocol error");
#endif
#ifdef EREMOTE
    inscode(m, ds, de, "EREMOTE", EREMOTE, "Object is remote");
#else
#ifdef WSAEREMOTE
    inscode(m, ds, de, "EREMOTE", WSAEREMOTE, "Object is remote");
#endif
#endif
#ifdef ENAVAIL
    inscode(m, ds, de, "ENAVAIL", ENAVAIL, "No XENIX semaphores available");
#endif
#ifdef ECHILD
    inscode(m, ds, de, "ECHILD", ECHILD, "No child processes");
#endif
#ifdef ELOOP
    inscode(m, ds, de, "ELOOP", ELOOP, "Too many symbolic links encountered");
#else
#ifdef WSAELOOP
    inscode(m, ds, de, "ELOOP", WSAELOOP, "Too many symbolic links encountered");
#endif
#endif
#ifdef EXDEV
    inscode(m, ds, de, "EXDEV", EXDEV, "Cross-device link");
#endif
#ifdef E2BIG
    inscode(m, ds, de, "E2BIG", E2BIG, "Arg list too long");
#endif
#ifdef ESRCH
    inscode(m, ds, de, "ESRCH", ESRCH, "No such process");
#endif
#ifdef EMSGSIZE
    inscode(m, ds, de, "EMSGSIZE", EMSGSIZE, "Message too long");
#else
#ifdef WSAEMSGSIZE
    inscode(m, ds, de, "EMSGSIZE", WSAEMSGSIZE, "Message too long");
#endif
#endif
#ifdef EAFNOSUPPORT
    inscode(m, ds, de, "EAFNOSUPPORT", EAFNOSUPPORT, "Address family not supported by protocol");
#else
#ifdef WSAEAFNOSUPPORT
    inscode(m, ds, de, "EAFNOSUPPORT", WSAEAFNOSUPPORT, "Address family not supported by protocol");
#endif
#endif
#ifdef EBADR
    inscode(m, ds, de, "EBADR", EBADR, "Invalid request descriptor");
#endif
#ifdef EHOSTDOWN
    inscode(m, ds, de, "EHOSTDOWN", EHOSTDOWN, "Host is down");
#else
#ifdef WSAEHOSTDOWN
    inscode(m, ds, de, "EHOSTDOWN", WSAEHOSTDOWN, "Host is down");
#endif
#endif
#ifdef EPFNOSUPPORT
    inscode(m, ds, de, "EPFNOSUPPORT", EPFNOSUPPORT, "Protocol family not supported");
#else
#ifdef WSAEPFNOSUPPORT
    inscode(m, ds, de, "EPFNOSUPPORT", WSAEPFNOSUPPORT, "Protocol family not supported");
#endif
#endif
#ifdef ENOPROTOOPT
    inscode(m, ds, de, "ENOPROTOOPT", ENOPROTOOPT, "Protocol not available");
#else
#ifdef WSAENOPROTOOPT
    inscode(m, ds, de, "ENOPROTOOPT", WSAENOPROTOOPT, "Protocol not available");
#endif
#endif
#ifdef EBUSY
    inscode(m, ds, de, "EBUSY", EBUSY, "Device or resource busy");
#endif
#ifdef EWOULDBLOCK
    inscode(m, ds, de, "EWOULDBLOCK", EWOULDBLOCK, "Operation would block");
#else
#ifdef WSAEWOULDBLOCK
    inscode(m, ds, de, "EWOULDBLOCK", WSAEWOULDBLOCK, "Operation would block");
#endif
#endif
#ifdef EBADFD
    inscode(m, ds, de, "EBADFD", EBADFD, "File descriptor in bad state");
#endif
#ifdef EDOTDOT
    inscode(m, ds, de, "EDOTDOT", EDOTDOT, "RFS specific error");
#endif
#ifdef EISCONN
    inscode(m, ds, de, "EISCONN", EISCONN, "Transport endpoint is already connected");
#else
#ifdef WSAEISCONN
    inscode(m, ds, de, "EISCONN", WSAEISCONN, "Transport endpoint is already connected");
#endif
#endif
#ifdef ENOANO
    inscode(m, ds, de, "ENOANO", ENOANO, "No anode");
#endif
#ifdef ESHUTDOWN
    inscode(m, ds, de, "ESHUTDOWN", ESHUTDOWN, "Cannot send after transport endpoint shutdown");
#else
#ifdef WSAESHUTDOWN
    inscode(m, ds, de, "ESHUTDOWN", WSAESHUTDOWN, "Cannot send after transport endpoint shutdown");
#endif
#endif
#ifdef ECHRNG
    inscode(m, ds, de, "ECHRNG", ECHRNG, "Channel number out of range");
#endif
#ifdef ELIBBAD
    inscode(m, ds, de, "ELIBBAD", ELIBBAD, "Accessing a corrupted shared library");
#endif
#ifdef ENONET
    inscode(m, ds, de, "ENONET", ENONET, "Machine is not on the network");
#endif
#ifdef EBADE
    inscode(m, ds, de, "EBADE", EBADE, "Invalid exchange");
#endif
#ifdef EBADF
    inscode(m, ds, de, "EBADF", EBADF, "Bad file number");
#else
#ifdef WSAEBADF
    inscode(m, ds, de, "EBADF", WSAEBADF, "Bad file number");
#endif
#endif
#ifdef EMULTIHOP
    inscode(m, ds, de, "EMULTIHOP", EMULTIHOP, "Multihop attempted");
#endif
#ifdef EIO
    inscode(m, ds, de, "EIO", EIO, "I/O error");
#endif
#ifdef EUNATCH
    inscode(m, ds, de, "EUNATCH", EUNATCH, "Protocol driver not attached");
#endif
#ifdef EPROTOTYPE
    inscode(m, ds, de, "EPROTOTYPE", EPROTOTYPE, "Protocol wrong type for socket");
#else
#ifdef WSAEPROTOTYPE
    inscode(m, ds, de, "EPROTOTYPE", WSAEPROTOTYPE, "Protocol wrong type for socket");
#endif
#endif
#ifdef ENOSPC
    inscode(m, ds, de, "ENOSPC", ENOSPC, "No space left on device");
#endif
#ifdef ENOEXEC
    inscode(m, ds, de, "ENOEXEC", ENOEXEC, "Exec format error");
#endif
#ifdef EALREADY
    inscode(m, ds, de, "EALREADY", EALREADY, "Operation already in progress");
#else
#ifdef WSAEALREADY
    inscode(m, ds, de, "EALREADY", WSAEALREADY, "Operation already in progress");
#endif
#endif
#ifdef ENETDOWN
    inscode(m, ds, de, "ENETDOWN", ENETDOWN, "Network is down");
#else
#ifdef WSAENETDOWN
    inscode(m, ds, de, "ENETDOWN", WSAENETDOWN, "Network is down");
#endif
#endif
#ifdef ENOTNAM
    inscode(m, ds, de, "ENOTNAM", ENOTNAM, "Not a XENIX named type file");
#endif
#ifdef EACCES
    inscode(m, ds, de, "EACCES", EACCES, "Permission denied");
#else
#ifdef WSAEACCES
    inscode(m, ds, de, "EACCES", WSAEACCES, "Permission denied");
#endif
#endif
#ifdef ELNRNG
    inscode(m, ds, de, "ELNRNG", ELNRNG, "Link number out of range");
#endif
#ifdef EILSEQ
    inscode(m, ds, de, "EILSEQ", EILSEQ, "Illegal byte sequence");
#endif
#ifdef ENOTDIR
    inscode(m, ds, de, "ENOTDIR", ENOTDIR, "Not a directory");
#endif
#ifdef ENOTUNIQ
    inscode(m, ds, de, "ENOTUNIQ", ENOTUNIQ, "Name not unique on network");
#endif
#ifdef EPERM
    inscode(m, ds, de, "EPERM", EPERM, "Operation not permitted");
#endif
#ifdef EDOM
    inscode(m, ds, de, "EDOM", EDOM, "Math argument out of domain of func");
#endif
#ifdef EXFULL
    inscode(m, ds, de, "EXFULL", EXFULL, "Exchange full");
#endif
#ifdef ECONNREFUSED
    inscode(m, ds, de, "ECONNREFUSED", ECONNREFUSED, "Connection refused");
#else
#ifdef WSAECONNREFUSED
    inscode(m, ds, de, "ECONNREFUSED", WSAECONNREFUSED, "Connection refused");
#endif
#endif
#ifdef EISDIR
    inscode(m, ds, de, "EISDIR", EISDIR, "Is a directory");
#endif
#ifdef EPROTONOSUPPORT
    inscode(m, ds, de, "EPROTONOSUPPORT", EPROTONOSUPPORT, "Protocol not supported");
#else
#ifdef WSAEPROTONOSUPPORT
    inscode(m, ds, de, "EPROTONOSUPPORT", WSAEPROTONOSUPPORT, "Protocol not supported");
#endif
#endif
#ifdef EROFS
    inscode(m, ds, de, "EROFS", EROFS, "Read-only file system");
#endif
#ifdef EADDRNOTAVAIL
    inscode(m, ds, de, "EADDRNOTAVAIL", EADDRNOTAVAIL, "Cannot assign requested address");
#else
#ifdef WSAEADDRNOTAVAIL
    inscode(m, ds, de, "EADDRNOTAVAIL", WSAEADDRNOTAVAIL, "Cannot assign requested address");
#endif
#endif
#ifdef EIDRM
    inscode(m, ds, de, "EIDRM", EIDRM, "Identifier removed");
#endif
#ifdef ECOMM
    inscode(m, ds, de, "ECOMM", ECOMM, "Communication error on send");
#endif
#ifdef ESRMNT
    inscode(m, ds, de, "ESRMNT", ESRMNT, "Srmount error");
#endif
#ifdef EREMOTEIO
    inscode(m, ds, de, "EREMOTEIO", EREMOTEIO, "Remote I/O error");
#endif
#ifdef EL3RST
    inscode(m, ds, de, "EL3RST", EL3RST, "Level 3 reset");
#endif
#ifdef EBADMSG
    inscode(m, ds, de, "EBADMSG", EBADMSG, "Not a data message");
#endif
#ifdef ENFILE
    inscode(m, ds, de, "ENFILE", ENFILE, "File table overflow");
#endif
#ifdef ELIBMAX
    inscode(m, ds, de, "ELIBMAX", ELIBMAX, "Attempting to link in too many shared libraries");
#endif
#ifdef ESPIPE
    inscode(m, ds, de, "ESPIPE", ESPIPE, "Illegal seek");
#endif
#ifdef ENOLINK
    inscode(m, ds, de, "ENOLINK", ENOLINK, "Link has been severed");
#endif
#ifdef ENETRESET
    inscode(m, ds, de, "ENETRESET", ENETRESET, "Network dropped connection because of reset");
#else
#ifdef WSAENETRESET
    inscode(m, ds, de, "ENETRESET", WSAENETRESET, "Network dropped connection because of reset");
#endif
#endif
#ifdef ETIMEDOUT
    inscode(m, ds, de, "ETIMEDOUT", ETIMEDOUT, "Connection timed out");
#else
#ifdef WSAETIMEDOUT
    inscode(m, ds, de, "ETIMEDOUT", WSAETIMEDOUT, "Connection timed out");
#endif
#endif
#ifdef ENOENT
    inscode(m, ds, de, "ENOENT", ENOENT, "No such file or directory");
#endif
#ifdef EEXIST
    inscode(m, ds, de, "EEXIST", EEXIST, "File exists");
#endif
#ifdef EDQUOT
    inscode(m, ds, de, "EDQUOT", EDQUOT, "Quota exceeded");
#else
#ifdef WSAEDQUOT
    inscode(m, ds, de, "EDQUOT", WSAEDQUOT, "Quota exceeded");
#endif
#endif
#ifdef ENOSTR
    inscode(m, ds, de, "ENOSTR", ENOSTR, "Device not a stream");
#endif
#ifdef EBADSLT
    inscode(m, ds, de, "EBADSLT", EBADSLT, "Invalid slot");
#endif
#ifdef EBADRQC
    inscode(m, ds, de, "EBADRQC", EBADRQC, "Invalid request code");
#endif
#ifdef ELIBACC
    inscode(m, ds, de, "ELIBACC", ELIBACC, "Can not access a needed shared library");
#endif
#ifdef EFAULT
    inscode(m, ds, de, "EFAULT", EFAULT, "Bad address");
#else
#ifdef WSAEFAULT
    inscode(m, ds, de, "EFAULT", WSAEFAULT, "Bad address");
#endif
#endif
#ifdef EFBIG
    inscode(m, ds, de, "EFBIG", EFBIG, "File too large");
#endif
#ifdef EDEADLK
    inscode(m, ds, de, "EDEADLK", EDEADLK, "Resource deadlock would occur");
#endif
#ifdef ENOTCONN
    inscode(m, ds, de, "ENOTCONN", ENOTCONN, "Transport endpoint is not connected");
#else
#ifdef WSAENOTCONN
    inscode(m, ds, de, "ENOTCONN", WSAENOTCONN, "Transport endpoint is not connected");
#endif
#endif
#ifdef EDESTADDRREQ
    inscode(m, ds, de, "EDESTADDRREQ", EDESTADDRREQ, "Destination address required");
#else
#ifdef WSAEDESTADDRREQ
    inscode(m, ds, de, "EDESTADDRREQ", WSAEDESTADDRREQ, "Destination address required");
#endif
#endif
#ifdef ELIBSCN
    inscode(m, ds, de, "ELIBSCN", ELIBSCN, ".lib section in a.out corrupted");
#endif
#ifdef ENOLCK
    inscode(m, ds, de, "ENOLCK", ENOLCK, "No record locks available");
#endif
#ifdef EISNAM
    inscode(m, ds, de, "EISNAM", EISNAM, "Is a named type file");
#endif
#ifdef ECONNABORTED
    inscode(m, ds, de, "ECONNABORTED", ECONNABORTED, "Software caused connection abort");
#else
#ifdef WSAECONNABORTED
    inscode(m, ds, de, "ECONNABORTED", WSAECONNABORTED, "Software caused connection abort");
#endif
#endif
#ifdef ENETUNREACH
    inscode(m, ds, de, "ENETUNREACH", ENETUNREACH, "Network is unreachable");
#else
#ifdef WSAENETUNREACH
    inscode(m, ds, de, "ENETUNREACH", WSAENETUNREACH, "Network is unreachable");
#endif
#endif
#ifdef ESTALE
    inscode(m, ds, de, "ESTALE", ESTALE, "Stale NFS file handle");
#else
#ifdef WSAESTALE
    inscode(m, ds, de, "ESTALE", WSAESTALE, "Stale NFS file handle");
#endif
#endif
#ifdef ENOSR
    inscode(m, ds, de, "ENOSR", ENOSR, "Out of streams resources");
#endif
#ifdef ENOMEM
    inscode(m, ds, de, "ENOMEM", ENOMEM, "Out of memory");
#endif
#ifdef ENOTSOCK
    inscode(m, ds, de, "ENOTSOCK", ENOTSOCK, "Socket operation on non-socket");
#else
#ifdef WSAENOTSOCK
    inscode(m, ds, de, "ENOTSOCK", WSAENOTSOCK, "Socket operation on non-socket");
#endif
#endif
#ifdef ESTRPIPE
    inscode(m, ds, de, "ESTRPIPE", ESTRPIPE, "Streams pipe error");
#endif
#ifdef EMLINK
    inscode(m, ds, de, "EMLINK", EMLINK, "Too many links");
#endif
#ifdef ERANGE
    inscode(m, ds, de, "ERANGE", ERANGE, "Math result not representable");
#endif
#ifdef ELIBEXEC
    inscode(m, ds, de, "ELIBEXEC", ELIBEXEC, "Cannot exec a shared library directly");
#endif
#ifdef EL3HLT
    inscode(m, ds, de, "EL3HLT", EL3HLT, "Level 3 halted");
#endif
#ifdef ECONNRESET
    inscode(m, ds, de, "ECONNRESET", ECONNRESET, "Connection reset by peer");
#else
#ifdef WSAECONNRESET
    inscode(m, ds, de, "ECONNRESET", WSAECONNRESET, "Connection reset by peer");
#endif
#endif
#ifdef EADDRINUSE
    inscode(m, ds, de, "EADDRINUSE", EADDRINUSE, "Address already in use");
#else
#ifdef WSAEADDRINUSE
    inscode(m, ds, de, "EADDRINUSE", WSAEADDRINUSE, "Address already in use");
#endif
#endif
#ifdef EOPNOTSUPP
    inscode(m, ds, de, "EOPNOTSUPP", EOPNOTSUPP, "Operation not supported on transport endpoint");
#else
#ifdef WSAEOPNOTSUPP
    inscode(m, ds, de, "EOPNOTSUPP", WSAEOPNOTSUPP, "Operation not supported on transport endpoint");
#endif
#endif
#ifdef EREMCHG
    inscode(m, ds, de, "EREMCHG", EREMCHG, "Remote address changed");
#endif
#ifdef EAGAIN
    inscode(m, ds, de, "EAGAIN", EAGAIN, "Try again");
#endif
#ifdef ENAMETOOLONG
    inscode(m, ds, de, "ENAMETOOLONG", ENAMETOOLONG, "File name too long");
#else
#ifdef WSAENAMETOOLONG
    inscode(m, ds, de, "ENAMETOOLONG", WSAENAMETOOLONG, "File name too long");
#endif
#endif
#ifdef ENOTTY
    inscode(m, ds, de, "ENOTTY", ENOTTY, "Not a typewriter");
#endif
#ifdef ERESTART
    inscode(m, ds, de, "ERESTART", ERESTART, "Interrupted system call should be restarted");
#endif
#ifdef ESOCKTNOSUPPORT
    inscode(m, ds, de, "ESOCKTNOSUPPORT", ESOCKTNOSUPPORT, "Socket type not supported");
#else
#ifdef WSAESOCKTNOSUPPORT
    inscode(m, ds, de, "ESOCKTNOSUPPORT", WSAESOCKTNOSUPPORT, "Socket type not supported");
#endif
#endif
#ifdef ETIME
    inscode(m, ds, de, "ETIME", ETIME, "Timer expired");
#endif
#ifdef EBFONT
    inscode(m, ds, de, "EBFONT", EBFONT, "Bad font file format");
#endif
#ifdef EDEADLOCK
    inscode(m, ds, de, "EDEADLOCK", EDEADLOCK, "Error EDEADLOCK");
#endif
#ifdef ETOOMANYREFS
    inscode(m, ds, de, "ETOOMANYREFS", ETOOMANYREFS, "Too many references: cannot splice");
#else
#ifdef WSAETOOMANYREFS
    inscode(m, ds, de, "ETOOMANYREFS", WSAETOOMANYREFS, "Too many references: cannot splice");
#endif
#endif
#ifdef EMFILE
    inscode(m, ds, de, "EMFILE", EMFILE, "Too many open files");
#else
#ifdef WSAEMFILE
    inscode(m, ds, de, "EMFILE", WSAEMFILE, "Too many open files");
#endif
#endif
#ifdef ETXTBSY
    inscode(m, ds, de, "ETXTBSY", ETXTBSY, "Text file busy");
#endif
#ifdef EINPROGRESS
    inscode(m, ds, de, "EINPROGRESS", EINPROGRESS, "Operation now in progress");
#else
#ifdef WSAEINPROGRESS
    inscode(m, ds, de, "EINPROGRESS", WSAEINPROGRESS, "Operation now in progress");
#endif
#endif
#ifdef ENXIO
    inscode(m, ds, de, "ENXIO", ENXIO, "No such device or address");
#endif
#ifdef ENOPKG
    inscode(m, ds, de, "ENOPKG", ENOPKG, "Package not installed");
#endif
#ifdef WSASY
    inscode(m, ds, de, "WSASY", WSASY, "Error WSASY");
#endif
#ifdef WSAEHOSTDOWN
    inscode(m, ds, de, "WSAEHOSTDOWN", WSAEHOSTDOWN, "Host is down");
#endif
#ifdef WSAENETDOWN
    inscode(m, ds, de, "WSAENETDOWN", WSAENETDOWN, "Network is down");
#endif
#ifdef WSAENOTSOCK
    inscode(m, ds, de, "WSAENOTSOCK", WSAENOTSOCK, "Socket operation on non-socket");
#endif
#ifdef WSAEHOSTUNREACH
    inscode(m, ds, de, "WSAEHOSTUNREACH", WSAEHOSTUNREACH, "No route to host");
#endif
#ifdef WSAELOOP
    inscode(m, ds, de, "WSAELOOP", WSAELOOP, "Too many symbolic links encountered");
#endif
#ifdef WSAEMFILE
    inscode(m, ds, de, "WSAEMFILE", WSAEMFILE, "Too many open files");
#endif
#ifdef WSAESTALE
    inscode(m, ds, de, "WSAESTALE", WSAESTALE, "Stale NFS file handle");
#endif
#ifdef WSAVERNOTSUPPORTED
    inscode(m, ds, de, "WSAVERNOTSUPPORTED", WSAVERNOTSUPPORTED, "Error WSAVERNOTSUPPORTED");
#endif
#ifdef WSAENETUNREACH
    inscode(m, ds, de, "WSAENETUNREACH", WSAENETUNREACH, "Network is unreachable");
#endif
#ifdef WSAEPROCLIM
    inscode(m, ds, de, "WSAEPROCLIM", WSAEPROCLIM, "Error WSAEPROCLIM");
#endif
#ifdef WSAEFAULT
    inscode(m, ds, de, "WSAEFAULT", WSAEFAULT, "Bad address");
#endif
#ifdef WSANOTINITIALISED
    inscode(m, ds, de, "WSANOTINITIALISED", WSANOTINITIALISED, "Error WSANOTINITIALISED");
#endif
#ifdef WSAEUSERS
    inscode(m, ds, de, "WSAEUSERS", WSAEUSERS, "Too many users");
#endif
#ifdef WSAMAKEASYNCREPL
    inscode(m, ds, de, "WSAMAKEASYNCREPL", WSAMAKEASYNCREPL, "Error WSAMAKEASYNCREPL");
#endif
#ifdef WSAENOPROTOOPT
    inscode(m, ds, de, "WSAENOPROTOOPT", WSAENOPROTOOPT, "Protocol not available");
#endif
#ifdef WSAECONNABORTED
    inscode(m, ds, de, "WSAECONNABORTED", WSAECONNABORTED, "Software caused connection abort");
#endif
#ifdef WSAENAMETOOLONG
    inscode(m, ds, de, "WSAENAMETOOLONG", WSAENAMETOOLONG, "File name too long");
#endif
#ifdef WSAENOTEMPTY
    inscode(m, ds, de, "WSAENOTEMPTY", WSAENOTEMPTY, "Directory not empty");
#endif
#ifdef WSAESHUTDOWN
    inscode(m, ds, de, "WSAESHUTDOWN", WSAESHUTDOWN, "Cannot send after transport endpoint shutdown");
#endif
#ifdef WSAEAFNOSUPPORT
    inscode(m, ds, de, "WSAEAFNOSUPPORT", WSAEAFNOSUPPORT, "Address family not supported by protocol");
#endif
#ifdef WSAETOOMANYREFS
    inscode(m, ds, de, "WSAETOOMANYREFS", WSAETOOMANYREFS, "Too many references: cannot splice");
#endif
#ifdef WSAEACCES
    inscode(m, ds, de, "WSAEACCES", WSAEACCES, "Permission denied");
#endif
#ifdef WSATR
    inscode(m, ds, de, "WSATR", WSATR, "Error WSATR");
#endif
#ifdef WSABASEERR
    inscode(m, ds, de, "WSABASEERR", WSABASEERR, "Error WSABASEERR");
#endif
#ifdef WSADESCRIPTIO
    inscode(m, ds, de, "WSADESCRIPTIO", WSADESCRIPTIO, "Error WSADESCRIPTIO");
#endif
#ifdef WSAEMSGSIZE
    inscode(m, ds, de, "WSAEMSGSIZE", WSAEMSGSIZE, "Message too long");
#endif
#ifdef WSAEBADF
    inscode(m, ds, de, "WSAEBADF", WSAEBADF, "Bad file number");
#endif
#ifdef WSAECONNRESET
    inscode(m, ds, de, "WSAECONNRESET", WSAECONNRESET, "Connection reset by peer");
#endif
#ifdef WSAGETSELECTERRO
    inscode(m, ds, de, "WSAGETSELECTERRO", WSAGETSELECTERRO, "Error WSAGETSELECTERRO");
#endif
#ifdef WSAETIMEDOUT
    inscode(m, ds, de, "WSAETIMEDOUT", WSAETIMEDOUT, "Connection timed out");
#endif
#ifdef WSAENOBUFS
    inscode(m, ds, de, "WSAENOBUFS", WSAENOBUFS, "No buffer space available");
#endif
#ifdef WSAEDISCON
    inscode(m, ds, de, "WSAEDISCON", WSAEDISCON, "Error WSAEDISCON");
#endif
#ifdef WSAEINTR
    inscode(m, ds, de, "WSAEINTR", WSAEINTR, "Interrupted system call");
#endif
#ifdef WSAEPROTOTYPE
    inscode(m, ds, de, "WSAEPROTOTYPE", WSAEPROTOTYPE, "Protocol wrong type for socket");
#endif
#ifdef WSAHOS
    inscode(m, ds, de, "WSAHOS", WSAHOS, "Error WSAHOS");
#endif
#ifdef WSAEADDRINUSE
    inscode(m, ds, de, "WSAEADDRINUSE", WSAEADDRINUSE, "Address already in use");
#endif
#ifdef WSAEADDRNOTAVAIL
    inscode(m, ds, de, "WSAEADDRNOTAVAIL", WSAEADDRNOTAVAIL, "Cannot assign requested address");
#endif
#ifdef WSAEALREADY
    inscode(m, ds, de, "WSAEALREADY", WSAEALREADY, "Operation already in progress");
#endif
#ifdef WSAEPROTONOSUPPORT
    inscode(m, ds, de, "WSAEPROTONOSUPPORT", WSAEPROTONOSUPPORT, "Protocol not supported");
#endif
#ifdef WSASYSNOTREADY
    inscode(m, ds, de, "WSASYSNOTREADY", WSASYSNOTREADY, "Error WSASYSNOTREADY");
#endif
#ifdef WSAEWOULDBLOCK
    inscode(m, ds, de, "WSAEWOULDBLOCK", WSAEWOULDBLOCK, "Operation would block");
#endif
#ifdef WSAEPFNOSUPPORT
    inscode(m, ds, de, "WSAEPFNOSUPPORT", WSAEPFNOSUPPORT, "Protocol family not supported");
#endif
#ifdef WSAEOPNOTSUPP
    inscode(m, ds, de, "WSAEOPNOTSUPP", WSAEOPNOTSUPP, "Operation not supported on transport endpoint");
#endif
#ifdef WSAEISCONN
    inscode(m, ds, de, "WSAEISCONN", WSAEISCONN, "Transport endpoint is already connected");
#endif
#ifdef WSAEDQUOT
    inscode(m, ds, de, "WSAEDQUOT", WSAEDQUOT, "Quota exceeded");
#endif
#ifdef WSAENOTCONN
    inscode(m, ds, de, "WSAENOTCONN", WSAENOTCONN, "Transport endpoint is not connected");
#endif
#ifdef WSAEREMOTE
    inscode(m, ds, de, "WSAEREMOTE", WSAEREMOTE, "Object is remote");
#endif
#ifdef WSAEINVAL
    inscode(m, ds, de, "WSAEINVAL", WSAEINVAL, "Invalid argument");
#endif
#ifdef WSAEINPROGRESS
    inscode(m, ds, de, "WSAEINPROGRESS", WSAEINPROGRESS, "Operation now in progress");
#endif
#ifdef WSAGETSELECTEVEN
    inscode(m, ds, de, "WSAGETSELECTEVEN", WSAGETSELECTEVEN, "Error WSAGETSELECTEVEN");
#endif
#ifdef WSAESOCKTNOSUPPORT
    inscode(m, ds, de, "WSAESOCKTNOSUPPORT", WSAESOCKTNOSUPPORT, "Socket type not supported");
#endif
#ifdef WSAGETASYNCERRO
    inscode(m, ds, de, "WSAGETASYNCERRO", WSAGETASYNCERRO, "Error WSAGETASYNCERRO");
#endif
#ifdef WSAMAKESELECTREPL
    inscode(m, ds, de, "WSAMAKESELECTREPL", WSAMAKESELECTREPL, "Error WSAMAKESELECTREPL");
#endif
#ifdef WSAGETASYNCBUFLE
    inscode(m, ds, de, "WSAGETASYNCBUFLE", WSAGETASYNCBUFLE, "Error WSAGETASYNCBUFLE");
#endif
#ifdef WSAEDESTADDRREQ
    inscode(m, ds, de, "WSAEDESTADDRREQ", WSAEDESTADDRREQ, "Destination address required");
#endif
#ifdef WSAECONNREFUSED
    inscode(m, ds, de, "WSAECONNREFUSED", WSAECONNREFUSED, "Connection refused");
#endif
#ifdef WSAENETRESET
    inscode(m, ds, de, "WSAENETRESET", WSAENETRESET, "Network dropped connection because of reset");
#endif
#ifdef WSAN
    inscode(m, ds, de, "WSAN", WSAN, "Error WSAN");
#endif
#ifdef ENOMEDIUM
    inscode(m, ds, de, "ENOMEDIUM", ENOMEDIUM, "No medium found");
#endif
#ifdef EMEDIUMTYPE
    inscode(m, ds, de, "EMEDIUMTYPE", EMEDIUMTYPE, "Wrong medium type");
#endif
#ifdef ECANCELED
    inscode(m, ds, de, "ECANCELED", ECANCELED, "Operation Canceled");
#endif
#ifdef ENOKEY
    inscode(m, ds, de, "ENOKEY", ENOKEY, "Required key not available");
#endif
#ifdef EKEYEXPIRED
    inscode(m, ds, de, "EKEYEXPIRED", EKEYEXPIRED, "Key has expired");
#endif
#ifdef EKEYREVOKED
    inscode(m, ds, de, "EKEYREVOKED", EKEYREVOKED, "Key has been revoked");
#endif
#ifdef EKEYREJECTED
    inscode(m, ds, de, "EKEYREJECTED", EKEYREJECTED, "Key was rejected by service");
#endif
#ifdef EOWNERDEAD
    inscode(m, ds, de, "EOWNERDEAD", EOWNERDEAD, "Owner died");
#endif
#ifdef ENOTRECOVERABLE
    inscode(m, ds, de, "ENOTRECOVERABLE", ENOTRECOVERABLE, "State not recoverable");
#endif
#ifdef ERFKILL
    inscode(m, ds, de, "ERFKILL", ERFKILL, "Operation not possible due to RF-kill");
#endif

    /* Solaris-specific errnos */
#ifdef ECANCELED
    inscode(m, ds, de, "ECANCELED", ECANCELED, "Operation canceled");
#endif
#ifdef ENOTSUP
    inscode(m, ds, de, "ENOTSUP", ENOTSUP, "Operation not supported");
#endif
#ifdef EOWNERDEAD
    inscode(m, ds, de, "EOWNERDEAD", EOWNERDEAD, "Process died with the lock");
#endif
#ifdef ENOTRECOVERABLE
    inscode(m, ds, de, "ENOTRECOVERABLE", ENOTRECOVERABLE, "Lock is not recoverable");
#endif
#ifdef ELOCKUNMAPPED
    inscode(m, ds, de, "ELOCKUNMAPPED", ELOCKUNMAPPED, "Locked lock was unmapped");
#endif
#ifdef ENOTACTIVE
    inscode(m, ds, de, "ENOTACTIVE", ENOTACTIVE, "Facility is not active");
#endif

    /* MacOSX specific errnos */
#ifdef EAUTH
    inscode(m, ds, de, "EAUTH", EAUTH, "Authentication error");
#endif
#ifdef EBADARCH
    inscode(m, ds, de, "EBADARCH", EBADARCH, "Bad CPU type in executable");
#endif
#ifdef EBADEXEC
    inscode(m, ds, de, "EBADEXEC", EBADEXEC, "Bad executable (or shared library)");
#endif
#ifdef EBADMACHO
    inscode(m, ds, de, "EBADMACHO", EBADMACHO, "Malformed Mach-o file");
#endif
#ifdef EBADRPC
    inscode(m, ds, de, "EBADRPC", EBADRPC, "RPC struct is bad");
#endif
#ifdef EDEVERR
    inscode(m, ds, de, "EDEVERR", EDEVERR, "Device error");
#endif
#ifdef EFTYPE
    inscode(m, ds, de, "EFTYPE", EFTYPE, "Inappropriate file type or format");
#endif
#ifdef ENEEDAUTH
    inscode(m, ds, de, "ENEEDAUTH", ENEEDAUTH, "Need authenticator");
#endif
#ifdef ENOATTR
    inscode(m, ds, de, "ENOATTR", ENOATTR, "Attribute not found");
#endif
#ifdef ENOPOLICY
    inscode(m, ds, de, "ENOPOLICY", ENOPOLICY, "Policy not found");
#endif
#ifdef EPROCLIM
    inscode(m, ds, de, "EPROCLIM", EPROCLIM, "Too many processes");
#endif
#ifdef EPROCUNAVAIL
    inscode(m, ds, de, "EPROCUNAVAIL", EPROCUNAVAIL, "Bad procedure for program");
#endif
#ifdef EPROGMISMATCH
    inscode(m, ds, de, "EPROGMISMATCH", EPROGMISMATCH, "Program version wrong");
#endif
#ifdef EPROGUNAVAIL
    inscode(m, ds, de, "EPROGUNAVAIL", EPROGUNAVAIL, "RPC prog. not avail");
#endif
#ifdef EPWROFF
    inscode(m, ds, de, "EPWROFF", EPWROFF, "Device power is off");
#endif
#ifdef ERPCMISMATCH
    inscode(m, ds, de, "ERPCMISMATCH", ERPCMISMATCH, "RPC version wrong");
#endif
#ifdef ESHLIBVERS
    inscode(m, ds, de, "ESHLIBVERS", ESHLIBVERS, "Shared library version mismatch");
#endif

    Py_DECREF(de);
    PyState_AddModule(m, &errnomodule);
    return m;
}
