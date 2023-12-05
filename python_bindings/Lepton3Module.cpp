/*
* Lepton3Module.cpp
*
* Python bindings for Lepton3 Module
* 
* Created on: 03 Jan 23
*     Author: Jonathan Morgan
*/

#include "Lepton3Module.h"
#include <stdlib.h>
#include <iostream>
#include <string>

using namespace std;

// Lepton3 container
typedef struct {
    PyObject_HEAD
    Lepton3* lepton3;
} PyLepton3_Object;

// Lepton3 class wrapper for accessing the Lepton3 class in Python:

// New 
static PyObject* PyLepton3_New(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
    PyLepton3_Object* self;
    self = (PyLepton3_Object*)type->tp_alloc(type, 0);
    if (!self)
    {
        return NULL;
    }

    self->lepton3 = NULL;
    return (PyObject*)self;
}

// Init
static int PyLepton3_Init(PyLepton3_Object* self, PyObject* args, PyObject* kwds)
{
    const char* spiDevice = NULL;
    const char* i2c_port = NULL;    
    Lepton3::DebugLvl deb_lvl = Lepton3::DBG_NONE;

    // Declare static char arrays for keyword list
    static char kwlist_spiDevice[] = "spiDevice";
    static char kwlist_i2c_port[] = "i2c_port";
    static char* kwlist[] = { kwlist_spiDevice, kwlist_i2c_port, NULL };

    // Parse arguments and keywords
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "ss", kwlist, 
                                        &spiDevice, &i2c_port))
    {
        return -1;
    }
    
    // Create new Lepton3 object with the parsed arguments
    self->lepton3 = new Lepton3(spiDevice, i2c_port, deb_lvl);
    return 0;
}

// Deallocate
static void PyLepton3_Dealloc(PyLepton3_Object* self)
{
    if (self->lepton3)
    {
        delete self->lepton3;
        self->lepton3 = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}

// stop
static PyObject* PyLepton3_Stop(PyLepton3_Object* self)
{
    if (self->lepton3)
    {
        self->lepton3->stop();
    }
    Py_RETURN_NONE;
}

// start
static PyObject* PyLepton3_Start(PyLepton3_Object* self)
{
    if (self->lepton3)
    {
        self->lepton3->start();
    }
    Py_RETURN_NONE;
}

// getFrame16
static PyObject* PyLepton3_GetFrame16(PyLepton3_Object* self)
{
    if (self->lepton3)
    {
        uint8_t width, height;
        uint16_t min, max;
        const uint16_t* frame = self->lepton3->getLastFrame16(width, height, &min, &max);
        double diff = static_cast<double>(max - min); // Image range
        double scale = 255./diff; // Scale factor
        
        if (frame != NULL)
        {
            // put items from frame into a Python list
            PyObject* pyList = PyList_New(width * height); // Use the width and height variables
            for (int i = 0; i < width * height; i++) // Use the width and height variables
            {
                double px = scale * (frame[i] - min);
                PyObject* pyItem = PyLong_FromLong(frame[i]);
                PyList_SetItem(pyList, i, pyItem);
            }
            return pyList;
        }
    }
    Py_RETURN_NONE;
}

// enableRGBOutput
static PyObject* PyLepton3_EnableRGBOutput(PyLepton3_Object* self, PyObject* args, PyObject* kwds)
{
    bool enable;
    if (!PyArg_ParseTuple(args, "b", &enable))
    {
        return PyLong_FromLong(-1);
    }
    if (self->lepton3)
    {

        if( self->lepton3->enableRadiometry( !enable ) < 0)
        {
            cerr << "Failed to set radiometry status" << std::endl;
            return PyLong_FromLong(-1);
        }
        else
        {
            if(!enable)
            {
                cout << " * Radiometry enabled " << std::endl;
            }
            else
            {
                cout << " * Radiometry disabled " << std::endl;
            }
        }

        // NOTE: if radiometry is enabled is unuseful to keep AGC enabled
        //       (see "FLIR LEPTON 3® Long Wave Infrared (LWIR) Datasheet" for more info)

        if( self->lepton3->enableAgc( enable ) < 0)
        {
            cerr << "Failed to set radiometry status" << std::endl;
            return PyLong_FromLong(-1);
        }
        else
        {
            if(!enable)
            {
                cout << " * AGC disabled " << std::endl;
            }
            else
            {
                cout << " * AGC enabled " << std::endl;
            }
        }

        if( self->lepton3->enableRgbOutput( enable ) < 0 )
        {
            cerr << "Failed to enable RGB output" << std::endl;
            return PyLong_FromLong(-1);
        }
        else
        {
            if(enable)
            {
                cout << " * RGB enabled " << std::endl;
            }
            else
            {
                cout << " * RGB disabled " << std::endl;
            }
        }

        return PyLong_FromLong(0);
    }
    return PyLong_FromLong(-1);
}




//-----------------------------------------------------------------------------
static PyTypeObject pyLepton3_Type = 
{
    PyVarObject_HEAD_INIT(NULL, 0)
};

static PyMethodDef pyLepton3_Methods[] = 
{
    { "stop", (PyCFunction)PyLepton3_Stop, METH_NOARGS, "Stop grabbing thread" },
    { "start", (PyCFunction)PyLepton3_Start, METH_NOARGS, "Start grabbing thread" },
    { "getFrame16", (PyCFunction)PyLepton3_GetFrame16, METH_NOARGS, "Get last available 16bit frame as vector not normalized" },
    { "enableRGBOutput", (PyCFunction)PyLepton3_EnableRGBOutput, METH_VARARGS, "Enable RGB output"},
    { NULL, NULL, 0, NULL }
};

// Register types
bool PyLepton3_RegisterTypes( PyObject* module )
{
    if (module == NULL)
    {
        return false;
    }

    // Lepton3
    pyLepton3_Type.tp_name = "Lepton3.Lepton3";
    pyLepton3_Type.tp_basicsize = sizeof(PyLepton3_Object);
    pyLepton3_Type.tp_itemsize = 0;
    pyLepton3_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    pyLepton3_Type.tp_doc = "Lepton3 class";
    pyLepton3_Type.tp_methods = pyLepton3_Methods;
    pyLepton3_Type.tp_new = PyLepton3_New;
    pyLepton3_Type.tp_init = (initproc)PyLepton3_Init;
    pyLepton3_Type.tp_dealloc = (destructor)PyLepton3_Dealloc;

    if (PyType_Ready(&pyLepton3_Type) < 0)
    {
        return false;
    }

    Py_INCREF(&pyLepton3_Type);
    if ( PyModule_AddObject(module, "Lepton3", (PyObject*)&pyLepton3_Type) < 0) 
    {
        return false;
    }

    return true;
}

// Module definition
static struct PyModuleDef pyLepton3_Module = 
{
    PyModuleDef_HEAD_INIT,
    "Lepton3",
    "Python bindings for Lepton3 Module",
    -1,
    NULL
};

PyMODINIT_FUNC
PyInit_Lepton3(void)
{
    PyObject* module = PyModule_Create(&pyLepton3_Module);
    if (module == NULL)
    {
        return NULL;
    }

    if (!PyLepton3_RegisterTypes(module))
    {
        return NULL;
    }

    return module;
}