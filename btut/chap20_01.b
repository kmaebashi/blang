main() {
  auto v[10], i, sum, n;

  i = 0;
  while (i <= 10) {
    printf("i..%d*n", i);
    v[i] = i;
    i++;
  }
  sum = 0;
  i = 0;
  n = 10;
  while (i<=n) sum =+ v[i++];
}
