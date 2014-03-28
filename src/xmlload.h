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
typedef xml_image * xml_imagep;
static const size_t xml_image_size = sizeof(xml_image);

xml_imagep processXML(FILE * file, size_t bufferSize);
xml_imagep getNextImage(xml_imagep current);
void destroyImageStructList(xml_imagep toDestroy);


#endif
