#ifndef ED_MODEL_H
#define ED_MODEL_H

#include "ED_math.h"
#include "ED_base.h"

struct Face {
  int v_ids[3];   // vertex
  int vn_ids[3];  // vertex normal
  int vt_ids[3];   // texture
};

struct Image {
  int width;
  int height;
  int bytes_per_pixel;
  u32 *data;

  u32 color(int, int, r32);
};

struct Entity {
  v3 position;
  v3 up;
  v3 direction;

  basis3 get_basis();
  m4x4 transform_to_entity_space();
};

struct Model : Entity {
  v3 *vertices;
  v3 *vns;
  v2 *vts;
  Face *faces;
  Image texture;

  void read_from_obj_file(char *);
  void read_texture(char *);
};

struct Camera : Entity {
  static constexpr r32 vertical_FOV = M_PI / 3;

  // View frustum
  r32 near = -1.0f;
  r32 far = -20.0f;
  r32 top;
  r32 right;

  void adjust_frustum(int, int);
  m4x4 persp_projection();
};

#endif  // ED_MODEL_H
