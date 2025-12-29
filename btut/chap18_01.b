main() {
  auto c, i;

  c = 0;
  i = 32;

  while ((i = i - 8) >= 0) c= c | getchar() << i;

  putchar(c);
}
