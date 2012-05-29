%module cproton
%{
/* Includes the header in the wrapper code */
#include <proton/engine.h>
#include <proton/message.h>
#include <proton/sasl.h>
#include <proton/driver.h>
%}

typedef unsigned int size_t;
typedef signed int ssize_t;

%include <cstring.i>

%cstring_output_withsize(char *OUTPUT, size_t *OUTPUT_SIZE)
%cstring_output_allocate_size(char **ALLOC_OUTPUT, size_t *ALLOC_SIZE, free(*$1));

ssize_t pn_send(pn_link_t *transport, char *STRING, size_t LENGTH);
%ignore pn_send;

%rename(pn_recv) wrap_pn_recv;
%inline %{
  int wrap_pn_recv(pn_link_t *link, char *OUTPUT, size_t *OUTPUT_SIZE) {
    ssize_t sz = pn_recv(link, OUTPUT, *OUTPUT_SIZE);
    if (sz >= 0) {
      *OUTPUT_SIZE = sz;
    } else {
      *OUTPUT_SIZE = 0;
    }
    return sz;
  }
%}
%ignore pn_recv;

ssize_t pn_input(pn_transport_t *transport, char *STRING, size_t LENGTH);
%ignore pn_input;

%rename(pn_output) wrap_pn_output;
%inline %{
  int wrap_pn_output(pn_transport_t *transport, char *OUTPUT, size_t *OUTPUT_SIZE) {
    ssize_t sz = pn_output(transport, OUTPUT, *OUTPUT_SIZE);
    if (sz >= 0) {
      *OUTPUT_SIZE = sz;
    } else {
      *OUTPUT_SIZE = 0;
    }
    return sz;
  }
%}
%ignore pn_output;

%rename(pn_delivery) wrap_pn_delivery;
%inline %{
  pn_delivery_t *wrap_pn_delivery(pn_link_t *link, char *STRING, size_t LENGTH) {
    return pn_delivery(link, pn_dtag(STRING, LENGTH));
  }
%}
%ignore pn_delivery;

%rename(pn_delivery_tag) wrap_pn_delivery_tag;
%inline %{
  void wrap_pn_delivery_tag(pn_delivery_t *delivery, char **ALLOC_OUTPUT, size_t *ALLOC_SIZE) {
    pn_delivery_tag_t tag = pn_delivery_tag(delivery);
    *ALLOC_OUTPUT = malloc(tag.size);
    *ALLOC_SIZE = tag.size;
    memcpy(*ALLOC_OUTPUT, tag.bytes, tag.size);
  }
%}
%ignore pn_delivery_tag;

%rename(pn_message_data) wrap_pn_message_data;
%inline %{
  int wrap_pn_message_data(char *STRING, size_t LENGTH, char *OUTPUT, size_t *OUTPUT_SIZE) {
    ssize_t sz = pn_message_data(OUTPUT, *OUTPUT_SIZE, STRING, LENGTH);
    if (sz >= 0) {
      *OUTPUT_SIZE = sz;
    } else {
      *OUTPUT_SIZE = 0;
    }
    return sz;
  }
%}
%ignore pn_message_data;

%rename(pn_listener) wrap_pn_listener;
%inline {
  pn_listener_t *wrap_pn_listener(pn_driver_t *driver, const char *host, const char *port, PyObject *context) {
    Py_XINCREF(context);
    return pn_listener(driver, host, port, context);
  }
}
%ignore pn_listener;

%rename(pn_listener_context) wrap_pn_listener_context;
%inline {
  PyObject *wrap_pn_listener_context(pn_listener_t *l) {
    PyObject *result = pn_listener_context(l);
    if (result) {
      Py_INCREF(result);
      return result;
    } else {
      Py_RETURN_NONE;
    }
  }
}
%ignore pn_listener_context;

%rename(pn_listener_destroy) wrap_pn_listener_destroy;
%inline %{
  void wrap_pn_listener_destroy(pn_listener_t *l) {
    PyObject *obj = pn_listener_context(l);
    Py_XDECREF(obj);
    pn_listener_destroy(l);
  }
%}
%ignore pn_listener_destroy;

%rename(pn_connector) wrap_pn_connector;
%inline {
  pn_connector_t *wrap_pn_connector(pn_driver_t *driver, const char *host, const char *port, PyObject *context) {
    Py_XINCREF(context);
    return pn_connector(driver, host, port, context);
  }
}
%ignore pn_connector;

%rename(pn_connector_context) wrap_pn_connector_context;
%inline {
  PyObject *wrap_pn_connector_context(pn_connector_t *c) {
    PyObject *result = pn_connector_context(c);
    if (result) {
      Py_INCREF(result);
      return result;
    } else {
      Py_RETURN_NONE;
    }
  }
}
%ignore pn_connector_context;

%rename(pn_connector_set_context) wrap_pn_connector_set_context;
%inline {
  void wrap_pn_connector_set_context(pn_connector_t *ctor, PyObject *context) {
    Py_XDECREF(pn_connector_context(ctor));
    Py_XINCREF(context);
    pn_connector_set_context(ctor, context);
  }
}
%ignore pn_connector_set_context;

%rename(pn_connector_destroy) wrap_pn_connector_destroy;
%inline %{
  void wrap_pn_connector_destroy(pn_connector_t *c) {
    PyObject *obj = pn_connector_context(c);
    Py_XDECREF(obj);
    pn_connector_destroy(c);
  }
%}
%ignore pn_connector_destroy;

%exception pn_driver_wait
{
  Py_BEGIN_ALLOW_THREADS
  $action
  Py_END_ALLOW_THREADS
}

%include "../cproton.i"