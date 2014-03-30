//
//  xmlload.c
//
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include "expat-2.1.0/lib/expat.h"
#include "xmlload.h"

#define XML_BUFFSIZE        8192

typedef struct xml_image_parse_info_struct {
        const char * name;
        size_t offset;
} xml_image_parse_info;
typedef xml_image_parse_info * xml_image_parse_infop;
static const size_t xml_image_parse_size = sizeof(xml_image_parse_info);
static const size_t xml_image_parse_num_elements = 8;

typedef struct xml_context_struct {
        xml_image * firstImage;
        xml_image * lastImage;
} xml_context;
typedef xml_context * xml_contextp;
static const size_t xml_context_size = sizeof(xml_context);

static xml_image_parse_info createImageParseInfo(const char * name, size_t offset)
{
        xml_image_parse_info info;
        info.name = name;
        info.offset = offset;
        return info;
}

static xml_contextp createXmlContext()
{
        xml_contextp newContext = (xml_contextp)malloc(xml_context_size);
        newContext->lastImage = NULL;
        newContext->firstImage = NULL;
        return newContext;
}

static void destroyXmlContext(xml_contextp context)
{
        free(context);
}

static xml_image * createXmlImage()
{
        xml_image * newImage = (xml_image *)malloc(xml_image_size);
        memset(newImage, 0, xml_image_size);
        newImage->next = NULL;
        return newImage;
}

xml_image * getNextImage(xml_image * current)
{
        return current->next;
}

static void addImageToContext(xml_contextp xmlContext, xml_image * imageData)
{
        imageData->next = NULL;
        xml_image * firstImage = xmlContext->firstImage;
        if (firstImage == NULL) {
                xmlContext->firstImage = imageData;
        }
        if (xmlContext->lastImage) {
                xmlContext->lastImage->next = imageData;
        }
        xmlContext->lastImage = imageData;
}

static xml_image_parse_info * xmlParseInfo = NULL;

void initXMLParseInfos()
{
        if (xmlParseInfo != NULL) return;
        xmlParseInfo = malloc(xml_image_parse_size * xml_image_parse_num_elements);
        size_t num = 0;
        xmlParseInfo[num++] = createImageParseInfo("x", offsetof(xml_image, x));
        xmlParseInfo[num++] = createImageParseInfo("y", offsetof(xml_image, y));
        xmlParseInfo[num++] = createImageParseInfo("width", offsetof(xml_image, width));
        xmlParseInfo[num++] = createImageParseInfo("height", offsetof(xml_image, height));
        xmlParseInfo[num++] = createImageParseInfo("frameX", offsetof(xml_image, xOffset));
        xmlParseInfo[num++] = createImageParseInfo("frameY", offsetof(xml_image, yOffset));
        xmlParseInfo[num++] = createImageParseInfo("frameWidth", offsetof(xml_image, fullWidth));
        xmlParseInfo[num++] = createImageParseInfo("frameHeight", offsetof(xml_image, fullHeight));
        assert(num == xml_image_parse_num_elements);
};

void deinitXMLParseInfos()
{
        free(xmlParseInfo);
}

void parseAttribute(const char * attrName, const char * attrValue, xml_image * outInfo,
                    xml_image_parse_infop parseInfos, size_t parseInfoCount)
{
        for (int x = 0; x < parseInfoCount; x++) {
                xml_image_parse_infop parseInfo = parseInfos+x;
                if (strcmp(attrName, parseInfo->name) == 0) {
                        float * value = (float *)((intptr_t)outInfo + parseInfo->offset);
                        *value = atof(attrValue);
                }
        }
}

static const char element_name[]  = "SubTexture";
static const size_t element_name_size = sizeof(element_name);
static void XMLCALL start(void *data, const char *el, const char **attr)
{
        xml_contextp context = (xml_contextp)data;
        
        if (strncmp(el, "SubTexture", element_name_size) == 0) {
                // Found a sub texture element - discover dimensions
                xml_image * newImage = createXmlImage();
                while (*attr != NULL) {
                        parseAttribute(attr[0], attr[1], newImage, xmlParseInfo, xml_image_parse_num_elements);
                        attr++;
                }
                addImageToContext(context, newImage);
        }
}

static void XMLCALL end(void *data, const char *el)
{
}

xml_image * processXML(FILE * file, size_t bufferSize)
{
        char * buffer = (char *)malloc(bufferSize);
        
        initXMLParseInfos();
        xml_contextp context = createXmlContext();
        XML_Parser p = XML_ParserCreate(NULL);
        if (! p) {
                fprintf(stderr, "Couldn't allocate memory for parser\n");
                exit(-1);
        }
        
        XML_SetUserData(p, context);
        XML_SetElementHandler(p, start, end);
        
        for (;;) {
                int done;
                int len;
                
                len = (int)fread(buffer, 1, XML_BUFFSIZE, file);
                if (ferror(file)) {
                        fprintf(stderr, "Read error\n");
                        exit(-1);
                }
                done = feof(file);
                
                if (XML_Parse(p, buffer, len, done) == XML_STATUS_ERROR) {
                        fprintf(stderr, "Parse error at line %lu:\n%s\n",
                                XML_GetCurrentLineNumber(p),
                                XML_ErrorString(XML_GetErrorCode(p)));
                        exit(-1);
                }
                
                if (done)
                        break;
        }
        XML_ParserFree(p);
        deinitXMLParseInfos();
        destroyXmlContext(context);
        free(buffer);
        return context->firstImage;
}

void destroyImageStructList(xml_image * toDestroy)
{
        while (toDestroy) {
                xml_image * next = toDestroy->next;
                free(toDestroy);
                toDestroy = next;
        }
}
