main()
{
  auto a;
  extrn ext1;
  auto ap;
  auto ext1p;

  printf("&a..%d, &ext1..%d*n", &a, &ext1);

  ap = &a;
  ext1p = &ext1;
  *ap = 10;
  *ext1p = 20;

  printf("a..%d, ext1..%d*n", a, ext1);
}
ext1;