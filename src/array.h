//
//  array.h
//
//  shrinkwrap is a tool for triangulating bitmap alpha.
//  Copyright (c) 2014 Jarrod Moldrich. All rights reserved.
//
//  This file is part of shrinkwrap.
//
//  shrinkwrap is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License as
//  published by the Free Software Foundation, either version 3 of the
//  License, or (at your option) any later version.
//
//  shrinkwrap is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Affero General Public License for more details.
//
//  You should have received a copy of the GNU Affero General Public License
//  along with shrinkwrap.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef shrinkwrap_array_h
#define shrinkwrap_array_h

struct array_desc_struct;
typedef struct array_desc_struct array_desc;
typedef array_desc * array_descp;

array_descp array_create(size_t startCapacity, size_t stride);
void array_resize(array_descp desc, size_t capacity);
void * array_push(array_descp desc);
void * array_get(array_descp desc, size_t i);
size_t array_size(array_descp desc);

#endif
