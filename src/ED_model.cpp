#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "include/stb_stretchy_buffer.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_TGA
#define STBI_ONLY_GIF
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
  this->vns = NULL;

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
             &face.vt_ids[0], &face.vn_ids[0], &face.v_ids[1], &face.vt_ids[1],
             &face.vn_ids[1], &face.v_ids[2], &face.vt_ids[2], &face.vn_ids[2]);
      for (int i = 0; i < 3; i++) {
        // All indices should start from 0
        face.v_ids[i]--;
        face.vn_ids[i]--;
        face.vt_ids[i]--;
      }
      sb_push(Face *, this->faces, face);
    }
    if (string[0] == 'v' && string[1] == 't' && string[2] == ' ') {
      v2 vt;  // only expecting 2d textures
      sscanf(string + 3, "%f %f", &vt.x, &vt.y);
      sb_push(v2 *, this->vts, vt);
    }
    if (string[0] == 'v' && string[1] == 'n' && string[2] == ' ') {
      v3 vn;
      sscanf(string + 3, "%f %f %f", &vn.x, &vn.y, &vn.z);
      sb_push(v3 *, this->vns, vn);
    }
  }

  fclose(f);
}

void Model::read_texture(char *filename) {
  Image image = {};
  image.data = (u32 *)stbi_load(filename, &image.width, &image.height,
                                &image.bytes_per_pixel, 4);
  if (image.bytes_per_pixel < 3 || image.bytes_per_pixel > 4) {
    printf("Image format not supported: %s\n", filename);
    exit(1);
  }
  if (image.data == NULL) {
    printf("Can't read texture file %s\n", filename);
    exit(1);
  }
  this->texture = image;
}

u32 Image::color(int x, int y, r32 intensity = 1.0f) {
  u32 result;
  // if (this->bytes_per_pixel == 4) {
  //   result = *(this->data + this->width * y + x);
  // } else {
  //   assert(!"TODO: add support for this");
  //   u8 *pixel_byte = (u8 *)this->data + (this->width * y + x) * 3;
  //   result = *((u32 *)pixel_byte) >> 8;
  // }

  // TODO: fix the images
  u32 raw_pixel = *(this->data + this->width * y + x);
  u32 R = intensity * ((0x000000FF & raw_pixel) >> 0);
  u32 G = intensity * ((0x0000FF00 & raw_pixel) >> 8);
  u32 B = intensity * ((0x00FF0000 & raw_pixel) >> 16);
  u32 A = (0xFF000000 & raw_pixel) >> 24;
  result = (A << 24 | R << 16 | G << 8 | B << 0);
  return result;
}
