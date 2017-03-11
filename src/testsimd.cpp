#include <stdio.h>
#include <intrin.h>

#include "include/stb_stretchy_buffer.h"

struct Model {
  __m128 simd;
  int a;
  int b;
};

int main() {
  printf("vasia\n");

  Model *models = NULL;

  for (int i = 0; i < 100; ++i) {
    Model model;
    model.a = i;
    model.b = i + 1;
    sb_push(models, model);
  }

  for (int i = 0; i < sb_count(models); ++i) {
    printf("%d,", models[i].a);
  }

  return 0;
}
