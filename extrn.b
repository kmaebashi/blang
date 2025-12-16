sub()
{
  extrn ext1;

  printf("ext1..%d*n", ext1);
}

main()
{
  extrn ext1;
  extrn ext2;
  extrn ext3;
  extrn ext4;

  ext1 = 10;

  sub();

  auto i;

  printf("ext2..%d*n", ext2);

  i = 0;
  while (i < 3) {
    printf("ext3[%d]..%d*n", i, ext3[i]);
    i++;
  }
  i = 0;
  while (i < 3) {
    printf("ext4[%d]..%s*n", i, ext4[i]);
    i++;
  }
}

ext1;
ext2 100;
ext3[] 10, 20, 30;
ext4[] "str1", "str2", "str3";