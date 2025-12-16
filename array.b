main()
{
  auto i;
  auto auto_vec[10];
  extrn ext_vec;

  i = 0;
  while (i <= 10) {
    auto_vec[i] = i + 100;
    i++;
  }

  i = 0;
  while (i <= 10) {
    printf("auto_vec[%d]..%d*n", i, auto_vec[i]);
    i++;
  }

  i = 0;
  while (i <= 10) {
    ext_vec[i] = i + 200;
    i++;
  }

  i = 0;
  while (i <= 10) {
    printf("ext_vec[%d]..%d*n", i, ext_vec[i]);
    i++;
  }
}

ext_vec[10];
