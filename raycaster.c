/*********************************************************************
| Title: raycaster.c
| Author: JT Barrett
| 
|
\*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "raycaster.h"

int main (int argc, char** argv){
	//Create the scene of objects and parse the json file into it
	Scene scene;
	read_scene(SCENE_FILE, &scene);

	//Take the scene and do the raycasting to generate a pixel buffer
	Pixmap buffer;
	raycast(&buffer, &scene);

	//Take the pixel buffer and write it as a valid ppm file.
	ppm_output(&buffer, OUTPUT_FILE, sizeof(buffer), 3, 255);

	return 0;
}

/**********************
|Main Methods
\*********************/

int raycast (Pixmap *pixmap, Scene *scene){

}

int raycast (Pixmap *buffer, Scene *scene){

}

int ppm_output(Pixmap *buffer, char *output_file_name, int size, int format, int depth){

}

/**********************
|Parser Methods
\*********************/
double* next_vector(FILE* json){

}
double next_number(FILE* json){

}
char* next_string(FILE* json){

}
void skip_ws(FILE* json){

}
void expect_c(FILE* json, int d){

}
int next_c(FILE* json){

}

/**********************
|Intersect Methods
\*********************/

double sphere_intersection(double* Ro, double* Rd, double* C, double r){

}
double cylinder_intersection(double* Ro, double* Rd, double* C, double r){

}
double plane_intersection(double* Ro, double* Rd, double* C, double h, double w, double* N){

}