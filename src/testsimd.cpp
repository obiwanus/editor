#include <stdio.h>
#include <intrin.h>
#include <stdlib.h>

#include "include/stb_stretchy_buffer.h"

struct Model {
  __m128 simd;
  int a;
};

int main() {
  Model *models = NULL;
  int *models_raw = NULL;

  for (int i = 0; i < 100; ++i) {
    Model model;
    model.a = i;
    // models[i] = model;

    {
      int cur = models_raw ? models_raw[0] : 0;
      int min_needed = (models_raw ? models_raw[1] : 0) + 1;
      if (cur < min_needed) {
        int dbl_cur = 2 * cur;
        int m = dbl_cur > min_needed ? dbl_cur : min_needed;
        int itemsize = sizeof(Model);

        models_raw =
            (int *)realloc(models_raw ? models_raw : 0, itemsize * m + sizeof(int) * 2);

        if (!models) models_raw[1] = 0;
        models_raw[0] = m;
        models = (Model *)(models_raw + 2);
      }

      models[i] = model;
      models_raw[1] += 1;
    }
    // sb_push(models, model);
  }

  for (int i = 0; i < 100; ++i) {
    printf("%d,", models[i].a);
  }

  return 0;
}
