add(a, b)
{
  auto c;

  c = 5;

  return a + b + c;
}

main()
{
  auto ret;

  ret = add(1, 2);

  printf("ret..%d*n", ret);
}
