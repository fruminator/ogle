#ifndef __OBJFILE_H_
#define __OBJFILE_H_

#include "..\\InterceptPluginInterface.h"

#include <string>
#include <vector>
#include <deque>

#include "ogle.h"

class ObjFile : public Interface {

  public:

	class Element {
		public:
			int vid, nid, tid;
			Element(int _vid = 0, int _nid = 0, int _tid = 0) 
					: vid(_vid), nid(_nid), tid(_tid) {}
	};


	class Face : public Interface {
		public:
			void addElement(Element v);
			void shiftElements(int n = 1);
			void clear();

			deque<Element> elements;
	};

	typedef Ptr<Face> FacePtr;


	ObjFile(string _objFileName);
	~ObjFile();


	void addSet(OGLE::ElementSetPtr set);
	void printSet(OGLE::ElementSetPtr set);
	Element generateElement(OGLE::ElementPtr e);


	void printVertex(OGLE::VertexPtr v, const char *typeStr, int n = 0);
	void printFace(FacePtr face, bool flip = 0);

	
	FILE *f;
	string objFileName;
	
	static void init();

	static int nextVertexID();
	static int nextNormalID();
	static int nextTexCoordID();
	static int nextGroupID();

	static int vertexCount;
	static int normalCount;
	static int texCoordCount;
	static int groupCount;




};

typedef Ptr<ObjFile> ObjFilePtr;

#endif // __OBJFILE_H_
