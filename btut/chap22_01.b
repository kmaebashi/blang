main() {
  auto u[10], v[10], i;

/*
  printf("v..%d*n", v);
  printf("i..%d*n", &i);
*/
  i = 0;
  while (i <= 10) {
/*    printf("i..%d*n", i);*/
    v[i] = i;
    i++;
  }

  u = v;
}