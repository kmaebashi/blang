main() {
  auto c;
loop:
  switch (c=getchar()){

  pr:
    case 'p':  /* print */
    case 'P':
      print();
      goto loop;

    case 's':  /* subs */
    case 'S':
      subs() ;
      goto pr;

    case 'd':  /* delete */
    case 'D':
      delete();
      goto loop;

    case '*n': /* quit on newline */
      break;

    default:
      error();  /* error if fall out */
         goto loop;
  }
}

print() {
  printf("print called.*n");
}

subs() {
  printf("subs called.*n");
}

delete() {
  printf("delete called.*n");
}

error() {
  printf("error called.*n");
}
