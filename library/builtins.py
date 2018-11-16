"""Built-in classes, functions, and constants."""


class object(bootstrap=True):
    def __str__(self):
        return self.__repr__()


class ImportError(bootstrap=True):
    def __init__(self, *args, name=None, path=None, **kwargs):
        if len(kwargs) > 0:
            raise TypeError("invalid keyword arguments supplied")
        # TODO(mpage): Call super once we have EX calling working for built-in methods
        self.args = args
        if len(args) == 1:
            self.msg = args[0]
        self.name = name
        self.path = path


class tuple(bootstrap=True):
    def __repr__(self):
        num_elems = len(self)
        if num_elems == 1:
            return f"({repr(self[0])},)"
        output = "("
        i = 0
        while i < num_elems:
            if i != 0:
                output += ", "
            output += repr(self[i])
            i += 1
        return output + ")"
