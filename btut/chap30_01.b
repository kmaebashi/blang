main() {
  my_putnumb(123);
  putchar('*n');
}

my_putnumb(n) {
  auto a;
  if(a=n/10)   /* assignment, not equality test */
    my_putnumb(a);  /* recursive */
  putchar(n%10 + '0');
}