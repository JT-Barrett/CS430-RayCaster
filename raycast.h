#define SCENE_FILE "example.json"
#define OUTPUT_FILE "output.ppm"
#define ORIGIN 0
#define PIXELS_M 100
#define PIXELS_N 100
#define MAX_DEPTH 255

//Needed Structs and Types

typedef struct Object{
  int kind; // 0 = cylinder, 1 = sphere, 2 = plane, 3 = camera
  double color[3];
  union {
    struct {
      double center[3];
      double radius;
    } cylinder;
    struct {
      double center[3];
      double radius;
    } sphere;
    struct {
      double center[3];
      double width;
      double height;
      double normal[3];
    } plane;
    struct {
      double width;
      double height;
    } camera;
  };
} Object;

typedef struct Pixel{
  int r,g,b;
} Pixel;

typedef unsigned int Pixbuff[PIXELS_M*PIXELS_N*3];

typedef Object Scene[128];
//Main method prototypes
int read_scene(char* json, Scene scene);
int raycast (Pixbuff buffer, Scene scene, int num_objects);
int ppm_output(Pixbuff buffer, char *output_file_name, int size, int depth);

//Parser prototypes
double* next_vector(FILE* json);
double next_number(FILE* json);
char* next_string(FILE* json);
void skip_ws(FILE* json);
void expect_c(FILE* json, int d);
int next_c(FILE* json);

//Intersect prototypes and methods
double sphere_intersection(double* Ro, double* Rd, double* C, double r);
double cylinder_intersection(double* Ro, double* Rd, double* C, double r);
double plane_intersection(double* Ro, double* Rd, double* C, double h, double w, double* N);
static inline double sqr(double v) {
  return v*v;
}
static inline void normalize(double* v) {
  double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
  v[0] /= len;
  v[1] /= len;
  v[2] /= len;
}
static inline int float_color_to_int(double d){
}
