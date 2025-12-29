main() {
  auto v[5], i;

  v[0] = 1;
  v[1] = 2;
  v[2] = 3;
  v[3] = 0;
  v[4] = 1;
  v[5] = 2;

  i = -1;
  while (v[++i]);

  printf("i..%d*n", i);
}