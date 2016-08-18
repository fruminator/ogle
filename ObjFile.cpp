#include "stdafx.h"

#include "Ptr/Ptr.in"

#include "ogle.h"

#include "ObjFile.h"


int ObjFile::vertexCount;
int ObjFile::normalCount;
int ObjFile::texCoordCount;
int ObjFile::groupCount;

ObjFile::ObjFile(string _objFileName) {
	objFileName = _objFileName;
	f =	fopen(objFileName.c_str(), "w");

	init();
}

ObjFile::~ObjFile() {
	if(f) {
		fclose(f);
	}
}

void ObjFile::init() {
	vertexCount = 0;
	normalCount = 0;
	texCoordCount = 0;
	groupCount = 0;
}


ObjFile::Element ObjFile::generateElement(OGLE::ElementPtr e) {
	OGLE::VertexPtr v;

	v = e->v;
	printVertex(v, "", 3);
	Element oe(nextVertexID());

	v = e->n;
	if(v) {
		printVertex(v, "n", 3);
		oe.nid = nextNormalID();
	}

	v = e->t;
	if(v) {
		printVertex(v, "t", 2);
		oe.tid = nextTexCoordID();
	}

	return oe;

}

void ObjFile::addSet(OGLE::ElementSetPtr set) {	
	printSet(set);
}


void ObjFile::printSet(OGLE::ElementSetPtr set) {
	if(!f) return;

	int i;
	Element e;

	FacePtr face = new ObjFile::Face();


	if(0) {}
	else if(set->mode == GL_TRIANGLES) {
		if(set->elements.size() >= 3) fprintf(f, "#TRIANGLES\ng %d\n", nextGroupID());
		for(i = 0; i < set->elements.size(); i++) {

			face->addElement(generateElement(set->elements[i]));

			if(((i + 1) % 3) == 0) {
				printFace(face);
				face->clear();
			}
		}
	}

	else if(set->mode == GL_TRIANGLE_STRIP) {
		if(set->elements.size() >= 3) fprintf(f, "#TRIANGLE_STRIP\ng %d\n", nextGroupID());
		int flip_flag = 1;
		for(i = 0; i < set->elements.size(); i++) {

			face->addElement(generateElement(set->elements[i]));

			if(i >= 2) {
				printFace(face,
					OGLE::config.flipPolyStrips	&& (flip_flag = (flip_flag + 1) % 2)
					);

				face->shiftElements(1);
			}
		}
	}

	else if(set->mode == GL_TRIANGLE_FAN) {
		if(set->elements.size() >= 3) fprintf(f, "#TRIANGLE_FAN\ng %d\n", nextGroupID());

		Element firste, laste;

		for(i = 0; i < set->elements.size(); i++) {
			e = generateElement(set->elements[i]);

			if(i == 0) {
				firste = e;
			}
			laste = e;

			face->addElement(e);
			
			if(i >= 2) {
				printFace(face);
				face->clear();
				face->addElement(firste);
				face->addElement(laste);
			}
		}
	}

	else if(set->mode == GL_QUADS) {
		if(set->elements.size() >= 4) fprintf(f, "#QUADS\ng %d\n", nextGroupID());
		for(i = 0; i < set->elements.size(); i++) {

			face->addElement(generateElement(set->elements[i]));

			if(((i + 1) % 4) == 0) {
				printFace(face);
				face->clear();
			}
		}
	}

	else if(set->mode == GL_QUAD_STRIP) {
		if(set->elements.size() >= 4) fprintf(f, "#QUAD_STRIP\ng %d\n", nextGroupID());
		int flip_flag = 1;
		for(i = 0; i < set->elements.size(); i++) {

			face->addElement(generateElement(set->elements[i]));

			if(i >= 3) {
				printFace(face,
					OGLE::config.flipPolyStrips	&& (flip_flag = (flip_flag + 1) % 2)					
					);
				face->shiftElements(1);
			}
		}
	}

	else if(set->mode == GL_POLYGON) {
		if(set->elements.size() >= 3) fprintf(f, "#POLYGON [%d]\ng %d\n", set->elements.size(), nextGroupID());
		for(i = 0; i < set->elements.size(); i++) {
			face->addElement(generateElement(set->elements[i]));
		}
		printFace(face);
	}

	fflush(f);
}


void ObjFile::printFace(FacePtr face, bool flip) {
	if(!f) return;

	fprintf(f, "f ");
	for(int i = (flip ? face->elements.size() - 1 : 0); 
					i < face->elements.size() && i >= 0; 
					i += (flip ? -1 : 1)) {

		Element v = face->elements[i];

		fprintf(f, "%d", v.vid);

		if(v.tid) {
			fprintf(f, "/%d", v.tid);
		}
		else if(v.nid) {
			fprintf(f, "/");
		}

		if(v.nid) {
			fprintf(f, "/%d", v.nid);
		}

		fprintf(f, " ");
	}

	fprintf(f, "\n");
}


void ObjFile::printVertex(OGLE::VertexPtr v, const char *typeStr, int n) {
	if(!f) return;

	fprintf(f, "v%s", typeStr);

	if(n <= 0) n = v->size;

	if(n >= 1) fprintf(f, " %e", v->x);
	if(n >= 2) fprintf(f, " %e", v->y);
	if(n >= 3) fprintf(f, " %e", v->z);

	fprintf(f, "\n");	
}






void ObjFile::Face::addElement(Element e) { 
	elements.push_back(e); 
}

void ObjFile::Face::shiftElements(int n) { 
	while(n-- > 0) elements.pop_front(); 
}

void ObjFile::Face::clear() { 
	elements.clear(); 
}


int ObjFile::nextVertexID() {
	return ++vertexCount;
}

int ObjFile::nextNormalID() {
	return ++normalCount;
}

int ObjFile::nextTexCoordID() {
	return ++texCoordCount;
}

int ObjFile::nextGroupID() {
	return ++groupCount;
}