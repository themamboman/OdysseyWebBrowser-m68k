/* expat_stub.c - Minimal stub implementation for cross-compilation */
#include "expat.h"
#include <stdlib.h>
#include <string.h>

struct XML_ParserStruct {
    int dummy;
};

XML_Parser XML_ParserCreate(const char *encoding) {
    (void)encoding;
    return calloc(1, sizeof(struct XML_ParserStruct));
}

XML_Parser XML_ParserCreateNS(const char *encoding, char nsSep) {
    (void)encoding;
    (void)nsSep;
    return calloc(1, sizeof(struct XML_ParserStruct));
}

void XML_ParserFree(XML_Parser parser) {
    free(parser);
}

XML_Status XML_Parse(XML_Parser parser, const char *buf, int len, int isFinal) {
    (void)parser;
    (void)buf;
    (void)len;
    (void)isFinal;
    return XML_STATUS_OK;
}

XML_Status XML_ParseBuffer(XML_Parser parser, int len, int isFinal) {
    (void)parser;
    (void)len;
    (void)isFinal;
    return XML_STATUS_OK;
}

void *XML_GetBuffer(XML_Parser parser, int len) {
    (void)parser;
    (void)len;
    return malloc(len > 0 ? len : 1);
}

XML_Error XML_GetErrorCode(XML_Parser parser) {
    (void)parser;
    return XML_ERROR_NONE;
}

const char *XML_ErrorString(XML_Error code) {
    (void)code;
    return "No error";
}

long XML_GetCurrentLineNumber(XML_Parser parser) {
    (void)parser;
    return 1;
}

long XML_GetCurrentColumnNumber(XML_Parser parser) {
    (void)parser;
    return 1;
}

long XML_GetCurrentByteIndex(XML_Parser parser) {
    (void)parser;
    return 0;
}

int XML_GetCurrentByteCount(XML_Parser parser) {
    (void)parser;
    return 0;
}

void XML_SetElementHandler(XML_Parser parser, XML_StartElementHandler start, XML_EndElementHandler end) {
    (void)parser;
    (void)start;
    (void)end;
}

void XML_SetCharacterDataHandler(XML_Parser parser, XML_CharacterDataHandler handler) {
    (void)parser;
    (void)handler;
}

void XML_SetProcessingInstructionHandler(XML_Parser parser, XML_ProcessingInstructionHandler handler) {
    (void)parser;
    (void)handler;
}

void XML_SetCommentHandler(XML_Parser parser, XML_CommentHandler handler) {
    (void)parser;
    (void)handler;
}

void XML_SetCdataSectionHandler(XML_Parser parser, XML_StartCdataSectionHandler start, XML_EndCdataSectionHandler end) {
    (void)parser;
    (void)start;
    (void)end;
}

void XML_SetDefaultHandler(XML_Parser parser, XML_DefaultHandler handler) {
    (void)parser;
    (void)handler;
}

void XML_SetDoctypeDeclHandler(XML_Parser parser, XML_StartDoctypeDeclHandler start, XML_EndDoctypeDeclHandler end) {
    (void)parser;
    (void)start;
    (void)end;
}

void XML_SetEntityDeclHandler(XML_Parser parser, XML_EntityDeclHandler handler) {
    (void)parser;
    (void)handler;
}

void XML_SetUnparsedEntityDeclHandler(XML_Parser parser, XML_UnparsedEntityDeclHandler handler) {
    (void)parser;
    (void)handler;
}

void XML_SetNotationDeclHandler(XML_Parser parser, XML_NotationDeclHandler handler) {
    (void)parser;
    (void)handler;
}

void XML_SetStartNamespaceDeclHandler(XML_Parser parser, XML_StartNamespaceDeclHandler handler) {
    (void)parser;
    (void)handler;
}

void XML_SetEndNamespaceDeclHandler(XML_Parser parser, XML_EndNamespaceDeclHandler handler) {
    (void)parser;
    (void)handler;
}

void XML_SetNotStandaloneHandler(XML_Parser parser, XML_NotStandaloneHandler handler) {
    (void)parser;
    (void)handler;
}

void XML_SetExternalEntityRefHandler(XML_Parser parser, XML_ExternalEntityRefHandler handler) {
    (void)parser;
    (void)handler;
}

void XML_SetUserData(XML_Parser parser, void *userData) {
    (void)parser;
    (void)userData;
}

void *XML_GetUserData(XML_Parser parser) {
    (void)parser;
    return NULL;
}

XML_Status XML_SetEncoding(XML_Parser parser, const char *encoding) {
    (void)parser;
    (void)encoding;
    return XML_STATUS_OK;
}

XML_Status XML_UseParserAsHandlerArg(XML_Parser parser) {
    (void)parser;
    return XML_STATUS_OK;
}

XML_Status XML_SetParamEntityParsing(XML_Parser parser, enum XML_ParamEntityParsing parsing) {
    (void)parser;
    (void)parsing;
    return XML_STATUS_OK;
}

int XML_GetSpecifiedAttributeCount(XML_Parser parser) {
    (void)parser;
    return 0;
}

int XML_GetIdAttributeIndex(XML_Parser parser) {
    (void)parser;
    return -1;
}

void XML_SetReturnNSTriplet(XML_Parser parser, int do_nst) {
    (void)parser;
    (void)do_nst;
}