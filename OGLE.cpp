#include "stdafx.h"

#include "ogle.h"

#include "ObjFile.h"

#include "Ptr/Ptr.in"

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 34962
#endif

#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 34963
#endif

#ifndef GL_READ_ONLY
#define GL_READ_ONLY 35000
#endif

#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY 35001
#endif

#ifndef GL_TEXTURE0
#define GL_TEXTURE0  33984
#endif


/////////////////////////////////////////////
// Static OGLE Variables
/////////////////////////////////////////////

char *OGLE::Config::polyTypes[] = {
	"TRIANGLES",
	"TRIANGLE_STRIP",
	"TRIANGLE_FAN",
	"QUADS",
	"QUAD_STRIP",
	"POLYGON"
};
int OGLE::Config::nPolyTypes = 6;

OGLE::Config OGLE::config;

FILE *OGLE::LOG = fopen("ogle.log", "w");


/////////////////////////////////////////////
// OGLE Initialization
/////////////////////////////////////////////

OGLE::OGLE(InterceptPluginCallbacks *_callBacks, const GLCoreDriver *_GLV) :
	currSet(0),
	currNormal(0),
	currTexCoord(0),
	activeClientTex(0),
	callBacks(_callBacks),
	GLV(_GLV),
	buffers(2048),
	tArray(0),
	tArrayActive(0),
	tArrays(64)
{
	currSet = 0;
	currNormal = 0;
	currTexCoord = 0;

	glClientActiveTexture(GL_TEXTURE0);
}

void OGLE::startRecording(string _objFileName) {
	objFileName = _objFileName;
	objFile = new ObjFile(objFileName);
}

void OGLE::stopRecording() {
	objFile = 0;
	objFileName = "";
}



/////////////////////////////////////////////////////////////////////////////////
// OGLE's OpenGLFunctions
/////////////////////////////////////////////////////////////////////////////////

void  OGLE::glVertexfv(GLfloat *V, GLsizei n) {

	if(currSet) {
		currSet->addElement(new OGLE::Element(new OGLE::Vertex(V, n), 
								(OGLE::config.captureTexCoords ? currTexCoord : 0),
								(OGLE::config.captureNormals ? currNormal : 0)
								)
							);
	}
}

void  OGLE::glNormalfv(GLfloat *V, GLsizei n) {
	if(OGLE::config.captureNormals) {
		currNormal = new OGLE::Vertex(V, n);
		currNormal->w = 0;
	}
}

void  OGLE::glTexCoordfv(GLfloat *V, GLsizei n) {
	if(OGLE::config.captureTexCoords) {
		currTexCoord = new OGLE::Vertex(V, n);
		currTexCoord->w = 0;
	}
}


void OGLE::glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, 
							GLenum type, const GLvoid *indices) {

  checkBuffers();

  indices = getBufferedIndices(indices);

  if(!indices) {
	  fprintf(OGLE::LOG, "\tOGLE::glDrawRangeElements: indices is null\n");	

	  return;
  }

  newSet(mode);

  for(int i = 0; i < count; i++) {
	GLint index = derefIndexArray(type, indices, i);

	if(index >= start && index <= end) {
		glArrayElement(index);
	}
  }

  addSet(currSet);
  currSet = 0;

}


void OGLE::glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
    checkBuffers();

	indices = getBufferedIndices(indices);

	if(!indices) {
		fprintf(OGLE::LOG, "\tOGLE::glDrawElements: indices is null\n");	
		return;
	}

	newSet(mode);

	int index;
	for(int i = 0; i < count; i ++) {
		index = derefIndexArray(type, indices, i);
		glArrayElement(index);
	}

	addSet(currSet);
	currSet = 0;
}


void  OGLE::glDrawArrays (GLenum mode, GLint first, GLsizei count) {

  checkBuffers();

  newSet(mode);

  for(int i = 0; i < count; i ++) {
	GLint index = i + first;
	glArrayElement(index);
  }

  addSet(currSet);
  currSet = 0;
}

void OGLE::glBegin(GLenum mode) {
    checkBuffers();

	newSet(mode);
}

void OGLE::glEnd() {
  addSet(currSet);
  currSet = 0;
}


void OGLE::glArrayElement(GLint i) {

	if(isElementLocked(i)) {
	  return;
	}

	if(!vArray.enabled) return;


	GLfloat V[4];

	if(OGLE::config.captureNormals && nArray.enabled) {
		if(derefClientArray(&nArray, V, i)) {
			glNormalfv(V, nArray.size);
		}
	}


	if(OGLE::config.captureTexCoords && tArray && tArray->enabled) {
		if(derefClientArray(tArray.rawPtr(), V, i)) {
			glTexCoordfv(V, tArray->size);
		}
	}

	if(derefClientArray(&vArray, V, i)) {
		if(currSet) {
			glVertexfv(V, vArray.size);
		}
	}
	else {
		fprintf(OGLE::LOG, "O::gAE Unable to DeRef Vertex Array\n");
		return;
	}

}


void OGLE::glEnableClientState (GLenum array)
{
  switch(array) {
	  case GL_VERTEX_ARRAY: vArray.enabled = true; break;
	  case GL_NORMAL_ARRAY: nArray.enabled = true; break;
	  case GL_TEXTURE_COORD_ARRAY: 
		  if(tArrayActive) tArrayActive->enabled = true; 		  
		  break;
  }
}

void OGLE::glDisableClientState (GLenum array)
{
  switch(array) {
	  case GL_VERTEX_ARRAY:
		  vArray.enabled = false;
		  nArray.enabled = false;
		  if(tArray) tArray->enabled = false;
		  break;
	  case GL_NORMAL_ARRAY: 
		  nArray.enabled = true;
		  break;
	  case GL_TEXTURE_COORD_ARRAY: 
		  if(tArrayActive) tArrayActive->enabled = false; break;
		  break;
  }
}


void OGLE::glClientActiveTexture(GLenum texture) {
	activeClientTex = texture;

	GLint ti = texture - GL_TEXTURE0;
	CArrayPtr ta = tArrays[ti];
	if(!ta) {
		ta = new CArray();
		tArrays[ti] = ta;
	}
	tArrayActive = ta;
}

void  OGLE::glLockArraysEXT(GLint first, GLsizei count) {	
  glState["GL_LOCK_ARRAYS_FIRST"] = new OGLE::Blob(first);  
  glState["GL_LOCK_ARRAYS_COUNT"] = new OGLE::Blob(count);  

}

void  OGLE::glUnlockArraysEXT() {
  glState["GL_LOCK_ARRAYS_FIRST"] = 0;
  glState["GL_LOCK_ARRAYS_COUNT"] = 0;
}

void OGLE::glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
  vArray.size = size;
  vArray.type = type;
  vArray.stride = stride;
  vArray.data = (const GLbyte *)pointer;

  this->glEnableClientState(GL_VERTEX_ARRAY);
}

void OGLE::glNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
{	
  nArray.size = 3;
  nArray.type = type;
  nArray.stride = stride;
  nArray.data = (const GLbyte *)pointer;

  this->glEnableClientState(GL_NORMAL_ARRAY);
}

void OGLE::glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{	
	
	if(tArrayActive) {
	  tArrayActive->size = 3;
	  tArrayActive->type = type;
	  tArrayActive->stride = stride;
	  tArrayActive->data = (const GLbyte *)pointer;

	  tArray = tArrayActive;
	}

    this->glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}


void OGLE::glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer) {

	int str;

	int f = sizeof(GL_FLOAT),
		c = 4 * sizeof(GL_UNSIGNED_BYTE);

	bool et = 0, ec = 0, en = 0;
	GLenum tc;
	int st = 0, sc = 0, sv = 0, pc = 0, pn = 0, pv = 0, s = 0;

	switch(format) {

		case GL_V2F:
			sv = 2; s = 2*f;
			break;

		case GL_V3F:
			sv = 3; s = 3*f;
			break;

		case GL_C4UB_V2F:
			ec = 1; sc = 4; sv = 2; tc = GL_UNSIGNED_BYTE; pv = c; s = c+2*f;
			break;

		case GL_C4UB_V3F:
			ec = 1; sc = 4; sv = 3; tc = GL_UNSIGNED_BYTE; pv = c; s = c+3*f;
			break;

		case GL_C3F_V3F:
			ec = 1; sc = 3; sv = 3; tc = GL_FLOAT; pv = 3*f; s = 6*f;
			break;

		case GL_N3F_V3F:
			en = 1; sv = 3; pv = 3*f; s = 6*f;
			break;

		case GL_C4F_N3F_V3F:
			ec = 1; en = 1; sc = 4; sv = 3; tc = GL_FLOAT; pn = 4*f; pv = 7*f; s = 10*f;
			break;

		case GL_T2F_V3F:
			et = 1; st = 2; sv = 3;pv = 2*f; s = 5*f;
			break;

		case GL_T4F_V4F:
			et = 1; st = 4; sv = 4; pv = 4*f; s = 8*f;
			break;

		case GL_T2F_C4UB_V3F:
			et = 1; ec = 1; st = 2; sc = 4; sv = 3; tc = GL_UNSIGNED_BYTE; pc = 2*f; pv = c+2*f; s = c+5*f;
			break;

		case GL_T2F_C3F_V3F:
			et = 1; ec = 1; st = 2; sc = 3; sv = 3; tc = GL_FLOAT; pc = 2*f; pv = 5*f; s = 8*f;
			break;

		case GL_T2F_N3F_V3F:
			et = 1; en = 1; st = 2; sv = 3; pn = 2*f; pv = 5*f; s = 8*f;
			break;
	
		case GL_T2F_C4F_N3F_V3F:
			et = 1; ec = 1; en = 1; st = 2; sc = 4; sv = 3; tc = GL_FLOAT; pc = 2*f; pn = 6*f; pv = 9*f; s = 12*f;
			break;
	
		case GL_T4F_C4F_N3F_V4F:
			et = 1; ec = 1; en = 1; st = 4; sc = 4; sv = 4; tc = GL_FLOAT; pc = 4*f; pn = 8*f; pv = 11*f; s = 15*f;
			break;

	
		default:
			return;


	}


	str = stride;

	if (str == 0) {
		str = s;
	}

	this->glDisableClientState(GL_EDGE_FLAG_ARRAY);
	this->glDisableClientState(GL_INDEX_ARRAY);

	if (et) {
		this->glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		this->glTexCoordPointer(st, GL_FLOAT, str, pointer);
	}
	else
		this->glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if (ec) {
//		this->glEnableClientState(GL_COLOR_ARRAY);
//		this->glColorPointer(sc, tc, str, pointer+pc);
	}
	else
//		this->glDisableClientState(GL_COLOR_ARRAY);

	if (en) {
		this->glEnableClientState(GL_NORMAL_ARRAY);
		this->glNormalPointer(GL_FLOAT, str, ((GLbyte *)pointer) + pn);
	}
	else
		this->glDisableClientState(GL_NORMAL_ARRAY);

	this->glEnableClientState(GL_VERTEX_ARRAY);
	this->glVertexPointer(sv, GL_FLOAT, str, ((GLbyte *)pointer)+pv);
}



void OGLE::glBindBuffer(GLenum target, GLuint buffer) {

	switch(target) {
		case GL_ARRAY_BUFFER: 
			glState["GL_ARRAY_BUFFER_INDEX"] = new Blob(buffer); break;
		case GL_ELEMENT_ARRAY_BUFFER: 
			glState["GL_ELEMENT_ARRAY_BUFFER_INDEX"] = new Blob(buffer); break;
	}
}

void OGLE::glBufferData(GLenum target, GLsizei size, const GLvoid *data, GLenum usage) {
	GLuint index = getBufferIndex(target);

	if(index) {
		BufferPtr buff = new Buffer(data, size);
		buffers[index] = buff;
	}
}

void OGLE::glBufferSubData(GLenum target, GLint offset, GLsizei size, const GLvoid *data) {
	GLuint index = getBufferIndex(target);

	if(index) {
		BufferPtr buff = buffers[index];
		if(buff && offset < buff->size) {
			GLbyte *p = (GLbyte *) buff->ptr;
			if(offset + size > buff->size) {
				size -= offset + size - buff->size;
			}
			if(data) {
				memcpy(p + offset, data, size);
			}
		}
	}
}

void OGLE::glMapBuffer(GLenum target, GLenum access) {
	glState["GL_MAPPED_BUFFER_TARGET"] = new Blob(target);

	if(GLuint index = getBufferIndex(target)) {
		BufferPtr buff = buffers[index];

		if(buff) {
			buff->mapAccess = access;
		}
	}
}

void OGLE::glMapBufferPost(GLvoid *retValue) {
	BlobPtr target = glState["GL_MAPPED_BUFFER_TARGET"];

	if(target) {
		if(GLuint index = getBufferIndex(target->toEnum())) {
			BufferPtr buff = buffers[index];

			if(buff) {
				buff->map = retValue;
			}
		}
	}

	glState["GL_MAPPED_BUFFER_TARGET"] = 0;
}

void OGLE::glUnmapBuffer(GLenum target) {

	if(GLuint index = getBufferIndex(target)) {
		BufferPtr buff = buffers[index];

		if(buff) {
			if(buff->map && buff->mapAccess != 0*GL_WRITE_ONLY) {
				memcpy(buff->ptr, buff->map, buff->size);
			}
			buff->map = 0;
			buff->mapAccess = 0;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////
// OGLE utility functions
////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// OGLE's Helper Functions
/////////////////////////////////////////////////////////////////////////////////


void OGLE::initFunctions() {

	extensionVBOSupported = false;

	float oglVersion = callBacks->GetGLVersion();
	if(oglVersion >= 1.5f ||
		callBacks->IsGLExtensionSupported("GL_ARB_vertex_buffer_object")) {
		extensionVBOSupported = true;

	    //Attempty to get the core version
		iglGetBufferSubData = (GLvoid (GLAPIENTRY *) (GLenum, GLint, GLsizei, GLvoid *))callBacks->GetGLFunction("glGetBufferSubData");

		if(iglGetBufferSubData == NULL) {
		  //Attempt to get the ARB version instead
		  iglGetBufferSubData = (GLvoid (GLAPIENTRY *) (GLenum, GLint, GLsizei, GLvoid *))callBacks->GetGLFunction("glGetBufferSubDataARB");

		  if(iglGetBufferSubData == NULL) {
			 //Unable to get the entry point, don't check for VBOs
			 extensionVBOSupported = false;
		  }
		}
	}

}




void OGLE::addSet(ElementSetPtr set) {
	
  if(! set) return;

  if(
	  (set->mode == GL_TRIANGLES && OGLE::config.polyTypesEnabled["TRIANGLES"]) 
	  || (set->mode == GL_TRIANGLE_STRIP && OGLE::config.polyTypesEnabled["TRIANGLE_STRIP"]) 
	  || (set->mode == GL_TRIANGLE_FAN && OGLE::config.polyTypesEnabled["TRIANGLE_FAN"])
	  || (set->mode == GL_QUADS && OGLE::config.polyTypesEnabled["QUADS"])
	  || (set->mode == GL_QUAD_STRIP && OGLE::config.polyTypesEnabled["QUAD_STRIP"])
	  || (set->mode == GL_POLYGON && OGLE::config.polyTypesEnabled["POLYGON"]) 
	  ) {

	  objFile->addSet(set);
	  // no need to store the ElementSets
	  //  sets.push_back(set);

  }


}

void OGLE::newSet(GLenum mode)
{
	Transform _transform = this->getCurrTransform();
	Transform _texCoordTransform = this->getCurrTransform(GL_TEXTURE_MATRIX);
	currSet = new OGLE::ElementSet(mode, _transform, _texCoordTransform);
}


GLint OGLE::derefClientArray(CArray *arr, GLfloat *v, GLint i) {
	if(!arr) return 0;

	const GLbyte *p = arr->data;
	p = getBufferedArray(p);

	if(!p) return 0;

	int j;
	for(j = 0; j < arr->size; j++) {
		v[j] = derefVertexArray(p, arr->size, arr->type, arr->stride, i, j);
	}
	return j;
}

GLfloat OGLE::derefVertexArray(const GLbyte *array, GLint dim, GLenum type, GLsizei stride, GLint vindex, GLint index) {
	GLsizei size = glTypeSize(type);

	const GLbyte *p = array + vindex * (stride ? stride : (dim * size) ) + (index * size);

	GLfloat c;

	switch (type) {
		case GL_SHORT: c = *((GLshort *) p); break;
		case GL_INT: c = *((GLint *) p); break;
		case GL_FLOAT: c = *((GLfloat *) p); break;
		case GL_DOUBLE: c = *((GLdouble *) p); break;
	}
		
	return c;
}


#define GLVoidPDeRef(TYPE, ARRAY, INDEX) (  *((TYPE *)(((char *)ARRAY) + INDEX * sizeof(TYPE)))  )

GLint OGLE::derefIndexArray(GLenum type, const GLvoid *indices, int i) {
	GLint index = 0;

	switch (type) {
		case GL_UNSIGNED_BYTE: index = GLVoidPDeRef(GLubyte, indices, i); break;
		case GL_UNSIGNED_SHORT:  index = GLVoidPDeRef(GLushort, indices, i); break;
		case GL_UNSIGNED_INT:  index = GLVoidPDeRef(GLuint, indices, i);break;
    }

	return index; 

}

bool OGLE::isElementLocked(int i) {
  GLint lock_first;
  GLsizei lock_count;
  BlobPtr lfP, lcP;

  lfP =	glState["GL_LOCK_ARRAYS_FIRST"];
  lock_first = lfP ? lfP->toInt() : -1;

  lcP =	glState["GL_LOCK_ARRAYS_COUNT"];
  lock_count = lcP ? lcP->toSizeI() : 0;

  return (lock_first > -1 && lock_count > 0) && (i < lock_first || i > lock_first + lock_count);
}


GLuint OGLE::getBufferIndex(GLenum target) {
	BlobPtr index;

	switch(target) {
		case GL_ARRAY_BUFFER: 
			index = glState["GL_ARRAY_BUFFER_INDEX"]; break;
		case GL_ELEMENT_ARRAY_BUFFER: 
			index = glState["GL_ELEMENT_ARRAY_BUFFER_INDEX"]; break;
	}

	return index ? index->toUInt() : 0;
}


const GLbyte *OGLE::getBufferedArray(const GLbyte *array) {
	GLuint buffIndex = 0;
	GLuint offset = 0;
	BufferPtr buff;


	if(buffIndex = getBufferIndex(GL_ARRAY_BUFFER)) {
		BufferPtr buff = buffers[buffIndex];
		if(buff && buff->ptr) {
			offset = (GLuint)array;
			array = ((GLbyte *)buff->ptr) + offset;
		}
	}
	return array;
}

const GLvoid *OGLE::getBufferedIndices(const GLvoid *indices) {

	const GLbyte *ptr = (GLbyte *)indices;

	if(GLuint buffIndex = getBufferIndex(GL_ELEMENT_ARRAY_BUFFER)) {
		BufferPtr buff = buffers[buffIndex];
		if(!buff || buff->ptr == 0) {
			return 0;
		}
		if(buff && buff->ptr) {
			GLuint offset = (GLuint)ptr;
			ptr = ((GLbyte *)buff->ptr) + offset;
		}
	}


	return ptr;
}


// Look to see if the program has set up Vertex or Element buffers
// Copy them into buffers we will read from.

void OGLE::checkBuffers() {
	GLenum targets[2] = {GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER};

	if(!(extensionVBOSupported && iglGetBufferSubData)) {
		return;
	}

	for(int i = 0; i < 2; i++) {
		int index = getBufferIndex(targets[i]);

		BufferPtr bp = buffers[index];
		if(bp) {
		  iglGetBufferSubData(targets[i], 0, bp->size, bp->ptr);
		}
	}
}


GLsizei OGLE::glTypeSize(GLenum type) {
	GLsizei size;
	switch (type) {
		case GL_SHORT: size = sizeof(GLshort); break;
		case GL_INT: size = sizeof(GLint); break;
		case GL_FLOAT: size = sizeof(GLfloat); break;
		case GL_DOUBLE: size = sizeof(GLdouble); break;
	}	
	return size;
}


OGLE::VertexPtr OGLE::doTransform(VertexPtr vp, Transform T) {
	
	  OGLE::Vector V = vp->toVector();

	  OGLE::Vector R(4);

	  mtl::mult(T,V,R);

	  return new OGLE::Vertex(R);
}
	
OGLE::Transform OGLE::getCurrTransform(GLenum type) {
		int i, j;

		GLfloat mat[16];
		GLV->glGetFloatv(type, mat);

		Transform T(4,4);
		for( i = 0; i < 4; i++) {
			for( j = 0; j < 4; j++) {
				T(j,i) = (float) mat[4*i + j];
			}
		}
		return T;
}

bool OGLE::isIdentityTransform(Transform T) {
	OGLE::Vector V(4,1);
	OGLE::Vector R(4);

	mtl::mult(T,V,R);

	float prod = 1.0;
	for(int t = 0; t < 4; t++) {
		prod *= R[t];
	}

	return prod == 1.0;
}







//////////////////////////////////////////////////////////////////////////////////
// OGLE::ElementSet functions
//////////////////////////////////////////////////////////////////////////////////

OGLE::ElementSet::ElementSet(GLenum _mode, Transform _transform, Transform _texCoordTransform) :
	mode(_mode),
	transform(_transform),
	texCoordTransform(_texCoordTransform),
	hasTransform(1)
{}

OGLE::ElementSet::ElementSet(GLenum _mode) :
	mode(_mode),
	hasTransform(0)
{}

void OGLE::ElementSet::addElement(const GLfloat V[], GLsizei dim) {
	addElement(new Vertex(V, dim));
}

void OGLE::ElementSet::addElement(VertexPtr V) {	
	addElement(new OGLE::Element(V));
}

void OGLE::ElementSet::addElement(ElementPtr E) {
	if(hasTransform) {
		if(E->v) E->v = OGLE::doTransform(E->v, transform);
		if(E->n) E->n = OGLE::doTransform(E->n, transform);
		if(E->t) E->t = OGLE::doTransform(E->t, texCoordTransform);
	}


	float scale = OGLE::config.scale;
	if(scale) {
		if(E->v) {
			E->v->x *= scale;
			E->v->y *= scale;
			E->v->z *= scale;
		}

		if(E->n) {
			E->n->x *= scale;
			E->n->y *= scale;
			E->n->z *= scale;
		}
	}

	elements.push_back(E);
}

