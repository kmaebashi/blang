main() {
  auto c;
read:
  c = putchar(getchar());
  if (c != '*n') goto read;
}
