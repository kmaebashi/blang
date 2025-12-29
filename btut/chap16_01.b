main() {
  auto a, b, c, x;

  a = 10;
  b = 15;
  x = a < b ? a : b;  

  printf("x..%d*n", x);

  a = 15;
  b = 10;
  x = a < b ? a : b;  

  printf("x..%d*n", x);

  a = 20;
  b = 15;
  c = 10;
  x = a < b
    ? a : b < c
    ? b : c;
	  
  printf("x..%d*n", x);

  if (a < 10) {
    printf("bad. a > 10*n");
  } else if (a < 15) {
    printf("bad. a > 15*n");
  } else {
    printf("good. a > 15*n");
  }
}
