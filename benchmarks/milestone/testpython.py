import utest


def raises():
    raise Exception("Testing 123")


class HelloWorldTests(utest.TestCase):
    def test_hello(self):
        self.assertRaises(Exception, raises)

if __name__ == '__main__':
    utest.main()
