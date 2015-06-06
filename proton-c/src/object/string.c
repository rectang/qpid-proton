/*
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

#include "platform.h"

#include <proton/error.h>
#include <proton/object.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define CFISH_USE_SHORT_NAMES
#include "Clownfish/ByteBuf.h"

struct pn_string_t {
  ByteBuf *cfobj;
};

static ByteBuf*
AS_CFOBJ(void *object) {
    pn_string_t *string = (pn_string_t *)object;
    if (string) {
        return string->cfobj;
    }
    return NULL;
}

static void
set_cfobj(void *object, ByteBuf *cfobj) {
    pn_string_t *string = (pn_string_t *)object;
    if (string) {
       string->cfobj = cfobj;
    }
    else {
        DECREF(cfobj);
    }
}

static void
null_terminate(ByteBuf *cfobj) {
  BB_Cat_Bytes(cfobj, "\0", 1);
  BB_Set_Size(cfobj, BB_Get_Size(cfobj) - 1);
}

static void pn_string_finalize(void *object)
{
  DECREF(AS_CFOBJ(object));
}

static uintptr_t pn_string_hashcode(void *object)
{
  ByteBuf *cfobj = AS_CFOBJ(object);
  ssize_t size = (ssize_t)BB_Get_Size(cfobj);
  const char *bytes = BB_Get_Buf(cfobj);

  if (bytes == NULL) {
    return 0;
  }

  uintptr_t hashcode = 1;
  for (ssize_t i = 0; i < size; i++) {
    hashcode = hashcode * 31 + bytes[i];
  }
  return hashcode;
}

static intptr_t pn_string_compare(void *oa, void *ob)
{
  ByteBuf *a = AS_CFOBJ(oa);
  ByteBuf *b = AS_CFOBJ(ob);
  intptr_t a_size = (intptr_t)BB_Get_Size(a);
  intptr_t b_size = (intptr_t)BB_Get_Size(b);

  if (a_size != b_size) {  // Order by length then memcmp?  Seems odd.
    return b_size - a_size;
  }

  const char *a_bytes = BB_Get_Buf(a);
  const char *b_bytes = BB_Get_Buf(b);

  if (a_bytes == NULL) {
    return 0;
  } else {
    return memcmp(a_bytes, b_bytes, a_size);
  }
}

static int pn_string_inspect(void *obj, pn_string_t *dst)
{
  ByteBuf *cfobj = AS_CFOBJ(obj);
  uint8_t *bytes = (uint8_t*)BB_Get_Buf(cfobj);
  ssize_t size = (ssize_t)BB_Get_Size(cfobj);

  if (bytes == NULL) {
    return pn_string_addf(dst, "null");
  }

  int err = pn_string_addf(dst, "\""); // Should return if error?

  for (int i = 0; i < size; i++) {
    uint8_t c = bytes[i];
    if (isprint(c)) {
      err = pn_string_addf(dst, "%c", c);
      if (err) return err;
    } else {
      err = pn_string_addf(dst, "\\x%.2x", c);
      if (err) return err;
    }
  }

  return pn_string_addf(dst, "\"");
}

pn_string_t *pn_string(const char *bytes)
{
  return pn_stringn(bytes, bytes ? strlen(bytes) : 0);
}

#define pn_string_initialize NULL


pn_string_t *pn_stringn(const char *bytes, size_t n)
{
  static const pn_class_t clazz = PN_CLASS(pn_string);
  pn_string_t *string = (pn_string_t *) pn_class_new(&clazz, sizeof(pn_string_t));

  cfish_bootstrap_parcel();
  ByteBuf *cfobj = BB_new(n + 1);
  set_cfobj(string, cfobj);

  pn_string_setn(string, bytes, n);

  return string;
}

const char *pn_string_get(pn_string_t *string)
{
  assert(string);
  return BB_Get_Buf(AS_CFOBJ(string));
}

size_t pn_string_size(pn_string_t *string)
{
  assert(string);
  return BB_Get_Size(AS_CFOBJ(string));
}

int pn_string_set(pn_string_t *string, const char *bytes)
{
  return pn_string_setn(string, bytes, bytes ? strlen(bytes) : 0);
}

int pn_string_grow(pn_string_t *string, size_t capacity)
{
  BB_Grow(AS_CFOBJ(string), (capacity * sizeof(char) + 1));
  return 0;
}

int pn_string_setn(pn_string_t *string, const char *bytes, size_t n)
{
  int err = pn_string_grow(string, n);
  if (err) return err;

  ByteBuf *cfobj = AS_CFOBJ(string);
  if (bytes) {
    BB_Mimic_Bytes(cfobj, bytes, n);
    null_terminate(cfobj);
  }
  else {
    // Hack to set internal buf to NULL in ByteBuf.
    DECREF(BB_Yield_Blob(cfobj));
  }

  return 0;
}

ssize_t pn_string_put(pn_string_t *string, char *dst)
{
  assert(string);
  assert(dst);

  ByteBuf *cfobj = AS_CFOBJ(string);
  const char *bytes = BB_Get_Buf(cfobj);
  size_t size = BB_Get_Size(cfobj);
  if (bytes != NULL) {
    memcpy(dst, bytes, size + 1);
  }

  return (ssize_t)size;
}

void pn_string_clear(pn_string_t *string)
{
  BB_Mimic_Bytes(AS_CFOBJ(string), NULL, 0);
}

int pn_string_format(pn_string_t *string, const char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  int err = pn_string_vformat(string, format, ap);
  va_end(ap);
  return err;
}

int pn_string_vformat(pn_string_t *string, const char *format, va_list ap)
{
  pn_string_set(string, "");
  return pn_string_vaddf(string, format, ap);
}

int pn_string_addf(pn_string_t *string, const char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  int err = pn_string_vaddf(string, format, ap);
  va_end(ap);
  return err;
}

int pn_string_vaddf(pn_string_t *string, const char *format, va_list ap)
{
  va_list copy;
  ByteBuf *cfobj = AS_CFOBJ(string);

  if (BB_Get_Buf(cfobj) == NULL) {
    return PN_ERR;
  }

  while (true) {
    va_copy(copy, ap);
    char *bytes = BB_Get_Buf(cfobj);
    ssize_t size = (ssize_t)BB_Get_Size(cfobj);
    ssize_t capacity = (ssize_t)BB_Get_Capacity(cfobj);
    int err = vsnprintf(bytes + size, capacity - size, format, copy);
    va_end(copy);
    if (err < 0) {
      return err;
    } else if (err >= capacity - size) {
      pn_string_grow(string, size + err);
    } else {
      BB_Set_Size(cfobj, size + err);
      return 0;
    }
  }
}

char *pn_string_buffer(pn_string_t *string)
{
  assert(string);
  return BB_Get_Buf(AS_CFOBJ(string));
}

size_t pn_string_capacity(pn_string_t *string)
{
  assert(string);
  size_t capacity = BB_Get_Capacity(AS_CFOBJ(string));
  if (capacity > 0) {
    capacity -= 1;
  }
  return capacity;
}

int pn_string_resize(pn_string_t *string, size_t size)
{
  assert(string);
  int err = pn_string_grow(string, size);
  if (err) return err;
  ByteBuf *cfobj = AS_CFOBJ(string);
  BB_Set_Size(cfobj, size); // may append junk to string
  null_terminate(cfobj);
  return 0;
}

int pn_string_copy(pn_string_t *string, pn_string_t *src)
{
  assert(string);
  return pn_string_setn(string, pn_string_get(src), pn_string_size(src));
}
