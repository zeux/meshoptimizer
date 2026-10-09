/*Licensed under MIT or Public Domain. See bottom of file for details.

micro_ply.h
	Written by Nick Klingensmith, @koujaku on Twitter, @maluoi on GitHub

	This is a small ASCII and binary little-endian .ply loader and converter.
	It's intended to be as short as possible, while still being easily
	readable! The idea is that it can be trivially embedded in another single
	header library or single file project without too much trouble. It
	compiles as both C and C++.

Example usage:

	// micro_ply.h follows stb library conventions, so MICRO_PLY_IMPL must
	// be defined before include in only one file in your project!
	#define MICRO_PLY_IMPL
	#include "micro_ply.h"

	// This is the vertex layout we'll be converting the PLY to.
	typedef struct skg_vert_t {
		float   pos [3];
		float   norm[3];
		float   uv  [2];
		uint8_t col [4];
	} skg_vert_t;

	// Read the data from file, that's still on you, sorry :)
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL) return false;
	fseek(fp, 0L, SEEK_END);
	size_t size = ftell(fp);
	rewind(fp);
	void *data = malloc(size);
	fread (data, size, 1, fp);
	fclose(fp);

	// Parse the data using ply_read
	ply_file_t file;
	if (!ply_read(data, size, &file))
		return false;

	// Describe the way the contents of the PLY file map to our own vertex
	// format. If the property can't be found in the file, the default value
	// will be assigned. A NULL default leaves that memory untouched.
	float     fzero = 0;
	uint8_t   white = 255;
	ply_map_t map_verts[] = {
		{ PLY_PROP_POSITION_X,  ply_prop_decimal, sizeof(float), 0,  &fzero },
		{ PLY_PROP_POSITION_Y,  ply_prop_decimal, sizeof(float), 4,  &fzero },
		{ PLY_PROP_POSITION_Z,  ply_prop_decimal, sizeof(float), 8,  &fzero },
		{ PLY_PROP_NORMAL_X,    ply_prop_decimal, sizeof(float), 12, &fzero },
		{ PLY_PROP_NORMAL_Y,    ply_prop_decimal, sizeof(float), 16, &fzero },
		{ PLY_PROP_NORMAL_Z,    ply_prop_decimal, sizeof(float), 20, &fzero },
		{ PLY_PROP_TEXCOORD_X,  ply_prop_decimal, sizeof(float), 24, &fzero },
		{ PLY_PROP_TEXCOORD_Y,  ply_prop_decimal, sizeof(float), 28, &fzero },
		{ PLY_PROP_COLOR_R,     ply_prop_uint,    sizeof(uint8_t), 32, &white },
		{ PLY_PROP_COLOR_G,     ply_prop_uint,    sizeof(uint8_t), 33, &white },
		{ PLY_PROP_COLOR_B,     ply_prop_uint,    sizeof(uint8_t), 34, &white },
		{ PLY_PROP_COLOR_A,     ply_prop_uint,    sizeof(uint8_t), 35, &white }, };
	ply_convert(&file, PLY_ELEMENT_VERTICES, map_verts, sizeof(map_verts)/sizeof(map_verts[0]), sizeof(skg_vert_t), (void **)out_verts, out_vert_count);

	// Properties defined as lists in the PLY format will get triangulated
	// during conversion, so you don't need to worry about quads or n-gons in
	// the geometry.
	uint32_t  izero = 0;
	ply_map_t map_inds[] = { { PLY_PROP_INDICES, ply_prop_uint, sizeof(uint32_t), 0, &izero } };
	ply_convert(&file, PLY_ELEMENT_FACES, map_inds, sizeof(map_inds)/sizeof(map_inds[0]), sizeof(uint32_t), (void **)out_indices, out_ind_count);

	// You gotta free the memory manually!
	ply_free(&file);
	free(data);

	// Text decimals are parsed with atof, which honors LC_NUMERIC. Define
	// MICRO_PLY_ATOF(str) before including to use your own parser instead.
*/

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define PLY_PROP_POSITION_X "x"
#define PLY_PROP_POSITION_Y "y"
#define PLY_PROP_POSITION_Z "z"
#define PLY_PROP_NORMAL_X "nx"
#define PLY_PROP_NORMAL_Y "ny"
#define PLY_PROP_NORMAL_Z "nz"
#define PLY_PROP_TEXCOORD_X "s"
#define PLY_PROP_TEXCOORD_Y "t"
#define PLY_PROP_COLOR_R "red"
#define PLY_PROP_COLOR_G "green"
#define PLY_PROP_COLOR_B "blue"
#define PLY_PROP_COLOR_A "alpha"
#define PLY_PROP_COLOR_DIFF_R "diffuse_red"
#define PLY_PROP_COLOR_DIFF_G "diffuse_green"
#define PLY_PROP_COLOR_DIFF_B "diffuse_blue"
#define PLY_PROP_COLOR_DIFF_A "diffuse_alpha"
#define PLY_PROP_INDICES "vertex_index"
#define PLY_ELEMENT_VERTICES "vertex"
#define PLY_ELEMENT_FACES "face"

///////////////////////////////////////////

typedef enum ply_prop_ {
	ply_prop_int = 1,
	ply_prop_uint,
	ply_prop_decimal,
} ply_prop_;

typedef struct ply_prop_t {
	uint8_t  bytes;
	uint8_t  type; // follows ply_prop_
	uint8_t  list_bytes;
	uint8_t  list_type;
	uint16_t offset;
	char     name[32];
} ply_prop_t;

typedef struct ply_element_t {
	char        name[64];
	int32_t     count;
	ply_prop_t *properties;
	int32_t     property_count;
	void       *data;
	int32_t     data_stride;
	void       *list_data;
} ply_element_t;

typedef struct ply_file_t {
	ply_element_t *elements;
	int32_t        count;
} ply_file_t;

typedef struct ply_map_t {
	const char *name;
	uint8_t     to_type;
	uint8_t     to_size;
	uint16_t    to_offset;
	const void *default_val;
} ply_map_t;

///////////////////////////////////////////

bool ply_read   (const void *data, size_t data_size, ply_file_t *out_file);
void ply_free   (ply_file_t *file);
void ply_convert(const ply_file_t *file, const char *element_name, const ply_map_t *to_format, int32_t to_format_count, int32_t format_stride, void **out_data, int32_t *out_count);

///////////////////////////////////////////

#ifdef MICRO_PLY_IMPL

// atof honors LC_NUMERIC, so define this to swap in a locale-independent parser
#ifndef MICRO_PLY_ATOF
#define MICRO_PLY_ATOF(str) atof(str)
#endif

///////////////////////////////////////////

typedef enum ply_fmt_ {
	ply_fmt_ascii = 1,
	ply_fmt_binary_le,
	ply_fmt_binary_be,
} ply_fmt_;

///////////////////////////////////////////

static void _ply_convert(uint8_t *dest, uint8_t dest_size, uint8_t dest_type, const uint8_t *src, uint8_t src_size, uint8_t src_type) {
	if (dest_size == src_size && src_type == dest_type) { memcpy(dest, src, dest_size); return; }

	// memcpy rather than pointer casts, binary file data isn't aligned
	double  dval = 0;
	int64_t ival = 0;
	if (src_type == ply_prop_decimal) {
		if (src_size == 4) { float f; memcpy(&f, src, 4); dval = f; }
		else               { memcpy(&dval, src, 8); }
	} else if (src_type == ply_prop_int) {
		switch (src_size) {
		case 1: { int8_t   v; memcpy(&v, src, 1); ival = v; } break;
		case 2: { int16_t  v; memcpy(&v, src, 2); ival = v; } break;
		case 4: { int32_t  v; memcpy(&v, src, 4); ival = v; } break;
		case 8: { int64_t  v; memcpy(&v, src, 8); ival = v; } break; }
	} else {
		switch (src_size) {
		case 1: { uint8_t  v; memcpy(&v, src, 1); ival = v; } break;
		case 2: { uint16_t v; memcpy(&v, src, 2); ival = v; } break;
		case 4: { uint32_t v; memcpy(&v, src, 4); ival = v; } break;
		case 8: { uint64_t v; memcpy(&v, src, 8); ival = (int64_t)v; } break; }
	}

	if (dest_type == ply_prop_decimal) {
		if (src_type != ply_prop_decimal) dval = (double)ival;
		if (dest_size == 4) { float f = (float)dval; memcpy(dest, &f, 4); }
		else                { memcpy(dest, &dval, 8); }
	} else {
		// int and uint truncate to the same bits, so only size matters
		if (src_type == ply_prop_decimal) ival = (int64_t)dval;
		switch (dest_size) {
		case 1: { uint8_t  v = (uint8_t )ival; memcpy(dest, &v, 1); } break;
		case 2: { uint16_t v = (uint16_t)ival; memcpy(dest, &v, 2); } break;
		case 4: { uint32_t v = (uint32_t)ival; memcpy(dest, &v, 4); } break; }
	}
}

///////////////////////////////////////////

static bool _ply_starts_with(const char *str, const char *prefix) {
	while (*prefix) {
		if (*prefix++ != *str++)
			return false;
	}
	return true;
}

static void _ply_get_word(const char *start, char *out, size_t out_size) {
	size_t count = 0;
	while (*start != ' ' && *start != '\t' && *start != '\n' && *start != '\r' && *start != '\0' && count+1 < out_size) {
		out[count] = *start++;
		count++;
	}
	out[count] = '\0';
}

static void _ply_type_info(const char *str, uint8_t *type, uint8_t *bytes) {
	if      (strcmp(str, "char"  ) == 0) { *bytes = 1; *type = ply_prop_int;     }
	else if (strcmp(str, "uchar" ) == 0) { *bytes = 1; *type = ply_prop_uint;    }
	else if (strcmp(str, "short" ) == 0) { *bytes = 2; *type = ply_prop_int;     }
	else if (strcmp(str, "ushort") == 0) { *bytes = 2; *type = ply_prop_uint;    }
	else if (strcmp(str, "int"   ) == 0) { *bytes = 4; *type = ply_prop_int;     }
	else if (strcmp(str, "uint"  ) == 0) { *bytes = 4; *type = ply_prop_uint;    }
	else if (strcmp(str, "float" ) == 0) { *bytes = 4; *type = ply_prop_decimal; }
	else if (strcmp(str, "double") == 0) { *bytes = 8; *type = ply_prop_decimal; }
}

bool ply_read(const void *file_data, size_t data_size, ply_file_t *out_file) {
	(void)data_size; // unused for now

	// Check file signature
	char *file = (char*)file_data;
	if (!_ply_starts_with(file, "ply"))
		return false;

	// File data
	ply_fmt_ format    = ply_fmt_ascii;
	out_file->count    = 0;
	out_file->elements = NULL;

	// Read the header
	char *line = strchr(file, '\n');
	char  word[128];
	while(true) {
		if (!line) return false;
		line += 1;

		if (_ply_starts_with(line, "format ")) {
			_ply_get_word(line + sizeof("format"), word, sizeof(word));
			if      (strcmp(word, "ascii"               ) == 0) format = ply_fmt_ascii;
			else if (strcmp(word, "binary_little_endian") == 0) format = ply_fmt_binary_le;
			else if (strcmp(word, "binary_big_endian"   ) == 0) format = ply_fmt_binary_be;
		} else if (_ply_starts_with(line, "comment ")) {
		} else if (_ply_starts_with(line, "element ")) {
			ply_element_t el;
			memset(&el, 0, sizeof(el));
			_ply_get_word(line + sizeof("element"), el.name, sizeof(el.name));
			_ply_get_word(line + sizeof("element ") + strlen(el.name), word, sizeof(word));
			el.count = atoi(word);

			out_file->count   += 1;
			out_file->elements = (ply_element_t*)realloc(out_file->elements, sizeof(ply_element_t) * (out_file->count));
			out_file->elements[out_file->count - 1] = el;
		} else if (_ply_starts_with(line, "property ")) {
			ply_prop_t prop;
			memset(&prop, 0, sizeof(prop));
			_ply_get_word(line + sizeof("property"), word, sizeof(word));

			if (strcmp(word, "list"  ) == 0) {
				size_t off = sizeof("property ") + strlen(word);
				_ply_get_word(line + off, word, sizeof(word));
				_ply_type_info(word, &prop.type, &prop.bytes);
				off += strlen(word) + 1;
				_ply_get_word(line + off, word, sizeof(word));
				_ply_type_info(word, &prop.list_type, &prop.list_bytes);
				off += strlen(word) + 1;
				_ply_get_word(line + off, prop.name, sizeof(prop.name));
			} else {
				_ply_type_info(word, &prop.type, &prop.bytes);
				_ply_get_word(line + sizeof("property ") + strlen(word), prop.name, sizeof(prop.name));
			}

			ply_element_t *el   = &out_file->elements[out_file->count-1];
			prop.offset = (uint16_t)el->data_stride;
			el->data_stride    += prop.bytes;
			el->property_count += 1;
			el->properties      = (ply_prop_t*)realloc(el->properties, sizeof(ply_prop_t) * el->property_count);
			el->properties[el->property_count-1] = prop;
		} else if (_ply_starts_with(line, "end_header")) {
			line = strchr(line, '\n')+1;
			break;
		}
		line = strchr(line, '\n');
	}

	if (format == ply_fmt_binary_be) {
		ply_free(out_file);
		return false;
	}

	// Parse the data
	for (int32_t i = 0; i < out_file->count; i++) {
		ply_element_t *el = &out_file->elements[i];
		el->data = malloc((size_t)el->data_stride * (size_t)el->count);
		if (el->data == NULL) {
			ply_free(out_file);
			return false;
		}
		uint8_t *data = (uint8_t*)el->data;

		// If it's a list type
		if (el->property_count == 1 && el->properties[0].list_type != 0) {
			const ply_prop_t* p = &el->properties[0];
			int32_t list_cap   = el->count * 4;
			int32_t list_count = 0;
			el->list_data = malloc(p->list_bytes * list_cap);
			uint8_t *list_data = (uint8_t*)el->list_data;

			if (format == ply_fmt_ascii) {
				for (int32_t e = 0; e < el->count; e++) {
					size_t off = 0;
					_ply_get_word(line + off, word, sizeof(word));
					off += strlen(word) + 1;
					int32_t count = atoi(word);
					_ply_convert(data, p->bytes, p->type, (uint8_t*)&count, sizeof(count), ply_prop_int);
					for (int32_t c = 0; c < count; c++) {
						_ply_get_word(line + off, word, sizeof(word));
						off += strlen(word) + 1;
						if      (p->list_type == ply_prop_uint)    { uint64_t val = atol(word); _ply_convert(list_data, p->list_bytes, p->list_type, (uint8_t*)&val, sizeof(uint64_t), ply_prop_uint   ); }
						else if (p->list_type == ply_prop_decimal) { double   val = MICRO_PLY_ATOF(word); _ply_convert(list_data, p->list_bytes, p->list_type, (uint8_t*)&val, sizeof(double  ), ply_prop_decimal); }
						else                                       { int64_t  val = atol(word); _ply_convert(list_data, p->list_bytes, p->list_type, (uint8_t*)&val, sizeof(int64_t ), ply_prop_int    ); }
						list_data  += p->list_bytes;
						list_count += 1;
						if (list_count >= list_cap) {
							list_cap = (int32_t)(list_cap * 1.25f);
							el->list_data = realloc(el->list_data, p->list_bytes * list_cap);
							list_data = ((uint8_t*)el->list_data) + (list_count * p->list_bytes);
						}
					}
					line = strchr(line, '\n') + 1;
					data += p->bytes;
				}
			} else if (format == ply_fmt_binary_le) {
				for (int32_t e = 0; e < el->count; e++) {
					// Get the count of items in this element
					memcpy(data, line, p->bytes);
					int32_t count = 0;
					_ply_convert((uint8_t*)&count, sizeof(count), ply_prop_int, data, p->bytes, p->type);
					line += p->bytes;
					data += p->bytes;

					// Make sure we have room for the elements
					int32_t needed = list_count + count;
					if (needed > list_cap) {
						list_cap      = needed > list_cap * 2 ? needed : list_cap * 2;
						el->list_data = realloc(el->list_data, (size_t)p->list_bytes * list_cap);
						list_data     = (uint8_t*)el->list_data + (size_t)list_count * p->list_bytes;
					}
					// Copy the elements
					memcpy(list_data, line, p->list_bytes * count);
					list_data  += p->list_bytes * count;
					line       += p->list_bytes * count;
					list_count += count;
				}
			}
		} else {
			if (format == ply_fmt_ascii) {
				for (int32_t e = 0; e < el->count; e++) {
					size_t off = 0;
					for (int32_t prop = 0; prop < el->property_count; prop++) {
						const ply_prop_t* p = &el->properties[prop];
						_ply_get_word(line + off, word, sizeof(word));
						off += strlen(word) + 1;
						if      (p->type == ply_prop_decimal) { double   val = MICRO_PLY_ATOF(word); _ply_convert(data+p->offset, p->bytes, p->type, (uint8_t*)&val, sizeof(double  ), ply_prop_decimal); }
						else if (p->type == ply_prop_int)     { int64_t  val = atol(word); _ply_convert(data+p->offset, p->bytes, p->type, (uint8_t*)&val, sizeof(int64_t ), ply_prop_int    ); }
						else                                  { uint64_t val = atol(word); _ply_convert(data+p->offset, p->bytes, p->type, (uint8_t*)&val, sizeof(uint64_t), ply_prop_uint   ); }
					}
					line = strchr(line, '\n') + 1;
					data += el->data_stride;
				}
			} else if (format == ply_fmt_binary_le) {
				size_t copy = el->data_stride * el->count;
				memcpy(data, line, copy);
				data += copy;
				line += copy;
			}
		}
	}

	return true;
}

///////////////////////////////////////////

void ply_convert(const ply_file_t *file, const char *element_name, const ply_map_t *to_format, int32_t format_count, int32_t format_stride, void **out_data, int32_t *out_count) {
	*out_data  = NULL;
	*out_count = 0;

	// Find the elements we want to convert by name
	const ply_element_t *elements = NULL;
	for (int32_t i = 0; i < file->count; i++) {
		if (strcmp(file->elements[i].name, element_name) == 0) {
			elements = &file->elements[i];
			break;
		}
	}
	if (elements == NULL)
		return;

	if (elements->list_data == NULL) {
		// Map the element properties to the out format
		int32_t *map  = (int32_t*)malloc(sizeof(int32_t) * format_count);
		for (int32_t i = 0; i < format_count; i++) {
			map[i] = -1;
			for (int32_t p = 0; p < elements->property_count; p++) {
				if (strcmp(elements->properties[p].name, to_format[i].name) == 0) {
					map[i] = p;
					break;
				}
			}
		}

		// Now convert and copy each item
		// Use size_t to avoid integer overflow for large files (>7M vertices)
		*out_data  = malloc((size_t)elements->count * (size_t)format_stride);
		if (*out_data == NULL) {
			free(map);
			return;
		}
		*out_count = elements->count;
		uint8_t *src  = (uint8_t*)elements->data;
		uint8_t *dest = (uint8_t*)*out_data;
		for (int32_t i = 0; i < elements->count; i++) {
			for (int32_t f = 0; f < format_count; f++) {
				if (map[f] == -1) {
					if (to_format[f].default_val)
						memcpy(dest+to_format[f].to_offset, to_format[f].default_val, to_format[f].to_size);
				} else {
					ply_prop_t *prop = &elements->properties[map[f]];
					_ply_convert(
						dest+to_format[f].to_offset, to_format[f].to_size, to_format[f].to_type,
						src+prop->offset,            prop->bytes,          prop->type);
				}
			}
			src  += elements->data_stride;
			dest += format_stride;
		}

		free(map);
	} else {
		uint8_t src_size     = elements->properties[0].bytes;
		uint8_t src_type     = elements->properties[0].type;
		uint8_t src_ind_size = elements->properties[0].list_bytes;
		uint8_t src_ind_type = elements->properties[0].list_type;
		// Count how many total indices there will be
		int32_t count = 0;
		uint8_t *src = (uint8_t *)elements->data;
		for (int32_t i = 0; i < elements->count; i++) {
			int32_t ct = 0;
			_ply_convert((uint8_t *)&ct, sizeof(int32_t), ply_prop_int, src, src_size, src_type);
			count += 3 + (ct-3)*3;
			src   += src_size;
		}

		*out_data  = malloc((size_t)count * (size_t)format_stride);
		if (*out_data == NULL) {
			return;
		}
		*out_count = count;
		src              = (uint8_t*)elements->data;
		uint8_t *src_ind = (uint8_t*)elements->list_data;
		uint8_t *dest    = (uint8_t*)*out_data;
		for (int32_t i = 0; i < elements->count; i++) {
			int32_t ct = 0;
			_ply_convert((uint8_t *)&ct, sizeof(int32_t), ply_prop_int, src, src_size, src_type);
			for (int32_t x = 0; x < ct-2; x++) {
				_ply_convert(
					dest+(x*3*to_format[0].to_size), to_format[0].to_size, to_format[0].to_type,
					src_ind, src_ind_size, src_ind_type);
				_ply_convert(
					dest+((x*3+1)*to_format[0].to_size), to_format[0].to_size, to_format[0].to_type,
					src_ind+((x+1)*src_ind_size), src_ind_size, src_ind_type);
				_ply_convert(
					dest+((x*3+2)*to_format[0].to_size), to_format[0].to_size, to_format[0].to_type,
					src_ind+((x+2)*src_ind_size), src_ind_size, src_ind_type);
			}
			src_ind += src_ind_size * ct;
			dest    += format_stride * (3 + (ct-3)*3);
			src     += src_size;
		}
	}
}

///////////////////////////////////////////

void ply_free(ply_file_t *file) {
	for (int32_t i = 0; i < file->count; i++) {
		free(file->elements[i].data);
		free(file->elements[i].list_data);
		free(file->elements[i].properties);
	}
	free(file->elements);
}

#endif

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2020 Nick Klingensmith
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
