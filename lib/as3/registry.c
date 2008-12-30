/* registry.c

   Routines for compiling Flash2 AVM2 ABC Actionscript

   Extension module for the rfxswf library.
   Part of the swftools package.

   Copyright (c) 2008 Matthias Kramm <kramm@quiss.org>
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include <assert.h>
#include "pool.h"
#include "registry.h"
#include "builtin.h"

static dict_t*classes=0;

// ----------------------- class signature ------------------------------

char classinfo_equals(classinfo_t*c1, classinfo_t*c2)
{
    if(!!c1 != !!c2)
        return 0;
    /* notice: access right is *not* respected */
    if(!strcmp(c1->name, c2->name) &&
       !strcmp(c1->package, c2->package)) {
        return 1;
    }
    return 0;
}
static unsigned int classinfo_hash(classinfo_t*c)
{
    unsigned int hash = 0;
    hash = crc32_add_string(hash, c->package);
    hash = crc32_add_string(hash, c->name);
    return hash;
}

static void* dummy_clone(void*other) {return other;}
static void dummy_destroy(classinfo_t*c) {}

type_t classinfo_type = {
    hash: (hash_func)classinfo_hash,
    equals: (equals_func)classinfo_equals,
    /* all signatures are static */
    dup: (dup_func)dummy_clone,
    free: (free_func)dummy_destroy,
};

// ----------------------- function signature ------------------------------

static char memberinfo_equals(memberinfo_t*f1, memberinfo_t*f2)
{
    return !strcmp(f1->name, f2->name);
}
static unsigned int memberinfo_hash(memberinfo_t*f)
{
    return crc32_add_string(0, f->name);
}
type_t memberinfo_type = {
    hash: (hash_func)memberinfo_hash,
    equals: (equals_func)memberinfo_equals,
    /* all signatures are static */
    dup: (dup_func)dummy_clone,
    free: (free_func)dummy_destroy,
};

// ------------------------- constructors --------------------------------

#define AVERAGE_NUMBER_OF_MEMBERS 8
classinfo_t* classinfo_register(int access, char*package, char*name)
{
    NEW(classinfo_t,c);
    c->access = access;
    c->package = package;
    c->name = name;
    dict_put(classes, c, c);
    dict_init(&c->members,AVERAGE_NUMBER_OF_MEMBERS);
    return c;
}
memberinfo_t* memberinfo_register(classinfo_t*cls, const char*name, U8 kind)
{
    NEW(memberinfo_t,m);
    m->kind = kind;
    m->name = strdup(name);
    dict_put(&cls->members, name, m);
    return m;
}

// --------------- builtin classes (from builtin.c) ----------------------

void registry_init()
{
    classes = builtin_getclasses();
}
classinfo_t* registry_safefindclass(const char*package, const char*name)
{
    classinfo_t*c = registry_findclass(package, name);
    assert(c);
    return c;
}
classinfo_t* registry_findclass(const char*package, const char*name)
{
    assert(classes);
    classinfo_t tmp;
    tmp.package = package;
    tmp.name = name;
    classinfo_t* c = (classinfo_t*)dict_lookup(classes, &tmp);
    /*if(c)
        printf("%s.%s->%08x (%s.%s)\n", package, name, c, c->package, c->name);*/
    return c;
}
memberinfo_t* registry_findmember(classinfo_t*cls, const char*name)
{
    return (memberinfo_t*)dict_lookup(&cls->members, name);
}
void registry_fill_multiname(multiname_t*m, namespace_t*n, classinfo_t*c)
{
    m->type = QNAME;
    m->ns = n;
    m->ns->access = c->access;
    m->ns->name = (char*)c->package;
    m->name = c->name;
    m->namespace_set = 0;
}
multiname_t* classinfo_to_multiname(classinfo_t*cls)
{
    if(!cls)
        return 0;
    multiname_t*m=0;
    namespace_t*ns = namespace_new(cls->access, cls->package);
    return multiname_new(ns,cls->name);
}

// ----------------------- builtin types ------------------------------
classinfo_t* registry_getanytype() {return 0;/*FIXME*/}

classinfo_t* registry_getobjectclass() {return registry_safefindclass("", "Object");}
classinfo_t* registry_getstringclass() {return registry_safefindclass("", "String");}
classinfo_t* registry_getintclass() {return registry_safefindclass("", "int");}
classinfo_t* registry_getuintclass() {return registry_safefindclass("", "uint");}
classinfo_t* registry_getbooleanclass() {return registry_safefindclass("", "Boolean");}
classinfo_t* registry_getnumberclass() {return registry_safefindclass("", "Number");}
classinfo_t* registry_getMovieClip() {return registry_safefindclass("flash.display", "MovieClip");}

// ----------------------- builtin dummy types -------------------------
classinfo_t nullclass = {
    ACCESS_PACKAGE, 0, "", "null", 0, 0, 0,
};
classinfo_t* registry_getnullclass() {
    return &nullclass;
}
