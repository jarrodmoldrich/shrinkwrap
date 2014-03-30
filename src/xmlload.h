//
//  xmlload.h
//

#ifndef makeshrinkwrap_xmlload_h
#define makeshrinkwrap_xmlload_h

typedef struct xml_image_struct {
  // Pixel locations and offsets
  float x;
  float y;
  float width;
  float height;
  // Geometry offsets and uncropped dimensions
  float xOffset;
  float yOffset;
  float fullWidth;
  float fullHeight;
  struct xml_image_struct * next;
} xml_image;
static const size_t xml_image_size = sizeof(xml_image);

xml_image * processXML(FILE * file, size_t bufferSize);
xml_image * getNextImage(xml_image * current);
void destroyImageStructList(xml_image * toDestroy);


#endif
