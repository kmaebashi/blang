main() {
  /* name tokenization */
  auto a.b, .a, a1;

  a.b = 1;
  .a = 2;
  a1 = 3;
  printf("a.b..%d, .a..%d, a1..%d*n", a.b, .a, a1);

  /* escape sequences in string */
  printf("*(*)*t***'*"*n*eabc");
  extrn esc_str;
  assert_eq(char(esc_str, 0), 0);

  /* unary operator */
  assert_eq(- 5, 5 - 10);
  assert_eq(!5, 0);
  auto i;
  i = 0;
  i++;
  assert_eq(i, 1);
  i = 0;
  i--;
  assert_eq(i, -1);
  i = 0;
  ++i;
  assert_eq(i, 1);
  i = 0;
  --i;
  assert_eq(i, -1);
  assert_eq(~100, -101);

  i = 0;
  assert_eq(i++, 0);
  assert_eq(i, 1);
  i = 0;
  assert_eq(i--, 0);
  assert_eq(i, -1);
  i = 0;
  assert_eq(++i, 1);
  assert_eq(i, 1);
  i = 0;
  assert_eq(--i, -1);
  assert_eq(i, -1);

  /* basic arithmetic operators */
  assert_eq(1 + 2 * 6 / 3 - 2, 3);
  i = 2;
  assert_eq(1 + i * 6 / 3 - 2, 3);
  assert_eq(8 % 3, 2);
  i = 8;
  assert_eq(i % 3, 2);

  /* shift operators */
  assert_eq(2 << 2, 8);
  assert_eq(8 >> 2, 2);

  /* relational operators */
  assert_eq(1 < 2, 1);
  assert_eq(1 < 1, 0);
  assert_eq(2 < 1, 0);
  assert_eq(1 <= 2, 1);
  assert_eq(1 <= 1, 1);
  assert_eq(2 <= 1, 0);
  assert_eq(2 > 1, 1);
  assert_eq(1 > 1, 0);
  assert_eq(1 > 2, 0);
  assert_eq(2 >= 1, 1);
  assert_eq(1 >= 1, 1);
  assert_eq(1 >= 2, 0);

  /*equality operators */
  assert_eq(1 == 1, 1);
  assert_eq(1 == 0, 0);
  assert_eq(1 != 0, 1);
  assert_eq(1 != 1, 0);

  /* bit operators */
  assert_eq(5 & 4, 4);
  assert_eq(5 ^ 4, 1);
  assert_eq(5 | 8, 13);

  /* function pointer */
  auto f;
  extrn sub;
  f = sub;
  assert_eq(f(1, 5), 6);

  /* passing array */
  auto array[10];
  i = 0;
  while (i <= 10) {
    array[i] = i * 2;
    i++;
  }
  pass_array(array);

  /* nargs */
  nargs_test(1, 2, 3, 4);

  auto j;
  extrn matrix;
  i = 0;
  while (i <= 3) {
    j = 0;
    while (j <= 3) {
      printf("matrix[%d][%d]..%d*n", i, j, matrix[i][j]);
      j++;
    }
    i++;
  }
  
}

esc_str "*0";

v1[] 1, 2, 3, 4;
v2[] 5, 6, 7, 8;
v3[] 9, 10, 11, 12;
v4[] 13, 14, 15, 16;
matrix[] v1, v2, v3, v4;


sub(a, b) {
  return a + b;
}

pass_array(a) {
  auto i;
  i = 0;

  while (i <= 10) {
    printf("a[%d]..%d*n", i, a[i]);
    i++;
  }
}

nargs_test(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) {
  assert_eq(nargs(), 4);
}

assert_eq(a, b) {
  if (a != b) {
    printf("ERROR!*n");
  }  
}
