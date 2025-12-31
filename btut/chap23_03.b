main() {
  auto a[10];

  strcopy(a, "hello");

  printf("a..%s*n", a);
}

strcopy(s1, s2) {
  auto i;
  i = 0;
  while (lchar(s1,i,char(s2,i)) != '*e') i++;
}
