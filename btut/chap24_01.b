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

my_char(s, n) {
  auto y,sh,cpos;
  y = s[n/4];        /* word containing n-th char */
  cpos = n%4;        /* position of char in word */
  sh = 24 - 8 * cpos;    /* bit positions to shift */
  y =  (y>>sh) & 0377; /* shift and mask all but 8 bits */
  return(y);         /* return character to caller */
}
