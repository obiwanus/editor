
void *Program_Memory::allocate(size_t size) {
  // Deallocation is not intended (yet)
  void *result;
  if (this->allocated + size > MAX_INTERNAL_MEMORY_SIZE) {
    return NULL;
  }
  result = this->free_memory;
  this->free_memory = (void *)((u8 *)this->free_memory + size);
  return result;
}

void Program_State::init(Program_Memory *memory, Pixel_Buffer *buffer) {
  Program_State *state = this;

  g_FPS.value = 0;
  g_FPS.min = 100;
  g_FPS.max = 0;
  g_FPS.frame_count = 10;

  state->kWindowWidth = 1500;
  state->kWindowHeight = 1000;

  state->UI = (User_Interface *)malloc(sizeof(*state->UI));
  memset(state->UI, 0, sizeof(*state->UI));
  state->UI->memory = memory;
  state->UI->buffer = buffer;

  // Allocate memory for the main buffer
  buffer->allocate();
  buffer->width = state->kWindowWidth;
  buffer->height = state->kWindowHeight;

  g_font.load_from_file("../src/ui/fonts/Ubuntu-R.ttf", 16);

  // Create main area
  sb_reserve(state->UI->areas, 10);  // reserve memory for 10 area pointers
  state->UI->create_area(NULL,
                         {0, state->kWindowHeight, state->kWindowWidth, 0});

  state->UI->cursor = V3(0, 0, 0);

  // For some reason these pointers are not initialized by default
  // on windows using C++11 struct initializers
  state->models = NULL;
  state->selected_model = NULL;

  Model model = {};
  model.read_from_obj_file("../models/african_head/african_head.wobj");
  model.read_texture("../models/african_head/african_head_diffuse.jpg");
  model.scale = 0.5f;
  model.position = V3(-1.0f, 0.5f, 0.0f);
  model.direction = V3(-1, 1, 1);
  model.debug = true;
  // model.display = false;
  sb_push(state->models, model);

  // Model model = {};
  // model.read_from_obj_file("../../models/geometricCuldesac.obj");
  // // model.read_texture("../models/african_head/african_head_diffuse.jpg");
  // model.scale = 0.5f;
  // model.position = V3(-1.0f, 0.5f, 0.0f);
  // model.direction = V3(-1, 1, 1);
  // // model.debug = true;
  // // model.display = false;
  // sb_push(state->models, model);

  model = {};
  model.read_from_obj_file("../models/cube/cube.wobj");
  // model.read_texture("../models/cube/cube.png");
  model.scale = 0.4f;
  model.position = V3(0.5f, 0.3f, 0.0f);
  model.direction = V3(1, 1, 1);
  model.debug = true;
  sb_push(state->models, model);

  state->selected_model = models + 0;  // head

  // model.read_from_obj_file("../models/capsule/capsule.wobj");
  // model.read_texture("../models/capsule/capsule0.jpg");
  // sb_push(Model *, state->models, model);
}

void ED_Font::load_from_file(char *filename, int char_height) {
  // Load font into buffer
  {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
      printf("Can't open file '%s'\n", filename);
      exit(1);
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    this->ttf_raw_data = (u8 *)malloc(size * sizeof(*this->ttf_raw_data));
    assert(this->ttf_raw_data != NULL);
    size_t result = fread(this->ttf_raw_data, 1, size, file);

    fclose(file);
  }

  int result =
      stbtt_InitFont(&this->info, this->ttf_raw_data,
                     stbtt_GetFontOffsetForIndex(this->ttf_raw_data, 0));
  if (!result) {
    printf("Can't init font\n");
    exit(1);
  }

  this->scale = stbtt_ScaleForPixelHeight(&this->info, (r32)char_height);
  int ascent, descent, line_gap;
  stbtt_GetFontVMetrics(&this->info, &ascent, &descent, &line_gap);
  this->baseline = (int)(ascent * this->scale);
  this->line_height = (int)((line_gap + ascent - descent) * this->scale);

  int alphabet_size = this->last_char - this->first_char + 1;

  // Fill the char metadata and calculate the
  int buffer_size = 0;
  for (int i = 0; i < alphabet_size; ++i) {
    ED_Font_Codepoint *codepoint = this->codepoints + i;
    codepoint->glyph = stbtt_FindGlyphIndex(&this->info, this->first_char + i);
    int x0, y0, x1, y1;
    stbtt_GetGlyphBitmapBox(&this->info, codepoint->glyph, this->scale,
                            this->scale, &x0, &y0, &x1, &y1);
    codepoint->width = x1 - x0;
    codepoint->height = y1 - y0;
    buffer_size += codepoint->width * codepoint->height;
  }
  this->bitmap = (u8 *)malloc(buffer_size * sizeof(*this->bitmap));

  // Fill the char bitmaps
  u8 *char_bitmap = this->bitmap;
  for (int i = 0; i < alphabet_size; ++i) {
    ED_Font_Codepoint *codepoint = this->codepoints + i;
    codepoint->bitmap = char_bitmap;
    stbtt_MakeCodepointBitmap(&this->info, codepoint->bitmap, codepoint->width,
                              codepoint->height, codepoint->width, this->scale,
                              this->scale, this->first_char + i);
    // Advance to the next char
    char_bitmap += codepoint->width * codepoint->height;
  }
}

Update_Result update_and_render(Program_Memory *program_memory,
                                Program_State *state, User_Input *input) {
  Update_Result result = {};

  // Project mouse pointer into main area
  Area *main_area = state->UI->areas[0];
  input->mouse.y = main_area->get_height() - input->mouse.y;

  // Remember where dragging starts
  for (int i = 0; i < 3; ++i) {
    if (input->button_went_down((Input_Button)i)) {
      input->mouse_positions[i] = input->mouse;
    }
  }

  result = state->UI->update_and_draw(input, state);

  return result;
}

bool Rect::contains(v2i point) {
  bool result = (this->left <= point.x) && (point.x <= this->right) &&
                (this->bottom <= point.y) && (point.y <= this->top);
  return result;
}

v2i Rect::projected(v2i point) {
  v2i result = point;

  result.x -= this->left;
  result.y -= this->bottom;

  return result;
}

inline int Rect::get_width() {
  int result = this->right - this->left;
  assert(result >= 0);
  return result;
}

inline int Rect::get_height() {
  int result = this->top - this->bottom;
  assert(result >= 0);
  return result;
}

int Rect::get_area() {
  // Physical area
  return this->get_width() * this->get_height();
}

void Pixel_Buffer::allocate() {
  *this = {};
  this->max_width = 3000;
  this->max_height = 3000;
  this->memory = malloc(this->max_width * this->max_height * sizeof(u32));
}

Rect Pixel_Buffer::get_rect() {
  Rect result = {0, 0, this->width, this->height};
  return result;
}

bool User_Input::button_was_down(Input_Button button) {
  if (this->old == NULL) return false;
  return this->old->button_is_down(button);
}

bool User_Input::button_is_down(Input_Button button) {
  assert(button < IB__COUNT);
  return this->buttons[button];
}

bool User_Input::button_went_down(Input_Button button) {
  return this->button_is_down(button) && !this->button_was_down(button);
}

bool User_Input::button_went_up(Input_Button button) {
  return !this->button_is_down(button) && this->button_was_down(button);
}

bool User_Input::key_went_down(int key) {
  return this->button_went_down(IB_key) && this->symbol == key;
}
