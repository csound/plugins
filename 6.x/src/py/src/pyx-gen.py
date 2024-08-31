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

def generate_x_method(f, action, context, rate0, triggered):
    if rate0 != 'k':
        rate = rate0
    else:
        rate = ''

    if action in ('exec', 'assign', 'eval'):
        size = 1024
    elif action == 'run':
        size = 40960
    else:
        raise 'undefined action %s' % action

    if action in ('run', 'assign'):
        helper =  'run_statement'
    elif action == 'exec':
        helper = 'exec_file'
    elif action == 'eval':
        helper = 'eval_string'
    else:
        raise 'undefined action %s' % action

    if context == 'private':
        prefix = 'l'
        ns = 'GETPYLOCAL(p->h.insdshead)'
    elif context == 'global':
        prefix = ''
        ns = '0'
        prepare = ''
    else:
        raise 'undefined context %s' % context

    if triggered:
        if rate == 'i': raise 'cannot be triggered at i-rate'
        t, T = 't', 'T'
    else:
        t, T = '', ''

    ACTION = action.upper()
    RATE = rate.upper()

    name = 'py%(prefix)s%(action)s%(rate)s%(t)s_%(rate0)srate' % locals()

    s  = 'static int %(name)s(CSOUND *csound, PY%(ACTION)s%(T)s *p)\n' % locals()
    s += '{\n'
    s += '    char      source[%d];\n' % size
    s += '    PyObject  *result;\n'
    s += '    int       *py_initialize_done;\n'
    s += '    if ((py_initialize_done = csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||\n'
    s += '        *py_initialize_done == 0) {\n' 
    s += '        return NOTOK;\n'
    s += '    }\n\n'
 
    if triggered:
        if action == 'eval':
            s += '    if (!*p->trigger) {\n'
            s += '      *p->result = p->oresult;\n'
            s += '      return OK;\n'
            s += '    }\n'
        else:
            s += '    if (!*p->trigger) {\n'
            s += '      return OK;\n'
            s += '    }\n'

    if context == 'private' and rate0 == 'i':
        s += '    create_private_namespace_if_needed(&p->h);\n\n'

    if action == 'assign':
        s += '    snprintf(source, %d, "%%s = %%f", (char*) p->string->data, *p->value);\n' % size
    else:
        s += "    strNcpy(source, (char*) p->string->data, %d); // source[%d] = '\\0'\n\n" % (size, size-1)

    if action == 'exec':
        s += '    result = %(helper)s_in_given_context(csound, source, %(ns)s);\n' % locals()
    else:
        s += '    result = %(helper)s_in_given_context(source, %(ns)s);\n' % locals()
    s += '    if (result == NULL) {\n'
    s += '        return pyErrMsg(p, "python exception");\n'
    s += '    }\n'

    if action == 'eval':
        s += '    else if (!PyFloat_Check(result)) {\n'
        s += '        errMsg(p, "expression must evaluate in a float");\n'
        s += '    }\n'
        s += '    else {\n'
        s += '        *p->result = PyFloat_AsDouble(result);\n'
        if triggered:
            s += '        p->oresult = *p->result;\n'
        s += '    }\n'

    s += '    Py_DECREF(result);\n'
    s += '    return OK;\n'
    s += '}\n'
    print(s, file=f)

def generate_init_method(f, action, triggered):
    ACTION = action.upper()
    if triggered:
        t, T = 't', 'T'
    else:
        t, T = '', ''
    s  = 'static int pyl%(action)s%(t)s_irate(CSOUND *csound, PY%(ACTION)s%(T)s *p)\n' % locals()
    s += '{\n'
    s += '    int *py_initialize_done;\n'
    s += '    if((py_initialize_done = csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||\n'
    s += '       *py_initialize_done == 0) {\n' 
    s += '        return NOTOK;\n'
    s += '    }\n'
    s += '    create_private_namespace_if_needed(&p->h);\n'
    s += '    return OK;\n'
    s += '}\n'
    print(s, file=f)

# ----------------

def generate_pycall_opcode_struct(f, action, triggered):
    ACTION = action.upper()
    if triggered:
        T = 'T'
    else:
        T = ''
    s = 'typedef struct {\n'
    s += '    OPDS      h;\n'
    if action == 'eval':
        s += '    MYFLT     *result;\n'
    if triggered:
        s += '    MYFLT     *trigger;\n'
    s += '    STRINGDAT *string;\n'
    if action == 'assign':
        s += '    MYFLT     *value;\n'
    if action == 'eval' and triggered:
        s += '    MYFLT     oresult;\n'
    s += '} PY%(ACTION)s%(T)s;\n' % locals()
    print(s, file=f)

f = open('pyx.auto.c', 'w')
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

char *strNcpy(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit or until NULL */
    if (n != 0) {
      while (--n != 0) {
        if ((*d++ = *s++) == '\\0')
          break;
      }
    }

    /* Not enough room in dst, add NUL */
    if (n == 0) {
      if (siz != 0)
        *d = '\\0';     /* NUL-terminate dst */

      //while (*s++) ;
    }
    return dst;        /* count does not include NUL */
}
'''
print(s, file=f)
for action in ['exec', 'run', 'eval', 'assign']:
    generate_x_method(f, action, 'global', 'k', 0)
    generate_x_method(f, action, 'global', 'i', 0)

    generate_init_method(f, action, 0)
    generate_x_method(f, action, 'private', 'k', 0)
    generate_x_method(f, action, 'private', 'i', 0)

    generate_x_method(f, action, 'global', 'k', 1)
    generate_init_method(f, action, 1)
    generate_x_method(f, action, 'private', 'k', 1)
f.close()

f = open('pyx.auto.h', 'w')
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
for action in ['exec', 'run', 'eval', 'assign']:
    for triggered in [0, 1]:
        generate_pycall_opcode_struct(f, action, triggered)
f.close()

