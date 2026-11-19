#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "scvpi_tlm_target_api.h"

#include <limits>
#include <vector>

namespace {

PyObject *py_try_recv(PyObject *self, PyObject *args) {
    const char *endpoint = nullptr;
    (void)self;
    if (!PyArg_ParseTuple(args, "s", &endpoint)) {
        return nullptr;
    }

    uint64_t id = 0;
    uint64_t address = 0;
    int command = 0;
    unsigned int streaming_width = 0;
    unsigned int data_length = 0;
    unsigned int byte_enable_length = 0;
    const int available = scvpi_tlm_target_pop_request(
        endpoint, &id, &command, &address, &streaming_width, &data_length, &byte_enable_length);
    if (available < 0) {
        PyErr_SetString(PyExc_RuntimeError, "failed to receive SCVPI TLM request");
        return nullptr;
    }
    if (available == 0) {
        Py_RETURN_NONE;
    }

    std::vector<unsigned char> data(data_length);
    std::vector<unsigned char> byte_enable(byte_enable_length);
    if (scvpi_tlm_target_copy_request(
            endpoint, id, data.empty() ? nullptr : data.data(), data_length,
            byte_enable.empty() ? nullptr : byte_enable.data(), byte_enable_length) != 0) {
        PyErr_SetString(PyExc_RuntimeError, "failed to copy SCVPI TLM request data");
        return nullptr;
    }

    PyObject *data_bytes = PyBytes_FromStringAndSize(reinterpret_cast<const char *>(data.data()),
                                                     static_cast<Py_ssize_t>(data.size()));
    PyObject *byte_enable_bytes =
        PyBytes_FromStringAndSize(reinterpret_cast<const char *>(byte_enable.data()),
                                  static_cast<Py_ssize_t>(byte_enable.size()));
    if (data_bytes == nullptr || byte_enable_bytes == nullptr) {
        Py_XDECREF(data_bytes);
        Py_XDECREF(byte_enable_bytes);
        return nullptr;
    }

    PyObject *result = Py_BuildValue("(KiKIOO)", static_cast<unsigned long long>(id), command,
                                     static_cast<unsigned long long>(address), streaming_width,
                                     data_bytes, byte_enable_bytes);
    Py_DECREF(data_bytes);
    Py_DECREF(byte_enable_bytes);
    return result;
}

PyObject *py_respond(PyObject *self, PyObject *args) {
    const char *endpoint = nullptr;
    unsigned long long id = 0;
    const char *data = nullptr;
    Py_ssize_t data_length = 0;
    int response_status = 0;
    (void)self;

    if (!PyArg_ParseTuple(args, "sKy#i", &endpoint, &id, &data, &data_length, &response_status)) {
        return nullptr;
    }
    if (static_cast<size_t>(data_length) > std::numeric_limits<unsigned int>::max()) {
        PyErr_SetString(PyExc_OverflowError, "response exceeds the TLM payload length limit");
        return nullptr;
    }
    const int status = scvpi_tlm_target_respond(
        endpoint, static_cast<uint64_t>(id), reinterpret_cast<const unsigned char *>(data),
        static_cast<unsigned int>(data_length), response_status);
    if (status != 0) {
        PyErr_Format(PyExc_RuntimeError, "SCVPI TLM response failed (%d)", status);
        return nullptr;
    }
    Py_RETURN_NONE;
}

PyMethodDef methods[] = {
    {"try_recv", py_try_recv, METH_VARARGS,
     "Return the next pending request as (id, command, address, width, data, byte_en), or None."},
    {"respond", py_respond, METH_VARARGS,
     "Complete a request with endpoint, id, response data bytes, and TLM response status."},
    {nullptr, nullptr, 0, nullptr}};

PyModuleDef module = {PyModuleDef_HEAD_INIT, "scvpi_tlm_target_ext",
                      "SCVPI SystemC-initiated TLM target API", -1, methods};

} // namespace

PyMODINIT_FUNC PyInit_scvpi_tlm_target_ext() {
    return PyModule_Create(&module);
}

extern "C" void scvpi_tlm_target_ext_entry() {
    const int rc = PyImport_AppendInittab("scvpi_tlm_target_ext", &PyInit_scvpi_tlm_target_ext);
    if (rc != 0) {
        fprintf(stderr, "failed to register scvpi_tlm_target_ext (rc=%d)\n", rc);
    }
}
