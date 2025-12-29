main() {
  auto a, b, t;

  a = 3; b = 5;
  if (a < b) {
    t = a;
    a = b;
    b = t;
  }
  printf("a..%d, b..%d*n", a, b);
}

