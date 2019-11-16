# -*- coding: utf-8 -*-

'''
Copyright (C) 2002 Maurizio Umberto Puxeddu, Michael Gogins

This file is part of Csound.

The Csound Library is free software; you can redistribute it
and/or modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

Csound is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Csound; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
02110-1301 USA

Modification from BSD sources for strNcpy
Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>

Modified for speed -- JPff

Adapted for Python3 -- François Pinot, 2019

Automatically generate opcode structures, methods [and table entries]. Just run it.
'''

def generate_pycall_common_init_code(f, n, pre, post, rate, triggered=0):
    if triggered:
        t, T = 't', 'T'
    else:
        t, T = '', ''
    name = 'py%scall%d%s%s_%srate' % (pre, n, post, t, rate)
    s  = 'static int %s(CSOUND *csound, PYCALL%d%s *p)\n' % (name, n, T)
    s += '{\n'
    s += '    char     command[1024];\n'
    s += '    PyObject *result;\n'
    s += '    int      *py_initialize_done;\n'
    s += '    if (UNLIKELY((py_initialize_done =\n'
    s += '                  csound->QueryGlobalVariable(csound, "PY_INITIALIZE")) == NULL ||\n'
    s += '                  *py_initialize_done == 0)) {\n' 
    s += '        return NOTOK;\n'
    s += '    }\n'
    if triggered:
        s += '    if (!*p->trigger) {\n'
        if n == 0:
            pass
        if n == 1:
            s += '        *p->result = p->oresult;\n'
        elif n > 1:
            for i in range(n):
                s += '        *p->result%d = p->oresult%d;\n' % (i+1, i+1)
        s += '        return OK;\n'
        s += '    }\n'
    print(s, file=f)

def generate_pycall_common_call_code(f, context, withinit, triggered):
    if triggered:
        skip = 2
    else:
        skip = 1
    s  = '    format_call_statement(command, (char*) p->function->data,\n'
    s += '                          p->INOCOUNT, p->args, %d);\n\n' % skip
    if context == 'private':
        s += '    result = eval_string_in_given_context(command, 0);\n'
    else:
        if withinit:
            s += '    create_private_namespace_if_needed(&p->h);\n'
        s += '    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));\n'
    print(s, file=f)

def generate_pycall_exception_handling_code(f, n, pre, post, rate, triggered=0):
    s  = '    if (UNLIKELY(result == NULL)) {\n'
    s += '        return pyErrMsg(p, "python exception");\n'
    s += '    }\n'
    print(s, file=f)

def generate_pycall_result_conversion_code(f, n, pre, post, rate, triggered=0):
    if triggered:
        t, T = 't', 'T'
    else:
        t, T = '', ''
    s = ""
    if n == 0:
        s += '    if (UNLIKELY(result != Py_None)) {\n'
        s += '        return errMsg(p, "callable must return None");\n'
        s += '    }\n'
    elif n == 1:
        s += '    if (UNLIKELY(!PyFloat_Check(result))) {\n'
        s += '        return errMsg(p, "callable must return a float");\n'
        s += '    }\n'    
        s += '    else {\n'
        s += '        *p->result = PyFloat_AsDouble(result);\n'
        if triggered:
            s += '        p->oresult = *p->result;\n'
        s += '    }\n'
        s += '    return OK;\n'
    else:
        name = 'py%scall%d%s%s_%srate' % (pre, n, post, t, rate)
        s += '    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != %d)) {\n' % n
        s += '        return errMsg(p, "callable must return %d values");\n' % n
        s += '    }\n'
        s += '    else {\n'
        for i in range(n):
            s += '        *p->result%d = PyFloat_AsDouble(PyTuple_GET_ITEM(result, %d));\n' % (i+1, i)
            if triggered:
                s += '        p->oresult%d = *p->result%d;\n' % (i+1, i+1)
        s += '    }\n'
    s += '\n'
    s += '    Py_DECREF(result);\n'
    s += '    return OK;\n'
    s += '}\n'
    print(s, file=f)

def generate_pycall_krate_method(f, n, triggered=0):
    generate_pycall_common_init_code(f, n, '', '', 'k', triggered)
    generate_pycall_common_call_code(f, 'private', 1, triggered)
    generate_pycall_exception_handling_code(f, n, '', '', 'k', triggered)
    generate_pycall_result_conversion_code(f, n, '', '', 'k', triggered)

def generate_pylcall_irate_method(f, n, triggered=0):
    if triggered:
        t, T = 't', 'T'
    else:
        t, T = '', ''
    name = 'pylcall%d%s_irate' % (n, t)
    s  = 'static int %s(CSOUND *csound, PYCALL%d%s *p)\n' % (name, n, T)
    s += '{\n'
    s += '    int *py_initialize_done;\n'
    s += '    if (UNLIKELY((py_initialize_done =\n'
    s += '                  csound->QueryGlobalVariable(csound, "PY_INITIALIZE")) == NULL ||\n'
    s += '                  *py_initialize_done == 0)) {\n' 
    s += '        return NOTOK;\n'
    s += '    }\n'
    s += '    create_private_namespace_if_needed(&p->h);\n'
    s += '    return OK;\n'
    s += '}\n'
    print(s, file=f)

def generate_pylcall_krate_method(f, n, triggered=0):
    generate_pycall_common_init_code(f, n, 'l', '', 'k', triggered)
    generate_pycall_common_call_code(f, 'global', 0, triggered)
    generate_pycall_exception_handling_code(f, n, 'l', '', 'k', triggered)
    generate_pycall_result_conversion_code(f, n, 'l', '', 'k', triggered)

def generate_pylcalli_irate_method(f, n):
    generate_pycall_common_init_code(f, n, 'l', 'i', 'i')
    generate_pycall_common_call_code(f, 'global', 1, 0)
    generate_pycall_exception_handling_code(f, n, 'l', 'i', 'i')
    generate_pycall_result_conversion_code(f, n, 'l', 'i', 'i')

# ----------

def generate_pycall_opcode_struct(f, n, triggered=0):
    if triggered:
        T = 'T'
    else:
        T = ''
    s  = 'typedef struct {\n'
    s += '    OPDS      h;\n'
    if n == 1:
        s += '    MYFLT     *result;\n'
    else:
        for i in range(n):
            s += '    MYFLT     *result%d;\n' % (i+1)
    if triggered:
        s += '    MYFLT     *trigger;\n'
    s += '    STRINGDAT *function;\n'
    s += '    MYFLT     *args[VARGMAX-3];\n'
    if triggered:
        if n == 1:
            s += '    MYFLT     oresult\n;'
        else:
            for i in range(n):
                s += '    MYFLT     oresult%d;\n' % (i+1)
    s += '} PYCALL%d%s;\n' % (n, T)
    print(s, file=f)

# --------

f = open('pycall.auto.c', 'w')
s = '''/*
 * pyx.auto.c
 *
 * Copyright (C) 2002 Maurizio Umberto Puxeddu, Michael Gogins
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Don't modify this file, it's automatically genetated by pyx-gen.py
 */
'''
print(s, file=f)
for n in range(9):
    generate_pycall_krate_method(f, n)
    generate_pylcall_irate_method(f, n)
    generate_pylcall_krate_method(f, n)
    generate_pylcalli_irate_method(f, n)

    generate_pycall_krate_method(f, n, 1)
    generate_pylcall_irate_method(f, n, 1)
    generate_pylcall_krate_method(f, n, 1)
f.close()

f = open('pycall.auto.h', 'w')
s = '''/*
 * pyx.auto.h
 *
 * Copyright (C) 2002 Maurizio Umberto Puxeddu, Michael Gogins
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Don't modify this file. It's automatically generated by pyx-gen.py
 */
'''
print(s, file=f)
for n in range(9):
    generate_pycall_opcode_struct(f, n)
    generate_pycall_opcode_struct(f, n, 1)
f.close()

