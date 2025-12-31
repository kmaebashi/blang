main() {
  auto a, b;
  a = 10;
  b = 20;
  flip(&a, &b);
  printf("a..%d, b..%d*n");
}

flip(x, y) {
auto t;
 t = *y;
 *y = *x;
 *x = t;
}