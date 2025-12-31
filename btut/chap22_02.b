main() {
  auto a, b;

  b = &a;
  *b = 10;
  printf("a..%d*n");

  auto x[10];
  x[0] = 10;
  printf("**x..%d*n", *x);
  x[1] = 20;
  printf("**(x+1)..%d*n", *(x+1));
  printf("1[x]..%d*n", 1[x]);
}
