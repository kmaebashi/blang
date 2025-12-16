main()
{
  auto a;
  auto b;

  a = 1 + 2 * 3 / (5 % 3) - 2;

  printf("a..%d*n", a);

  /* a == 2*/  
  b = a + a * 3 / (5 % a) - 2;
  printf("b..%d*n", b);

}
