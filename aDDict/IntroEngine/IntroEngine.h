#ifndef __engine__
#define __engine__

#include "introwindow.h"
#include "stdio.h"
#include "texgen.h"
#include "vectors.h"

//#define radtheta                 0.017453292

#define dd3d_box                 1
#define dd3d_sphere              3
#define dd3d_dodecaeder          4
#define dd3d_hasab               6
#define dd3d_cone                7
#define dd3d_icosaeder           8
#define dd3d_arc                 9
#define dd3d_loft                10
#define dd3d_line                11
#define dd3d_grid                12
#define dd3d_freetrack           13
#define dd3d_freeobj             14
#define dd3d_clone               15

#define dd3d_butterfly           100
#define dd3d_linearsubdivision   101
#define dd3d_boolean             102
#define dd3d_blur                103
#define dd3d_map                 104

#define dd3d_default             0
#define dd3d_flatshade           1
#define dd3d_gouraudshade        2

#define dd3d_defaultmap          0
#define dd3d_spheremap           1
#define dd3d_planemap            2
#define dd3d_tubemap             3

struct tTexture {
	char			name[100];
	rgba			layers[4][256][256];
	texturecommand	commands[100];
	byte			commandnum;
	string			texts[100];
	byte            number;
	tTexture		*next;
};

struct material {
	char name[100];
	byte texture;
	byte layer;
	byte alpha;
	byte alphalayer;
	byte alphamode;
	GLuint handle;
	byte number;
	material *next;
};

struct SELECTION {
	short int selected;
	SELECTION *next;
};

struct selectionlist {
	char *name;
	SELECTION *selection;
	selectionlist *next;
};


struct camera {
	char *name;

    int number;
    vector3 eye,target,up;
    float fov;

    camera *next;
};

struct edge {
	int v1,v2;
};

struct light{
	GLint identifier;
	bool turnedon;
	GLfloat ambient[4];
	GLfloat color[4];
	GLfloat position[4];
	GLfloat spot_direction[4];
	GLfloat spot_exponent;
	GLfloat spot_cutoff;
	GLfloat c_att,l_att,q_att;
};

struct vertex {

	vector3 d;
	vector3 base;
	vector3 generated;
	vector3 v;
	vector3 transformed;
	vector3 defaultnormal;
	vector3 generatednormal;
	vector3 normal;
	vector3 transformednormal;

	vector2 t;
	vector2 dt;
	vector2 bt;

	int vedgenum;
	int vedgecapacity;
	int *vedgelist;

};

struct polygon {

	int v1,v2,v3;
	vector2 t1,t2,t3;
	vector2 d1,d2,d3;
	vector2 b1,b2,b3;
	vector4 c1,c2,c3;
	vector3 defaultnormal;
	vector3 generatednormal;
	vector3 normal;
	vector3 transformednormal;
	int shading;
	int defshading;
	GLuint materialhandle;
	GLuint envmaphandle;
	int insideotherobject;
	bool normalinverted;

	int pedgelist[3];

};

struct tobjdata {
	bool normalsinverted; //1 bit *******
	byte texx,texy; //2 byte + 2 bit *******
	byte texox,texoy; //2 byte + 2 bit *******
	byte color[4]; //4 byte + 4 bit*********
	byte shading; //2 bit ******
	bool notlit; //1 bit *******
	byte material1; //1 byte + 1 bit******
	byte alphamap1,chnnl1; //1 byte +3 bit*****
	byte material2; //1 byte + 1 bit**** ENVMAP CSAK
	bool alphamapped,envmapped; //*****
	bool textured; //1 bit *******
	byte mapping; //2-3 bit *******
    bool swaptexturexy;
    bool inverttexx;
	bool inverttexy;
	bool buffer;
	unsigned short int number;//2 byte
}; //legjobb esetben: 5 byte :)
   //legroszabb esetben: 20 byte
   //most: 21 byte

struct tminimalobjdata
{
	unsigned primitive       :8,
             normalsinverted :1,
             notlit          :1,
             textured        :1,
             red             :1,
             green           :1,
             blue            :1,
             alpha           :1,
             alphamap1set    :1,
             alphachannel    :2,
             material2set    :1,
             texxset         :1,
             texyset         :1,
             texoffxset      :1,
             texoffyset      :1,
             shading         :2,
             mapping         :3,
             swaptexturexy   :1,
             inverttexx      :1,
			 inverttexy      :1;
}; //31 bit jol :)

struct tminimalheader
{
	unsigned texturesave     :1,
             camerasave      :1,
             lightsave       :1;
};

struct object
{
	char *name;
	bool selected;
	matrix buffermatrix;

	byte primitive; //********
	int params[6];
	tobjdata objdata;

	matrix xformmatrix;

	bool track;
	bool effect;

	vector3 center;
	int vertexnum;
	int vlistsize;
	vertex *vertexlist;

	int polynum;
	int plistsize;
	polygon *polygonlist;

	int edgenum;
	int elistsize;
	edge *edgelist;

	SELECTION *clones,*lastclone;
	SELECTION *cloneparents;
	SELECTION *effects;

	bool bugfixed;
	bool hidden;

	object *next;
};

struct scene
{
	char *name;
	int number;

	camera *cameras;
	selectionlist *selections;
	selectionlist *lastselection;

	object *objects;
	object *lastobject;

	//unsigned char lightnum;
	light lights[8];

	unsigned char floatbytes;

	scene *next;
};

extern camera upcam,leftcam,frontcam,perspcam;
extern vector3 origo;
void init3dengine();
void setcameraview(camera c, float aspectratio);

void     addvertex(object *obj, double x, double y);
vertex  *searchvertex(vertex *vlist, int num);
void     addpolygon(object *obj, int v1, int v2, int v3,int shading);
polygon *searchpolygon(polygon *vlist, int num);
object *searchobjectbynumber(object *vlist, int num);
int objnum(object *pf);

object  *newobject();
scene  *newscene();
int selnum(selectionlist *scn);
int selsize(SELECTION *scn);
extern rgba maptexture[256][256];

void scene_render(scene &scn);
void scene_addobject(scene *scn);
void scene_renderwireframe(scene &scn,byte a,byte b,int tr,int me);
void scene_render(scene &scn);
void scene_rendersolid(scene &scn, bool renderhidden);
void scene_rendertransparent(scene &scn, bool renderhidden);
void scene_renderselect(scene &scn);
void scene_rendernormals(scene &scn);

void obj_transform(object *obj, matrix m);
void obj_createprimitive(object *obj, int primitive, int param1, int param2, int param3, int param4, int param5);

void butterflysubdivision(object *obj, bool linear);
void meshblur(object *obj);
void obj_boolean(object *o1, object*o2, int function);

void obj_counttexturecoordinates(object *o, int mapping, int xr, int yr, int xoff, int yoff, bool swap, bool xswp, bool yswp);
void generatenormals(object *obj);

float getmappixel(vector2 t, int channel, bool wrap);
void obj_boolean(object *o1, object *o2, int function);


#endif