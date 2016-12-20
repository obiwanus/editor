#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "include/stb_stretchy_buffer.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT(x)
#include "include/stb_image.h"
#include "ED_base.h"
#include "ED_math.h"
#include "ED_model.h"

void Model::read_from_obj_file(char *filename) {
  FILE *f = fopen(filename, "rb");
  if (f == NULL) {
    printf("Can't open model file %s\n", filename);
    exit(1);
  }

  this->vertices = NULL;  // stb_stretchy_buffer needs this
  this->faces = NULL;
  this->vts = NULL;

  const int kBufSize = 100;
  char string[kBufSize];
  while (fgets(string, kBufSize, f) != NULL) {
    if (string[0] == 'v' && string[1] == ' ') {
      v3 vertex;
      sscanf(string + 2, "%f %f %f", &vertex.x, &vertex.y, &vertex.z);
      sb_push(v3 *, this->vertices, vertex);
    }
    if (string[0] == 'f' && string[1] == ' ') {
      Face face;
      sscanf(string + 2, "%d/%d/%d %d/%d/%d %d/%d/%d", &face.v_ids[0],
             &face.vn_ids[0], &face.vt_ids[0], &face.v_ids[1], &face.vn_ids[1],
             &face.vt_ids[1], &face.v_ids[2], &face.vn_ids[2], &face.vt_ids[2]);
      sb_push(Face *, this->faces, face);
    }
    if (string[0] == 'v' && string[1] == 't' && string[2] == ' ') {
      v2 vt;  // only expecting 2d textures
      sscanf(string + 3, "%f %f", &vt.x, &vt.y);
      sb_push(v2 *, this->vts, vt);
    }
  }

  fclose(f);
}

void Model::read_texture(char *filename) {
  Image image = {};
  int bytes_per_pixel;

  image.data = (u32 *)stbi_load(filename, &image.width, &image.height,
                                &bytes_per_pixel, sizeof(u32));
  if (image.data == NULL) {
    printf("Can't read texture file %s\n", filename);
    exit(1);
  }

  this->texture = image;
}
