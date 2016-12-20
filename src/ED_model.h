#ifndef ED_MODEL_H
#define ED_MODEL_H

#include "ED_base.h"

struct Face {
  int v_ids[3];   // vertex
  int vn_ids[3];  // vertex normal
  int vt_ids[3];   // texture
};

struct Image {
  int width;
  int height;
  u32 *data;
};

struct Model {
  v3 *vertices;
  v2 *vts;
  Face *faces;
  Image texture;

  void read_from_obj_file(char *);
  void read_texture(char *);
};

#endif  // ED_MODEL_H
