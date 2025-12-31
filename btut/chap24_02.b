main() {
  auto i, ch;

  i = 0;
  while (1) {
    ch = my_char("hello, world!*n", i);
    if (ch == '*e') {
      break;
    }
    putchar(ch);
    i++;
  }
}

my_char(s, n) return((s[n/4]>>(24-8*(n%4)))&0377);

