main() {
  if (1 == 1) {
    printf("good1_1*n");
  } else {
    printf("bad1_1*n");
  }
  if (1 == 2) {
    printf("bad1_2*n");
  } else {
    printf("good1_2*n");
  }

  if (1 != 2) {
    printf("good2_1*n");
  } else {
    printf("bad2_1*n");
  }
  if (1 != 1) {
    printf("bad2_2*n");
  } else {
    printf("good2_2*n");
  }

  if (2 > 1) {
    printf("good3_1*n");
  } else {
    printf("bad3_1*n");
  }
  if (2 > 2) {
    printf("bad3_2*n");
  } else {
    printf("good3_2*n");
  }

  if (1 < 2) {
    printf("good4_1*n");
  } else {
    printf("bad4_1*n");
  }
  if (2 < 2) {
    printf("bad4_2*n");
  } else {
    printf("good4_2*n");
  }

  if (2 >= 1) {
    printf("good5_1*n");
  } else {
    printf("bad5_1*n");
  }
  if (2 >= 2) {
    printf("good5_2*n");
  } else {
    printf("bad5_2*n");
  }
  if (2 >= 3) {
    printf("bad5_3*n");
  } else {
    printf("good5_3*n");
  }

  if (1 <= 2) {
    printf("good6_1*n");
  } else {
    printf("bad6_1*n");
  }
  if (2 <= 2) {
    printf("good6_2*n");
  } else {
    printf("bad6_2*n");
  }
  if (3 <= 2) {
    printf("bad6_3*n");
  } else {
    printf("good6_3*n");
  }

}