"""Built-in classes, functions, and constants."""


class object(bootstrap=True):
    def __str__(self):
        return self.__repr__()
