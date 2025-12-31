main() {
  auto s[20];

  my_getstr(s);

  printf("s..%s*n", s);
}

my_getstr(s){
auto c,i;
i=0;
while ((c=getchar()) != '*n') lchar(s,i++,c);
lchar(s,i,'*e');
return(s) ;
}
