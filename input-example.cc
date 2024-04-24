extern int printf(const char *, ...);

#define ZERO 0
#define UNUSED

namespace AA {
  int __attribute__((noinline)) f(void) {
    return ZERO;
  }
};

int main(int argc, char *argv[])
{
  char hello[] = "Hello, world!\n";
  hello[0] = 'h';
  printf("%s", hello);
  return AA::f();
}
