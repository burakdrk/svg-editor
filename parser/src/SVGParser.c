/*  CIS*2750 Assignment 2
 * Name: Burak Ege Duruk
 * Student ID: 1161479
 *
 * Used the shared sample code from lectures (StructListDemo.c and libXmlExample.c)
 * as well as code from libxml2 example documentation and shared blogpost.
 * (http://www.xmlsoft.org/examples/tree1.c , http://www.xmlsoft.org/examples/tree2.c and
 * http://knol2share.blogspot.com/2009/05/validate-xml-against-xsd-in-c.html)
 */

#include "SVGParser.h"
#include <math.h>

//Ass1
void populateSVGTree(SVG *svg, xmlNode *a_node);
Attribute* parseAttribute(xmlAttr *attr);
Rectangle* setRectangle(xmlNode *node);
Circle* setCircle(xmlNode *node);
Path* setPath(xmlNode *node);
Group* setGroup(xmlNode *node);
void genericGet(List *to, List *from);
void getFromGroups(List *to, List *from, char type);
void dummyDelete();

//Ass2
bool validateXML(xmlDoc *doc, const char* schemaFile);
xmlDoc* convertSVGtoXML(const SVG* img);
void writeProp(xmlNode *node, List *list);
void writeChild(xmlNode *node, List *rectangleList, List *circleList, List *pathList);
void writeGroup(xmlNode *node, List *list);
bool validateProp(List *list);
bool validateChild(List *rectangleList, List *circleList, List *pathList);
bool validateGroup(List *list);
bool modifyAttribute(List *list, Attribute *newAttribute);
bool modifyCircle(List *list, Attribute *newAttribute, int elemIndex);
bool modifyRectangle(List *list, Attribute *newAttribute, int elemIndex);
bool modifyPath(List *list, Attribute *newAttribute, int elemIndex);
bool modifyGroup(List *list, Attribute *newAttribute, int elemIndex);
void *getAtIndex(List *list, int elemIndex);

SVG* createSVG(const char* fileName)    {
    if(fileName == NULL) return NULL;
    if(strlen(fileName) == 0) return NULL;

    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;

    LIBXML_TEST_VERSION

    doc = xmlReadFile(fileName, NULL, 0);

    if(doc == NULL) {
        xmlCleanupParser();
        return NULL;
    }

    root_element = xmlDocGetRootElement(doc);
    if(root_element == NULL) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return NULL;
    }
    
    //Simply allocate memory and initialize each element of the svg struct
    SVG *svg = malloc(sizeof(SVG));
    if(svg == NULL) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return NULL;
    }

    //Initialization
    svg->namespace[0] = 0;
    svg->description[0] = 0;
    svg->title[0] = 0;
    svg->otherAttributes = initializeList(&attributeToString, &deleteAttribute, &compareAttributes);
    svg->rectangles = initializeList(&rectangleToString, &deleteRectangle, &compareRectangles);
    svg->circles = initializeList(&circleToString, &deleteCircle, &compareCircles);
    svg->paths = initializeList(&pathToString, &deletePath, &comparePaths);
    svg->groups = initializeList(&groupToString, &deleteGroup, &compareGroups);

    //Traverse tree
    populateSVGTree(svg, root_element);

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return svg;
}

//Simply free every list and the struct itself
void deleteSVG(SVG* img)    {
    if(img == NULL) return;

    freeList(img->otherAttributes);
    freeList(img->rectangles);
    freeList(img->circles);
    freeList(img->paths);
    freeList(img->groups);

    free(img);
}

//Allocate and set a string
char* SVGToString(const SVG* img)   {
    if(img == NULL) return NULL;

    //Get every list as a string
    char *attr = toString(img->otherAttributes);
    char *rect = toString(img->rectangles);
    char *circle = toString(img->circles);
    char *path = toString(img->paths);
    char *g = toString(img->groups);

    int len = strlen(attr)+strlen(img->namespace)+strlen(img->title)+strlen(img->description)+100+strlen(rect)+strlen(circle)+strlen(path)+strlen(g);
    char *tempStr = malloc(sizeof(char) * len);
    if(tempStr == NULL) return NULL;
    sprintf(tempStr, "SVG:\n\tNamespace: %s\n\tAttributes:\n%s\tTitle: %s\n\tDescription: %s\n%s%s%s%s\n",img->namespace,attr,img->title,img->description,rect,circle,path,g);

    //Free the strings
    free(path);
    free(circle);
    free(rect);
    free(attr);
    free(g);

    return tempStr;
}

//Populate the SVG struct using the xml nodes recursively.
void populateSVGTree(SVG *svg, xmlNode *a_node)   {
    if(svg == NULL || a_node == NULL) return;

    for(xmlNode *cur_node = a_node; cur_node != NULL; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if(strcmp((char*)cur_node->name, "title") == 0) {
                //Set the title of the SVG struct
                sprintf(svg->title, "%.*s", 255, (char*)(cur_node->children->content));
            }
            else if(strcmp((char*)cur_node->name, "desc") == 0) {
                //Set the description of the SVG struct
                sprintf(svg->description, "%.*s", 255, (char*)(cur_node->children->content));
            }
            else if(strcmp((char*)cur_node->name, "svg") == 0) {
                //Set the attributes and namespace of the SVG struct
                strcpy(svg->namespace, (char*)(cur_node->ns->href));
                for(xmlAttr *attr = cur_node->properties; attr != NULL; attr = attr->next)  {
                    insertBack(svg->otherAttributes, parseAttribute(attr));
                }
            }
            else if(strcmp((char*)cur_node->name, "rect") == 0) {
                //Set the rectangles of the SVG struct
                insertBack(svg->rectangles, setRectangle(cur_node));
            }
            else if(strcmp((char*)cur_node->name, "circle") == 0) {
                //Set the circles of the SVG struct
                insertBack(svg->circles, setCircle(cur_node));
            }
            else if(strcmp((char*)cur_node->name, "path") == 0) {
                //Set the paths of the SVG struct
                insertBack(svg->paths, setPath(cur_node));
            }
            else if(strcmp((char*)cur_node->name, "g") == 0) {
                //Set the groups of the SVG struct
                insertBack(svg->groups, setGroup(cur_node));
                //Don't go any deeper (the children of the groups) this is done in setGroup.
                continue;
            }
        }

        //Go to the deeper children
        populateSVGTree(svg, cur_node->children);
    }
}


//Attributes

//Allocate and populate the attribute struct
Attribute* parseAttribute(xmlAttr *attr)    {
    if(attr == NULL) return NULL;

    char *attrName = (char*)attr->name;
    char *cont = (char*)attr->children->content;

    //Malloc with flexible array member
    Attribute *temp = malloc(sizeof(Attribute) + sizeof(char)*(strlen(cont)+1));
    if(temp == NULL) return NULL;

    temp->name = malloc(sizeof(char) * (strlen(attrName)+1));
    if(temp->name == NULL) return NULL;

    strcpy(temp->value, cont);
    strcpy(temp->name, attrName);

    return temp;
}

//Simple delete function for Attribute structs
void deleteAttribute(void* data)    {
    if(data == NULL) return;

    //Assign generic data to attribute
    Attribute *temp = (Attribute*)data;

    free(temp->name);
    free(temp);
}

//Allocate and set a string
char* attributeToString(void* data) {
    if(data == NULL) return NULL;

    Attribute *temp = (Attribute*)data;
    int len = strlen(temp->value)+strlen(temp->name)+100;

    char *tempStr = malloc(sizeof(char)*len);
    if(tempStr == NULL) return NULL;
    sprintf(tempStr, "\t\t%s = %s\n", temp->name, temp->value);

    return tempStr;
}

int compareAttributes(const void *first, const void *second)    {
    return 0;
}

// End of Attributes


// Rectangles

Rectangle* setRectangle(xmlNode *node)    {
    if(node == NULL) return NULL;

    //Initialize
    Rectangle *temp = malloc(sizeof(Rectangle));
    if(temp == NULL) return NULL;
    temp->units[0] = 0;
    temp->width=0;
    temp->height=0;
    temp->x=0;
    temp->y=0;
    temp->otherAttributes = initializeList(&attributeToString, &deleteAttribute, &compareAttributes);

    //Same technique as before to set the struct
    for(xmlAttr *attr = node->properties; attr != NULL; attr = attr->next)  {
        //Separating units from the values
        if(strcmp((char*)attr->name, "x") == 0)    {
            char *anan;
            float ret = (float)strtod((char*)attr->children->content, &anan);
            strcpy(temp->units, anan);
            temp->x = ret;
        }
        else if(strcmp((char*)attr->name, "y") == 0)   {
            char *anan;
            float ret = (float)strtod((char*)attr->children->content, &anan);
            strcpy(temp->units, anan);
            temp->y = ret;
        }
        else if(strcmp((char*)attr->name, "width") == 0)   {
            char *anan;
            float ret = (float)strtod((char*)attr->children->content, &anan);
            strcpy(temp->units, anan);
            temp->width = ret;
        }
        else if(strcmp((char*)attr->name, "height") == 0)   {
            char *anan;
            float ret = (float)strtod((char*)attr->children->content, &anan);
            strcpy(temp->units, anan);
            temp->height = ret;
        }
        else {
            insertBack(temp->otherAttributes, parseAttribute(attr));
        }
    }

    return temp;
}

//Free the attribute list and struct itself
void deleteRectangle(void* data)    {
    if(data == NULL) return;

    Rectangle *temp = (Rectangle*)data;

    freeList(temp->otherAttributes);
    free(temp);
}

//Same as before
char* rectangleToString(void* data) {
    if(data == NULL) return NULL;

    Rectangle *temp = (Rectangle*)data;
    char *attr = toString(temp->otherAttributes);
    int len = (sizeof(float)*4)+strlen(temp->units)+100+strlen(attr);

    char *tempStr = malloc(sizeof(char)*len);
    if(tempStr == NULL) return NULL;
    sprintf(tempStr, "\tRectangle:\n\t\tUnits: %s\n\t\tx = %f\n\t\ty = %f\n\t\theight = %f\n\t\twidth = %f\n%s",temp->units,temp->x,temp->y,temp->height,temp->width,attr);

    free(attr);

    return tempStr;
}

int compareRectangles(const void *first, const void *second)    {
    return 0;
}

// End of Rectangles


// Circles

Circle* setCircle(xmlNode *node)  {
    if(node == NULL) return NULL;

    //Initialize
    Circle *temp = malloc(sizeof(Circle));
    if(temp == NULL) return NULL;
    temp->units[0] = 0;
    temp->r=0;
    temp->cx=0;
    temp->cy=0;
    temp->otherAttributes = initializeList(&attributeToString, &deleteAttribute, &compareAttributes);

    for(xmlAttr *attr = node->properties; attr != NULL; attr = attr->next)  {
        if(strcmp((char*)attr->name, "cx") == 0)    {
            char *anan;
            float ret = (float)strtod((char*)attr->children->content, &anan);
            strcpy(temp->units, anan);
            temp->cx = ret;
        }
        else if(strcmp((char*)attr->name, "cy") == 0)   {
            char *anan;
            float ret = (float)strtod((char*)attr->children->content, &anan);
            strcpy(temp->units, anan);
            temp->cy = ret;
        }
        else if(strcmp((char*)attr->name, "r") == 0)   {
            char *anan;
            float ret = (float)strtod((char*)attr->children->content, &anan);
            strcpy(temp->units, anan);
            temp->r = ret;
        }
        else {
            insertBack(temp->otherAttributes, parseAttribute(attr));
        }
    }

    return temp;
}

void deleteCircle(void* data)   {
    if(data == NULL) return;

    Circle *temp = (Circle*)data;

    freeList(temp->otherAttributes);
    free(temp);
}

char* circleToString(void* data)    {
    if(data == NULL) return NULL;

    Circle *temp = (Circle*) data;
    char *attr = toString(temp->otherAttributes);
    int len = (sizeof(float)*3)+strlen(temp->units)+100+strlen(attr);

    char *tempStr = malloc(sizeof(char)*len);
    if(tempStr == NULL) return NULL;

    sprintf(tempStr, "\tCircle:\n\t\tUnits: %s\n\t\tcx = %f\n\t\tcy = %f\n\t\tr = %f\n%s",temp->units,temp->cx,temp->cy,temp->r,attr);

    free(attr);

    return tempStr;
}

int compareCircles(const void *first, const void *second)   {
    return 0;
}

// End of Circles


// Paths

Path* setPath(xmlNode *node)  {
    if(node == NULL) return NULL;

    int len = 0;
    //Get the length of data so that we can malloc enough space
    for(xmlAttr *attr = node->properties; attr != NULL; attr = attr->next)  {
        if(strcmp((char*)attr->name, "d") == 0)    {
            len = (int)strlen((char*)attr->children->content);
        }
    }

    //Initialize with malloc-ing at least 1 byte for null terminator (flexible array member)
    Path *temp = malloc(sizeof(Path) + (sizeof(char)*(len+1)));
    if(temp == NULL) return NULL;
    temp->data[0] = 0;
    temp->otherAttributes = initializeList(&attributeToString, &deleteAttribute, &compareAttributes);

    //Same thing as before
    for(xmlAttr *attr = node->properties; attr != NULL; attr = attr->next)  {
        if(strcmp((char*)attr->name, "d") == 0)    {
            strcpy(temp->data, (char*)attr->children->content);
        } else  {
            insertBack(temp->otherAttributes, parseAttribute(attr));
        }
    }

    return temp;
}

//Free the attribute list and the path itself
void deletePath(void* data) {
    if(data == NULL) return;

    Path *temp = (Path*)data;

    freeList(temp->otherAttributes);
    free(temp);
}

//Same logic as before
char* pathToString(void* data)  {
    if(data == NULL) return NULL;

    Path *temp = (Path *) data;
    char *attr = toString(temp->otherAttributes);
    int len = ((int)strlen(temp->data)+100+(int)strlen(attr));

    char *tempStr = malloc(sizeof(char)*len);
    if(tempStr == NULL) return NULL;

    sprintf(tempStr, "\tPath:\n\t\td = %s\n%s",temp->data,attr);

    free(attr);

    return tempStr;
}

int comparePaths(const void *first, const void *second) {
    return 0;
}

// End of Paths


// Groups

Group* setGroup(xmlNode *node)    {
    if(node == NULL) return NULL;

    //Initialize
    Group *temp = malloc(sizeof(Group));
    if(temp == NULL) return NULL;
    temp->otherAttributes = initializeList(&attributeToString, &deleteAttribute, &compareAttributes);
    temp->rectangles = initializeList(&rectangleToString, &deleteRectangle, &compareRectangles);
    temp->circles = initializeList(&circleToString, &deleteCircle, &compareCircles);
    temp->paths = initializeList(&pathToString, &deletePath, &comparePaths);
    temp->groups = initializeList(&groupToString, &deleteGroup, &compareGroups);

    //Set the attributes of the group first
    for(xmlAttr *attr = node->properties; attr != NULL; attr = attr->next)  {
        insertBack(temp->otherAttributes, parseAttribute(attr));
    }

    for (xmlNode* cur_node = node->children; cur_node != NULL; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if(strcmp((char*)cur_node->name, "rect") == 0) {
                insertBack(temp->rectangles, setRectangle(cur_node));
            }
            else if(strcmp((char*)cur_node->name, "circle") == 0) {
                insertBack(temp->circles, setCircle(cur_node));
            }
            else if(strcmp((char*)cur_node->name, "path") == 0) {
                insertBack(temp->paths, setPath(cur_node));
            }
            else if(strcmp((char*)cur_node->name, "g") == 0) {
                //Go to the deeper groups with recursion
                insertBack(temp->groups, setGroup(cur_node));
            }
        }
    }

    return temp;
}

//Free every list and group itself
void deleteGroup(void* data)    {
    if(data == NULL) return;

    Group *g = (Group*)data;

    freeList(g->rectangles);
    freeList(g->circles);
    freeList(g->paths);
    freeList(g->groups);
    freeList(g->otherAttributes);
    free(g);
}

//Same as before
char* groupToString(void* data) {
    if(data == NULL) return NULL;

    Group *gr = (Group*)data;

    //Get every list as strings
    char *attr = toString(gr->otherAttributes);
    char *rect = toString(gr->rectangles);
    char *circle = toString(gr->circles);
    char *path = toString(gr->paths);
    char *g = toString(gr->groups);

    int len = ((int)strlen(attr)+100+(int)strlen(rect)+(int)strlen(circle)+(int)strlen(path)+(int)strlen(g));

    char *tempStr = malloc(sizeof(char)*len);
    if(tempStr == NULL) return NULL;

    sprintf(tempStr, "\n\tGroup:\n\t--------------------\n\tAttributes:\n%s%s%s%s%s\t\n\t--------------------",attr,rect,circle,path,g);

    //Free the strings
    free(attr);
    free(rect);
    free(circle);
    free(path);
    free(g);

    return tempStr;
}

int compareGroups(const void *first, const void *second)    {
    return 0;
}

// End of Groups

//Dummy delete for getters
void dummyDelete()   {}

// Accessors
List* getRects(const SVG* img)  {
    if(img == NULL) return NULL;
    List *temp = initializeList(&rectangleToString, &dummyDelete, &compareRectangles);
    if(img->rectangles == NULL) return temp;

    genericGet(temp, img->rectangles);
    getFromGroups(temp, img->groups, 'r');

    return temp;
}

List* getCircles(const SVG* img)    {
    if(img == NULL) return NULL;
    List *temp = initializeList(&circleToString, &dummyDelete, &compareCircles);
    if(img->circles == NULL) return temp;

    genericGet(temp, img->circles);
    getFromGroups(temp, img->groups, 'c');

    return temp;
}

List* getGroups(const SVG* img) {
    if(img == NULL) return NULL;
    List *temp = initializeList(&groupToString, &dummyDelete, &compareGroups);
    if(img->groups == NULL) return temp;

    genericGet(temp, img->groups);
    getFromGroups(temp, img->groups, 'g');

    return temp;
}

List* getPaths(const SVG* img)  {
    if(img == NULL) return NULL;
    List *temp = initializeList(&pathToString, &dummyDelete, &comparePaths);
    if(img->paths == NULL) return temp;

    genericGet(temp, img->paths);
    getFromGroups(temp, img->groups, 'p');


    return temp;
}

//Generic getter to traverse iterate through the source list and populate the destination list
void genericGet(List *to, List *from)  {
    if(to == NULL || from == NULL) return;
    ListIterator itr = createIterator(from);

    void* data = nextElement(&itr);
    while (data != NULL)    {
        insertBack(to,data);
        data = nextElement(&itr);
    }
}

//To get them from inside the groups
void getFromGroups(List *to, List *from, char type)  {
    if(to == NULL || from == NULL) return;

    ListIterator itr = createIterator(from);

    Group *data = nextElement(&itr);

    //Call the generic getter depending on the type
    while (data != NULL)    {
        if(type == 'r')    {
            genericGet(to, data->rectangles);
        } else if(type == 'c')   {
            genericGet(to, data->circles);
        } else if(type == 'g')    {
            genericGet(to, data->groups);
        } else if(type == 'p')    {
            genericGet(to, data->paths);
        }

        //Go to the deeper groups with recursion if they exist
        if(data->groups != NULL)    {
            getFromGroups(to, data->groups, type);
        }

        data = nextElement(&itr);
    }
}

// End of Accessors


// Summaries

//Iterate through the rectangles list and compare the areas
int numRectsWithArea(const SVG* img, float area)    {
    if(img == NULL || area<0) return 0;

    List *temp = getRects(img);
    ListIterator itr = createIterator(temp);

    Rectangle *data = nextElement(&itr);
    int count = 0;
    while (data != NULL)    {
        float ar = data->height * data->width;
        if(ceilf(ar)==ceilf(area))  {
            count++;
        }
        data = nextElement(&itr);
    }

    freeList(temp);
    return count;
}

//Iterate through the circles list and compare the areas
int numCirclesWithArea(const SVG* img, float area)  {
    if(img == NULL || area<0) return 0;

    List *temp = getCircles(img);
    ListIterator itr = createIterator(temp);

    Circle *data = nextElement(&itr);
    int count = 0;
    while (data != NULL)    {
        float ar = 3.14159265*(data->r)*(data->r);
        if(ceilf(ar)==ceilf(area))  {
            count++;
        }
        data = nextElement(&itr);
    }

    freeList(temp);
    return count;
}

//Iterate through the paths list and compare the datas
int numPathsWithdata(const SVG* img, const char* data)  {
    if(img == NULL || (strlen(data)==0)) return 0;

    List *temp = getPaths(img);
    ListIterator itr = createIterator(temp);

    Path *dota = nextElement(&itr);
    int count = 0;
    while (dota != NULL)    {
        if(strcmp(dota->data,data) == 0)  {
            count++;
        }
        dota = nextElement(&itr);
    }

    freeList(temp);
    return count;
}

//Iterate through the groups list and compare the lengths
int numGroupsWithLen(const SVG* img, int len)   {
    if(img == NULL || len<0) return 0;

    List *temp = getGroups(img);
    ListIterator itr = createIterator(temp);

    Group *data = nextElement(&itr);
    int count = 0;
    while (data != NULL)    {
        //Get every list inside the groups and sum up their length to compare
        List *a=data->groups;
        List *b=data->circles;
        List *c=data->rectangles;
        List *d=data->paths;
        int length = getLength(a)+getLength(b)+getLength(c)+getLength(d);
        if(len == length)   {
            count++;
        }
        data = nextElement(&itr);
    }

    freeList(temp);
    return count;
}

//Return the length of attribute lists inside every type list
int lengthReturner(List* temp, char type)  {
    ListIterator itr = createIterator(temp);
    int length = 0;

    if(type=='g')   {
        Group *data = nextElement(&itr);
        while (data != NULL)    {
            length = length + getLength(data->otherAttributes);
            data = nextElement(&itr);
        }
    }
    else if(type == 'r')    {
        Rectangle *data = nextElement(&itr);
        while (data != NULL)    {
            length = length + getLength(data->otherAttributes);
            data = nextElement(&itr);
        }
    }
    else if(type == 'p')    {
        Path *data = nextElement(&itr);
        while (data != NULL)    {
            length = length + getLength(data->otherAttributes);
            data = nextElement(&itr);
        }
    }
    else if(type == 'c')    {
        Circle *data = nextElement(&itr);
        while (data != NULL)    {
            length = length + getLength(data->otherAttributes);
            data = nextElement(&itr);
        }
    }

    //Free the allocated memory from getters.
    freeList(temp);
    return length;
}

int numAttr(const SVG* img) {
    if(img == NULL) return 0;

    //Calculate the total number of attributes
    int count = 0;
    count = count + getLength(img->otherAttributes);
    count = count + lengthReturner(getGroups(img),'g');
    count = count + lengthReturner(getPaths(img),'p');
    count = count + lengthReturner(getRects(img),'r');
    count = count + lengthReturner(getCircles(img),'c');

    return count;
}

// End of Summaries


/* A2 Stuff starts here */

// Same as createSVG except checks for validity
SVG* createValidSVG(const char* fileName, const char* schemaFile)   {
    if(fileName == NULL || schemaFile == NULL) return NULL;
    if((strlen(fileName) == 0) || (strlen(schemaFile) == 0)) return NULL;

    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;

    LIBXML_TEST_VERSION

    doc = xmlReadFile(fileName, NULL, 0);

    if(doc == NULL) {
        xmlCleanupParser();
        return NULL;
    }

    //This time check for validity using the helper function
    if(!validateXML(doc, schemaFile))   {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return NULL;
    }

    root_element = xmlDocGetRootElement(doc);
    if(root_element == NULL) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return NULL;
    }

    //Simply allocate memory and initialize each element of the svg struct
    SVG *svg = malloc(sizeof(SVG));
    if(svg == NULL) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return NULL;
    }

    //Initialization
    svg->namespace[0] = 0;
    svg->description[0] = 0;
    svg->title[0] = 0;
    svg->otherAttributes = initializeList(&attributeToString, &deleteAttribute, &compareAttributes);
    svg->rectangles = initializeList(&rectangleToString, &deleteRectangle, &compareRectangles);
    svg->circles = initializeList(&circleToString, &deleteCircle, &compareCircles);
    svg->paths = initializeList(&pathToString, &deletePath, &comparePaths);
    svg->groups = initializeList(&groupToString, &deleteGroup, &compareGroups);

    //Traverse tree
    populateSVGTree(svg, root_element);

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return svg;
}

// Helper function to validate an XML tree against a schema file.
bool validateXML(xmlDoc *doc, const char* schemaFile)   {
    if(doc == NULL || schemaFile == NULL) return false;

    /* This part is from the example */
    xmlSchemaPtr schema = NULL;
    xmlSchemaParserCtxtPtr ctxt;
    xmlSchemaValidCtxtPtr validCtxt;
    int ret;

    xmlLineNumbersDefault(1);

    ctxt = xmlSchemaNewParserCtxt(schemaFile);
    if(ctxt == NULL)    {
        xmlSchemaCleanupTypes();
        return false;
    }
    xmlSchemaSetParserErrors(ctxt, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);

    schema = xmlSchemaParse(ctxt);
    if(schema == NULL)  {
        xmlSchemaFreeParserCtxt(ctxt);
        xmlSchemaCleanupTypes();
        return false;
    }

    xmlSchemaFreeParserCtxt(ctxt);

    validCtxt = xmlSchemaNewValidCtxt(schema);
    if(validCtxt == NULL)   {
        xmlSchemaFree(schema);
        xmlSchemaCleanupTypes();
        return false;
    }
    xmlSchemaSetValidErrors(validCtxt, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);

    ret = xmlSchemaValidateDoc(validCtxt, doc);
    if(ret != 0)    {
        xmlSchemaFreeValidCtxt(validCtxt);
        xmlSchemaFree(schema);
        xmlSchemaCleanupTypes();
        return false;
    }

    xmlSchemaFreeValidCtxt(validCtxt);
    xmlSchemaFree(schema);
    xmlSchemaCleanupTypes();
    return true;
}

// For writing an SVG struct into a .svg file. Assumes valid struct.
bool writeSVG(const SVG* img, const char* fileName) {
    if(img == NULL || fileName == NULL) return false;

    //Check if correct file extension
    int len = strlen(fileName);
    if(len < 5) return false;
    char temp[5];
    temp[0] = fileName[len-4];
    temp[1] = fileName[len-3];
    temp[2] = fileName[len-2];
    temp[3] = fileName[len-1];
    temp[4] = '\0';
    if(strcmp(temp, ".svg") != 0) return false;

    //Convert to libxml tree so that it can be written into a file.
    xmlDoc *doc = convertSVGtoXML(img);
    if(doc == NULL) {
        xmlCleanupParser();
        return false;
    }

    //From the example
    if(xmlSaveFormatFileEnc(fileName, doc, "UTF-8", 1) == -1) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return false;
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
    return true;
}

// Helper function for converting an SVG structure to libxml tree.
xmlDoc* convertSVGtoXML(const SVG* img)   {
    if(img == NULL) return NULL;

    /* Using the libxml tree2.c example to create an svg libxml tree */
    xmlDoc *doc;
    xmlNodePtr root;
    xmlNsPtr ns;

    LIBXML_TEST_VERSION;

    doc = xmlNewDoc(BAD_CAST "1.0");

    root = xmlNewNode(NULL, BAD_CAST "svg");
    xmlDocSetRootElement(doc, root);

    //Set the namespace
    ns = xmlNewNs(root, BAD_CAST img->namespace, NULL);
    xmlSetNs(root, ns);

    //Add the title and desc if they exist
    if(img->title[0] != 0) xmlNewChild(root, NULL, BAD_CAST "title", BAD_CAST img->title);
    if(img->description[0] != 0) xmlNewChild(root, NULL, BAD_CAST "desc", BAD_CAST img->description);

    //Add the other attributes and non-group children of svg
    writeProp(root, img->otherAttributes);
    writeChild(root, img->rectangles, img->circles, img->paths);
    writeGroup(root, img->groups);

    return doc;
}

// Turning SVG attributes into libxml props.
void writeProp(xmlNode *node, List *list)  {
    if(node == NULL || list == NULL) return;

    ListIterator itr = createIterator(list);

    Attribute *data = nextElement(&itr);
    while (data != NULL) {
        xmlNewProp(node, BAD_CAST data->name, BAD_CAST data->value);
        data = nextElement(&itr);
    }
}

// Turning rectangles, circles and paths into libxml children in the correct order.
void writeChild(xmlNode *node, List *rectangleList, List *circleList, List *pathList)  {
    if(node == NULL || rectangleList == NULL || circleList == NULL || pathList == NULL) return;

    ListIterator itr;

    //To be able to turn the floats into strings and add their units.
    char *floatStr = malloc(sizeof(char) * 1000);

    //Iterate through the rectangle list and turn them into children.
    itr = createIterator(rectangleList);
    Rectangle *rect = nextElement(&itr);
    while (rect != NULL) {
        xmlNode *temp = xmlNewChild(node, NULL, BAD_CAST "rect", NULL);

        //Turn the special attributes into props
        sprintf(floatStr, "%f", rect->x);
        //Add units if they exist.
        if(strlen(rect->units) > 0) {
            strcat(floatStr, rect->units);
        }
        xmlNewProp(temp, BAD_CAST "x", BAD_CAST floatStr);

        sprintf(floatStr, "%f", rect->y);
        if(strlen(rect->units) > 0) {
            strcat(floatStr, rect->units);
        }
        xmlNewProp(temp, BAD_CAST "y", BAD_CAST floatStr);

        sprintf(floatStr, "%f", rect->width);
        if(strlen(rect->units) > 0) {
            strcat(floatStr, rect->units);
        }
        xmlNewProp(temp, BAD_CAST "width", BAD_CAST floatStr);

        sprintf(floatStr, "%f", rect->height);
        if(strlen(rect->units) > 0) {
            strcat(floatStr, rect->units);
        }
        xmlNewProp(temp, BAD_CAST "height", BAD_CAST floatStr);

        //Turn the other attributes into props
        writeProp(temp, rect->otherAttributes);

        rect = nextElement(&itr);
    }

    //Iterate through the circle list and turn them in the children.
    itr = createIterator(circleList);
    Circle *circle = nextElement(&itr);
    while (circle != NULL) {
        xmlNode *temp = xmlNewChild(node, NULL, BAD_CAST "circle", NULL);

        //Turn the special attributes into props.
        sprintf(floatStr, "%f", circle->cx);
        //Add units if they exist.
        if(strlen(circle->units) > 0) {
            strcat(floatStr, circle->units);
        }
        xmlNewProp(temp, BAD_CAST "cx", BAD_CAST floatStr);

        sprintf(floatStr, "%f", circle->cy);
        if(strlen(circle->units) > 0) {
            strcat(floatStr, circle->units);
        }
        xmlNewProp(temp, BAD_CAST "cy", BAD_CAST floatStr);

        sprintf(floatStr, "%f", circle->r);
        if(strlen(circle->units) > 0) {
            strcat(floatStr, circle->units);
        }
        xmlNewProp(temp, BAD_CAST "r", BAD_CAST floatStr);

        //Turn the other attributes into props.
        writeProp(temp, circle->otherAttributes);

        circle = nextElement(&itr);
    }

    //Iterate through the path list and turn them into children.
    itr = createIterator(pathList);
    Path *path = nextElement(&itr);
    while (path != NULL) {
        xmlNode *temp = xmlNewChild(node, NULL, BAD_CAST "path", NULL);

        //Turn the special attribute into a prop.
        xmlNewProp(temp, BAD_CAST "d", BAD_CAST path->data);

        //Turn the other attributes into props.
        writeProp(temp, path->otherAttributes);

        path = nextElement(&itr);
    }

    free(floatStr);
}

// Recursively turning groups into children using the other functions.
void writeGroup(xmlNode *node, List *list)  {
    if(node == NULL || list == NULL) return;

    ListIterator itr = createIterator(list);

    Group *data = nextElement(&itr);
    while (data != NULL) {
        //Add the group child into the libxml tree.
        xmlNode *temp = xmlNewChild(node, NULL, BAD_CAST "g", NULL);

        //Turn the other attributes of the group into props.
        writeProp(temp, data->otherAttributes);
        //Turn the rectangles, circles and paths of the group into children.
        writeChild(temp, data->rectangles, data->circles, data->paths);
        //Go to the deeper children (will stop at the beginning of the function if NULL)
        writeGroup(temp, data->groups);

        data = nextElement(&itr);
    }
}

// Turn the SVG into libxml tree for schema checking and validate the rest using header constraints.
bool validateSVG(const SVG* img, const char* schemaFile)    {
    if(img == NULL || schemaFile == NULL) return false;
    if(strlen(schemaFile) == 0) return false;

    xmlDoc *doc = convertSVGtoXML(img);
    if(doc == NULL) {
        xmlCleanupParser();
        return false;
    }

    //Validate the tree using a schema file.
    if(!validateXML(doc, schemaFile))   {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return false;
    }

    //Validate the attributes.
    if(!validateProp(img->otherAttributes)) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return false;
    }

    //Validate rectangles, circles and paths.
    if(!validateChild(img->rectangles, img->circles, img->paths))   {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return false;
    }

    //Validate groups
    if(!validateGroup(img->groups)) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return false;
    }

    //Success if none of them failed.
    xmlFreeDoc(doc);
    xmlCleanupParser();
    return true;
}

/* Validation functions similar to write functions */

// Validate attributes against the header constraints.
bool validateProp(List *list)   {
    if(list == NULL) return false;

    ListIterator itr = createIterator(list);

    //Invalid if name is NULL
    Attribute *data = nextElement(&itr);
    while (data != NULL) {
        if(data->name == NULL) return false;
        data = nextElement(&itr);
    }

    return true;
}

// Validate rectangles, circles and paths against the header constraints.
bool validateChild(List *rectangleList, List *circleList, List *pathList)   {
    if(rectangleList == NULL || circleList == NULL || pathList == NULL) return false;

    ListIterator itr;

    //Invalid if width or height is less than 0.
    itr = createIterator(rectangleList);
    Rectangle *rect = nextElement(&itr);
    while (rect != NULL) {
        if(rect->width < 0 || rect->height < 0) return false;
        if(!validateProp(rect->otherAttributes)) return false;
        rect = nextElement(&itr);
    }

    //Invalid if radius less than 0.
    itr = createIterator(circleList);
    Circle *circle = nextElement(&itr);
    while (circle != NULL) {
        if(circle->r < 0) return false;
        if(!validateProp(circle->otherAttributes)) return false;
        circle = nextElement(&itr);
    }

    //Invalid if data is NULL.
    itr = createIterator(pathList);
    Path *path = nextElement(&itr);
    while (path != NULL) {
        if((char*)(path->data) == NULL) return false;
        if(!validateProp(path->otherAttributes)) return false;
        path = nextElement(&itr);
    }

    return true;
}

// Recursively validate groups using the other validation functions.
bool validateGroup(List *list)  {
    if(list == NULL) return false;

    ListIterator itr = createIterator(list);

    Group *data = nextElement(&itr);
    while (data != NULL) {
        if(!validateProp(data->otherAttributes)) return false;
        if(!validateChild(data->rectangles, data->circles, data->paths)) return false;
        //Go to the deeper groups (Stop recursion if at the bottom (NULL))
        if(!validateGroup(data->groups)) return false;

        data = nextElement(&itr);
    }

    return true;
}


/* Module 2 for modification stuff*/

//Generic function for setting attributes
bool setAttribute(SVG* img, elementType elemType, int elemIndex, Attribute* newAttribute)   {
    if(img == NULL || newAttribute == NULL) return false;

    switch (elemType) {
        case SVG_IMG:
            if(!modifyAttribute(img->otherAttributes, newAttribute)) {
                //If it doesn't exist, add.
                if (img->otherAttributes == NULL) return false;
                insertBack(img->otherAttributes, newAttribute);
            }
            break;
        case CIRC:
            if(!modifyCircle(img->circles, newAttribute, elemIndex)) return false;
            break;
        case RECT:
            if(!modifyRectangle(img->rectangles, newAttribute, elemIndex)) return false;
            break;
        case PATH:
            if(!modifyPath(img->paths, newAttribute, elemIndex)) return false;
            break;
        case GROUP:
            if(!modifyGroup(img->groups, newAttribute, elemIndex)) return false;
            break;
        default:
            return false;
            break;
    }

    return true;
}

//Modify the attributes list using newAttribute
bool modifyAttribute(List *list, Attribute *newAttribute)   {
    if(list == NULL || newAttribute == NULL) return false;

    ListIterator itr = createIterator(list);

    Attribute *data = nextElement(&itr);
    while (data != NULL) {
        //If attribute exists in the list, copy the new one into the old one (assume new one is smaller)
        if(strcmp(newAttribute->name, data->name) == 0) {
            strcpy(data->value, newAttribute->value);
            deleteAttribute(newAttribute);
            return true;
        }
        data = nextElement(&itr);
    }

    return false;
}

//Modify a circle at an index using the newAttribute
bool modifyCircle(List *list, Attribute *newAttribute, int elemIndex)  {
    if(list == NULL || newAttribute == NULL) return false;
    Circle *data = getAtIndex(list, elemIndex);
    if(data == NULL) return false;

    //If corresponds to a specific attribute
    if(strcmp(newAttribute->name, "cx") == 0)   {
        //Turn string into float
        float temp = strtof(newAttribute->value, NULL);
        data->cx = temp;
        deleteAttribute(newAttribute);
        return true;
    }
    else if(strcmp(newAttribute->name, "cy") == 0)  {
        float temp = strtof(newAttribute->value, NULL);
        data->cy = temp;
        deleteAttribute(newAttribute);
        return true;
    }
    else if(strcmp(newAttribute->name, "r") == 0)   {
        float temp = strtof(newAttribute->value, NULL);
        data->r = temp;
        deleteAttribute(newAttribute);
        return true;
    }

    //If one of the other attributes
    if (modifyAttribute(data->otherAttributes, newAttribute)) return true;

    //If it doesn't exist, add
    insertBack(data->otherAttributes, newAttribute);
    return true;
}

//Same logic as before
bool modifyRectangle(List *list, Attribute *newAttribute, int elemIndex)    {
    if(list == NULL || newAttribute == NULL) return false;
    Rectangle *data = getAtIndex(list, elemIndex);
    if(data == NULL) return false;

    if(strcmp(newAttribute->name, "x") == 0)   {
        float temp = strtof(newAttribute->value, NULL);
        data->x = temp;
        deleteAttribute(newAttribute);
        return true;
    }
    else if(strcmp(newAttribute->name, "y") == 0)  {
        float temp = strtof(newAttribute->value, NULL);
        data->y = temp;
        deleteAttribute(newAttribute);
        return true;
    }
    else if(strcmp(newAttribute->name, "width") == 0)   {
        float temp = strtof(newAttribute->value, NULL);
        data->width = temp;
        deleteAttribute(newAttribute);
        return true;
    }
    else if(strcmp(newAttribute->name, "height") == 0)   {
        float temp = strtof(newAttribute->value, NULL);
        data->height = temp;
        deleteAttribute(newAttribute);
        return true;
    }

    if (modifyAttribute(data->otherAttributes, newAttribute)) return true;

    insertBack(data->otherAttributes, newAttribute);
    return true;
}

//Same logic as before
bool modifyPath(List *list, Attribute *newAttribute, int elemIndex) {
    if(list == NULL || newAttribute == NULL) return false;
    Path *data = getAtIndex(list, elemIndex);
    if(data == NULL) return false;

    if(strcmp(newAttribute->name, "d") == 0)   {
        //Assumes new one is smaller
        strcpy(data->data, newAttribute->value);
        deleteAttribute(newAttribute);
        return true;
    }

    if (modifyAttribute(data->otherAttributes, newAttribute)) return true;

    insertBack(data->otherAttributes, newAttribute);
    return true;
}

//Same logic as before (no recursion)
bool modifyGroup(List *list, Attribute *newAttribute, int elemIndex)    {
    if(list == NULL || newAttribute == NULL) return false;
    Group *data = getAtIndex(list, elemIndex);
    if(data == NULL) return false;

    if(modifyAttribute(data->otherAttributes, newAttribute)) return true;

    insertBack(data->otherAttributes, newAttribute);
    return true;
}

//Helper to get the list at the index (NULL if out of bounds).
void *getAtIndex(List *list, int elemIndex)   {
    if(list == NULL) return NULL;
    if(elemIndex < 0) return NULL;

    int i = 0;
    ListIterator itr = createIterator(list);
    void *data = nextElement(&itr);
    while(i < elemIndex) {
        i++;
        data = nextElement(&itr);
    }

    return data;
}

//Simple function to append a new component to an SVG struct list
void addComponent(SVG* img, elementType type, void* newElement) {
    if(img == NULL || newElement == NULL) return;

    switch (type) {
        case CIRC:
        {
            Circle *temp = (Circle *)newElement;
            insertBack(img->circles, temp);
            break;
        }
        case RECT:
        {
            Rectangle *temp = (Rectangle *)newElement;
            insertBack(img->rectangles, temp);
            break;
        }
        case PATH:
        {
            Path *temp = (Path *)newElement;
            insertBack(img->paths, temp);
            break;
        }
        default:
            break;
    }
}


/* Module 3 for JSON stuff */
/* JSON functions are close to toString functions. */

char* attrToJSON(const Attribute *a)    {
    if(a == NULL)   {
        char *emptyJson = malloc(sizeof(char)*3);
        strcpy(emptyJson, "{}");
        return emptyJson;
    }

    int len = strlen(a->value)+strlen(a->name)+100;
    char *json = malloc(sizeof(char)*len);

    sprintf(json, "{\"name\":\"%s\",\"value\":\"%s\"}", a->name, a->value);

    return json;
}

char* circleToJSON(const Circle *c) {
    if(c == NULL)   {
        char *emptyJson = malloc(sizeof(char)*3);
        strcpy(emptyJson, "{}");
        return emptyJson;
    }

    int len = (sizeof(float)*3)+sizeof(int)+strlen(c->units)+100;
    char *json = malloc(sizeof(char)*len);

    sprintf(json, "{\"cx\":%.2f,\"cy\":%.2f,\"r\":%.2f,\"numAttr\":%d,\"units\":\"%s\"}", c->cx, c->cy, c->r, getLength(c->otherAttributes), c->units);

    return json;
}

char* rectToJSON(const Rectangle *r)    {
    if(r == NULL)   {
        char *emptyJson = malloc(sizeof(char)*3);
        strcpy(emptyJson, "{}");
        return emptyJson;
    }

    int len = (sizeof(float)*4)+sizeof(int)+strlen(r->units)+100;
    char *json = malloc(sizeof(char)*len);

    sprintf(json, "{\"x\":%.2f,\"y\":%.2f,\"w\":%.2f,\"h\":%.2f,\"numAttr\":%d,\"units\":\"%s\"}", r->x, r->y, r->width, r->height, getLength(r->otherAttributes), r->units);

    return json;
}

char* pathToJSON(const Path *p) {
    if(p == NULL)   {
        char *emptyJson = malloc(sizeof(char)*3);
        strcpy(emptyJson, "{}");
        return emptyJson;
    }

    int len = ((int)strlen(p->data)+100+(int)sizeof(int));
    char *json = malloc(sizeof(char)*len);

    sprintf(json, "{\"d\":\"%.64s\",\"numAttr\":%d}", p->data, getLength(p->otherAttributes));

    return json;
}

char* groupToJSON(const Group *g)   {
    if(g == NULL)   {
        char *emptyJson = malloc(sizeof(char)*3);
        strcpy(emptyJson, "{}");
        return emptyJson;
    }

    int len = (int)(sizeof(int)*2)+100;
    char *json = malloc(sizeof(char)*len);

    sprintf(json, "{\"children\":%d,\"numAttr\":%d}", (getLength(g->groups)+getLength(g->rectangles)+getLength(g->circles)+getLength(g->paths)), getLength(g->otherAttributes));

    return json;
}

char* SVGtoJSON(const SVG* img) {
    if(img == NULL)   {
        char *emptyJson = malloc(sizeof(char)*3);
        strcpy(emptyJson, "{}");
        return emptyJson;
    }

    int len = (int)(sizeof(int)*4)+100;
    char *json = malloc(sizeof(char)*len);

    //Get every list for their length.
    List *circ = getCircles(img);
    List *path = getPaths(img);
    List *rect = getRects(img);
    List *group = getGroups(img);

    sprintf(json, "{\"numRect\":%d,\"numCirc\":%d,\"numPaths\":%d,\"numGroups\":%d}", getLength(rect), getLength(circ), getLength(path), getLength(group));

    freeList(circ);
    freeList(path);
    freeList(rect);
    freeList(group);

    return json;
}

char* attrListToJSON(const List *list)  {
    if(list == NULL || (getLength((List*)list) == 0))   {
        char *emptyJson = malloc(sizeof(char)*3);
        strcpy(emptyJson, "[]");
        return emptyJson;
    }

    //Append the first bracket
    int len = 2;
    int i = 0;
    char *json = malloc(sizeof(char)*len);
    strcpy(json, "[");

    ListIterator itr = createIterator((List*)list);

    //Append every JSON with the commas.
    Attribute *data = nextElement(&itr);
    while(data != NULL) {
        char *temp = attrToJSON(data);
        len += strlen(temp)+1;
        json = realloc(json, sizeof(char)*len);
        strcat(json, temp);
        if(i != getLength((List*)list)-1) strcat(json, ",");
        data = nextElement(&itr);
        i++;
        free(temp);
    }

    //Append the last bracket
    len++;
    json = realloc(json, sizeof(char)*len);
    strcat(json,"]");
    return json;
}

//Same as before
char* circListToJSON(const List *list)  {
    if(list == NULL || (getLength((List*)list) == 0))   {
        char *emptyJson = malloc(sizeof(char)*3);
        strcpy(emptyJson, "[]");
        return emptyJson;
    }

    int len = 2;
    int i = 0;
    char *json = malloc(sizeof(char)*len);
    strcpy(json, "[");

    ListIterator itr = createIterator((List*)list);

    Circle *data = nextElement(&itr);
    while(data != NULL) {
        char *temp = circleToJSON(data);
        len += strlen(temp)+1;
        json = realloc(json, sizeof(char)*len);
        strcat(json, temp);
        if(i != getLength((List*)list)-1) strcat(json, ",");
        data = nextElement(&itr);
        i++;
        free(temp);
    }

    len++;
    json = realloc(json, sizeof(char)*len);
    strcat(json,"]");
    return json;
}

//Same as before
char* rectListToJSON(const List *list)  {
    if(list == NULL || (getLength((List*)list) == 0))   {
        char *emptyJson = malloc(sizeof(char)*3);
        strcpy(emptyJson, "[]");
        return emptyJson;
    }

    int len = 2;
    int i = 0;
    char *json = malloc(sizeof(char)*len);
    strcpy(json, "[");

    ListIterator itr = createIterator((List*)list);

    Rectangle *data = nextElement(&itr);
    while(data != NULL) {
        char *temp = rectToJSON(data);
        len += strlen(temp)+1;
        json = realloc(json, sizeof(char)*len);
        strcat(json, temp);
        if(i != getLength((List*)list)-1) strcat(json, ",");
        data = nextElement(&itr);
        i++;
        free(temp);
    }

    len++;
    json = realloc(json, sizeof(char)*len);
    strcat(json,"]");
    return json;
}

//Same as before
char* pathListToJSON(const List *list)  {
    if(list == NULL || (getLength((List*)list) == 0))   {
        char *emptyJson = malloc(sizeof(char)*3);
        strcpy(emptyJson, "[]");
        return emptyJson;
    }

    int len = 2;
    int i = 0;
    char *json = malloc(sizeof(char)*len);
    strcpy(json, "[");

    ListIterator itr = createIterator((List*)list);

    Path *data = nextElement(&itr);
    while(data != NULL) {
        char *temp = pathToJSON(data);
        len += strlen(temp)+1;
        json = realloc(json, sizeof(char)*len);
        strcat(json, temp);
        if(i != getLength((List*)list)-1) strcat(json, ",");
        data = nextElement(&itr);
        i++;
        free(temp);
    }

    len++;
    json = realloc(json, sizeof(char)*len);
    strcat(json,"]");
    return json;
}

//Same as before
char* groupListToJSON(const List *list) {
    if(list == NULL || (getLength((List*)list) == 0))   {
        char *emptyJson = malloc(sizeof(char)*3);
        strcpy(emptyJson, "[]");
        return emptyJson;
    }

    int len = 2;
    int i = 0;
    char *json = malloc(sizeof(char)*len);
    strcpy(json, "[");

    ListIterator itr = createIterator((List*)list);

    Group *data = nextElement(&itr);
    while(data != NULL) {
        char *temp = groupToJSON(data);
        len += strlen(temp)+1;
        json = realloc(json, sizeof(char)*len);
        strcat(json, temp);
        if(i != getLength((List*)list)-1) strcat(json, ",");
        data = nextElement(&itr);
        i++;
        free(temp);
    }

    len++;
    json = realloc(json, sizeof(char)*len);
    strcat(json,"]");
    return json;
}

char* JSONforFileLog(char *path)    {
    if(path == NULL) {
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "wtpath");
        return error;
    }

    SVG *img = createValidSVG(path, "./xsd/svg.xsd");

    if (img == NULL) {
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "invalid");
        return error;
    }

    char *json = SVGtoJSON(img);
    deleteSVG(img);

    return json;
}

char* JSONforSVGPanel(char *fpath) {
    if(fpath == NULL) {
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "wtpath");
        return error;
    }

    SVG *img = createValidSVG(fpath, "./xsd/svg.xsd");

    if (img == NULL) {
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "invalid");
        return error;
    }

    char *attr = attrListToJSON(img->otherAttributes);
    char *rect = rectListToJSON(img->rectangles);
    char *circle = circListToJSON(img->circles);
    char *path = pathListToJSON(img->paths);
    char *g = groupListToJSON(img->groups);

    int len = strlen(attr)+strlen(rect)+strlen(circle)+strlen(path)+strlen(g)+100;
    char *json = malloc(sizeof(char)*len);

    sprintf(json, "{\"rect\":%s,\"circle\":%s,\"path\":%s,\"group\":%s,\"attr\":%s}", rect, circle, path, g, attr);

    free(attr);
    free(rect);
    free(circle);
    free(path);
    free(g);
    deleteSVG(img);

    return json;
}

char *getTitle(char *path)  {
    if(path == NULL) {
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "wtpath");
        return error;
    }

    SVG *img = createValidSVG(path, "./xsd/svg.xsd");

    if (img == NULL) {
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "invalid");
        return error;
    }

    char *ret = malloc(sizeof(char)*(strlen(img->title)+1));
    strcpy(ret, img->title);
    deleteSVG(img);
    return ret;
}

char *getDesc(char *path)   {
    if(path == NULL) {
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "wtpath");
        return error;
    }

    SVG *img = createValidSVG(path, "./xsd/svg.xsd");

    if (img == NULL) {
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "invalid");
        return error;
    }

    char *ret = malloc(sizeof(char)*(strlen(img->description)+1));
    strcpy(ret, img->description);
    deleteSVG(img);
    return ret;
}

SVG* JSONtoSVG(const char* svgString)   {
    if(svgString == NULL) return NULL;
    
    SVG *svg = malloc(sizeof(SVG));

    //Initialization
    strcpy(svg->namespace, "http://www.w3.org/2000/svg");
    svg->description[0] = 0;
    svg->title[0] = 0;
    svg->otherAttributes = initializeList(&attributeToString, &deleteAttribute, &compareAttributes);
    svg->rectangles = initializeList(&rectangleToString, &deleteRectangle, &compareRectangles);
    svg->circles = initializeList(&circleToString, &deleteCircle, &compareCircles);
    svg->paths = initializeList(&pathToString, &deletePath, &comparePaths);
    svg->groups = initializeList(&groupToString, &deleteGroup, &compareGroups);

    sscanf(svgString, "{\"title\":\"%[^\"]\",\"descr\":\"%[^\"]\"}", svg->title, svg->description);

    return svg;
}



char* setDesc(char *path,char*desc)   {
    SVG *img = createValidSVG(path, "./xsd/svg.xsd");

    if (img == NULL) {
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "invalid");
        return error;
    }

    strcpy(img->description,desc);

    bool huh = validateSVG(img, "./xsd/svg.xsd");
    if(huh)    {
        writeSVG(img,path);
        deleteSVG(img);
        char *succ = malloc(sizeof(char)*8);
        strcpy(succ, "success");
        return succ;
    }else {
        deleteSVG(img);
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "invalid");
        return error;
    }
}

char* setTitle(char *path, char*title)  {
    SVG *img = createValidSVG(path, "./xsd/svg.xsd");

    if (img == NULL) {
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "invalid");
        return error;
    }

    strcpy(img->title,title);

    bool huh = validateSVG(img, "./xsd/svg.xsd");
    if(huh)    {
        writeSVG(img,path);
        deleteSVG(img);
        char *succ = malloc(sizeof(char)*8);
        strcpy(succ, "success");
        return succ;
    }else {
        deleteSVG(img);
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "invalid");
        return error;
    }
}




char* addFile(const char* svgString, char* path) {
    SVG *svg = JSONtoSVG(svgString);
    bool huh = validateSVG(svg, "./xsd/svg.xsd");
    if(huh)    {
        writeSVG(svg,path);
        deleteSVG(svg);
        char *succ = malloc(sizeof(char)*8);
        strcpy(succ, "success");
        return succ;
    }else {
        deleteSVG(svg);
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "invalid");
        return error;
    }
}

Rectangle* JSONtoRect(const char* svgString){
    if(svgString == NULL) return NULL;

    Rectangle *temp = malloc(sizeof(Rectangle));

    //Initialization
    temp->units[0] = 0;
    temp->width=0;
    temp->height=0;
    temp->x=0;
    temp->y=0;
    temp->otherAttributes = initializeList(&attributeToString, &deleteAttribute, &compareAttributes);

    sscanf(svgString, "{\"x\":%f,\"y\":%f,\"w\":%f,\"h\":%f,\"units\":\"%[^\"]\"}", &(temp->x), &(temp->y), &(temp->width), &(temp->height), temp->units);

    return temp;
}

Circle* JSONtoCircle(const char* svgString){
    if(svgString == NULL) return NULL;

    Circle *temp = malloc(sizeof(Circle));

    //Initialization
    temp->units[0] = 0;
    temp->cx=0;
    temp->cy=0;
    temp->r=0;
    temp->otherAttributes = initializeList(&attributeToString, &deleteAttribute, &compareAttributes);

    sscanf(svgString, "{\"cx\":%f,\"cy\":%f,\"r\":%f,\"units\":\"%[^\"]\"}", &(temp->cx), &(temp->cy), &(temp->r), temp->units);

    return temp;
}


char* shapeAttributes(char *path, char *type, char *index)  {
    SVG *img = createValidSVG(path, "./xsd/svg.xsd");
    if (img == NULL) {
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "invalid");
        return error;
    }

    char *json = NULL;
    int i = atoi(index);
    if(strcmp("Circle",type) == 0)  {
        Circle *temp = getAtIndex(img->circles,i);
        json = attrListToJSON(temp->otherAttributes);
    }
    else if(strcmp("Rectangle",type) == 0) {
        Rectangle *temp = getAtIndex(img->rectangles,i);
        json = attrListToJSON(temp->otherAttributes);

    }
    else if(strcmp("Path",type) == 0) {
        Path *temp = getAtIndex(img->paths,i);
        json = attrListToJSON(temp->otherAttributes);
        
    }
    else if(strcmp("Group",type) == 0) {
        Group *temp = getAtIndex(img->groups,i);
        json = attrListToJSON(temp->otherAttributes);
        
    }

    deleteSVG(img);
    return json;
}

char* setAttributeWrapper(char *path, char *name, char *value, char *type, char *index) {
    SVG *img = createValidSVG(path, "./xsd/svg.xsd");
    if (img == NULL) {
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "invalif");
        return error;
    }

    int i = atoi(index);
    bool sugg = false;

    Attribute *temp = malloc(sizeof(Attribute) + sizeof(char)*(strlen(value)+1));
    temp->name = malloc(sizeof(char) * (strlen(name)+1));
    strcpy(temp->name,name);
    strcpy(temp->value,value);

    if(strcmp("Circle",type) == 0) sugg = setAttribute(img,CIRC,i,temp);
    else if(strcmp("SVG",type) == 0) sugg = setAttribute(img,SVG_IMG,i,temp);
    else if(strcmp("Rectangle",type) == 0) sugg = setAttribute(img,RECT,i,temp);
    else if(strcmp("Path",type) == 0) sugg = setAttribute(img,PATH,i,temp);
    else if(strcmp("Group",type) == 0) sugg = setAttribute(img,GROUP,i,temp);

    if(sugg == false) {
        deleteSVG(img);
        deleteAttribute(temp);
        char *error = malloc(sizeof(char)*7);
        strcpy(error, "failed");
        return error;
    }
    
    bool huh = validateSVG(img,"./xsd/svg.xsd");
    if(huh)    {
        writeSVG(img,path);
        deleteSVG(img);
        char *succ = malloc(sizeof(char)*8);
        strcpy(succ, "success");
        return succ;
    }else {
        deleteSVG(img);
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "invalid");
        return error;
    }
}

char* addComponentWrapper(char *path, char*json,char*type)  {
    SVG *img = createValidSVG(path, "./xsd/svg.xsd");
    if (img == NULL) {
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "invalif");
        return error;
    }

    if(strcmp("circ",type) == 0)    {
        Circle *temp = JSONtoCircle(json);
        addComponent(img,CIRC,temp);
    }
    else if(strcmp("rect",type) == 0)   {
        Rectangle *temp = JSONtoRect(json);
        addComponent(img,RECT,temp);
    }

    bool huh = validateSVG(img,"./xsd/svg.xsd");
    if(huh)    {
        writeSVG(img,path);
        deleteSVG(img);
        char *succ = malloc(sizeof(char)*8);
        strcpy(succ, "success");
        return succ;
    }else {
        deleteSVG(img);
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "invalid");
        return error;
    }
}

char* scaleShapes(char *path, char *scale, char *type)  {
    SVG *img = createValidSVG(path, "./xsd/svg.xsd");
    if (img == NULL) {
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "invalif");
        return error;
    }

    float factor = strtof(scale,NULL);

    if(strcmp("Circles",type) == 0)    {
        List *temp = getCircles(img);
        ListIterator itr = createIterator(temp);
        Circle *data = nextElement(&itr);
        while (data != NULL) {
            data->r = (factor*(data->r));
            data = nextElement(&itr);
        }
        freeList(temp);
    }
    else if(strcmp("Rectangles",type) == 0)   {
        List *temp = getRects(img);

        ListIterator itr = createIterator(temp);
        Rectangle *data = nextElement(&itr);
        while (data != NULL) {
            data->height = (factor*(data->height));
            data->width = (factor*(data->width));
            data = nextElement(&itr);
        }
        freeList(temp);
    }

    bool huh = validateSVG(img,"./xsd/svg.xsd");
    if(huh)    {
        writeSVG(img,path);
        deleteSVG(img);
        char *succ = malloc(sizeof(char)*8);
        strcpy(succ, "success");
        return succ;
    }else {
        deleteSVG(img);
        char *error = malloc(sizeof(char)*8);
        strcpy(error, "invalid");
        return error;
    }
}
