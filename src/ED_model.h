#ifndef ED_MODEL_H
#define ED_MODEL_H

#include "ED_base.h"

struct Face {
  int v_ids[3];   // vertex
  int vn_ids[3];  // vertex normal
  int tx_ids[3];   // texture
};

struct Model {
  v3 *vertices;
  Face *faces;

  void read_from_obj_file(char *);
};

#endif  // ED_MODEL_H
