#ifndef ED_MODEL_H
#define ED_MODEL_H

struct Ray {
  v3 origin;
  v3 direction;

  v3 point_at(r32 t);
};

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
  v3 position = {{0, 0, 0}};
  v3 up = {{0, 1.0f, 0}};
  v3 direction = {{1.0f, 0, 0}};

  basis3 get_basis();
  m4x4 transform_to_entity_space();
};

struct AABBox {
  r32 x_min, x_max;
  r32 y_min, y_max;
  r32 z_min, z_max;

  r32 hit_by(Ray);
};

struct Model : Entity {
  v3 *vertices;
  v3 *vns;
  v2 *vts;
  Face *faces;
  Image texture;

  r32 scale = 1.0f;
  bool display = true;
  bool debug = false;

  AABBox aabb;

  void read_from_obj_file(char *);
  void read_texture(char *);
  void destroy();
};

struct Camera : Entity {
  // View frustum
  r32 near = -1.0f;
  r32 far = -20.0f;
  r32 top;
  r32 right;
  v2i viewport;

  bool ortho_projection = false;

  v3 pivot = {};
  v3 old_pivot = {};
  v3 old_position = {};
  v3 old_up = {};
  basis3 old_basis = {};

  void adjust_frustum(int, int);
  void look_at(v3);
  m4x4 projection_matrix();
  m4x4 rotation_matrix(v2);
  r32 distance_to_pivot();
  Ray get_ray_through_pixel(v2i);
};

struct Plane {
  v3 normal;
  v3 point;

  r32 hit_by(Ray);
};

struct Triangle {
  v3 vertices[3];

  r32 hit_by(Ray);
};


#endif  // ED_MODEL_H
