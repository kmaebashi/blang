main() {
  auto a;

  a = 1;
  a = a | 2;
  printf("a..%d*n", a);
  a = a & 2;
  printf("a..%d*n", a);
  a = a << 3;
  printf("a..%d*n", a);
  a = a >> 3;
  printf("a..%d*n", a);
  a = ~a;
  printf("a..%d*n", a);

  a = 10;
  a = a ^ 15;
  printf("a..%d*n", a);
}
