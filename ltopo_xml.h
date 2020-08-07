#ifndef LTOPO_XML_H
#define LTOPO_XML_H

#include "ltopo_list.h"
int ltopo_xml_load_file(char * xml);
int ltopo_xml_save_file(LTOPO_LIST* head, const char *filename);
int ltopo_xml_save_ev_list(const char *filename, LTOPO_LIST * head);

#endif
