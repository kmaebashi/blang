sub(a)
{
  switch (a) {
    case 1:
      printf("case1*n");
      break;
    case 2:
      printf("case2*n");
      break;
    case 3:
      printf("case3*n");
      break;
    default:
      printf("default*n");
  }
}

main()
{
  sub(1);
  sub(2);
  sub(3);
  sub(4);
}
