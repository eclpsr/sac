/*
    This file is part of Soupe Au Caillou.

    @author Soupe au Caillou - Jordane Pelloux-Prayer
    @author Soupe au Caillou - Gautier Pelloux-Prayer
    @author Soupe au Caillou - Pierre-Eric Pelloux-Prayer

    Soupe Au Caillou is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    Soupe Au Caillou is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Soupe Au Caillou.  If not, see <http://www.gnu.org/licenses/>.
*/



#include <algorithm>
#include <string>
#include "base/Color.h"
#include "base/Log.h"


template <>
Property<float>::Property(hash_t id, unsigned long offset, float pEpsilon) :
    IProperty(id, PropertyType::Float, PropertyAttribute::None, offset, sizeof(float)), epsilon(pEpsilon) {}

template <>
Property<int>::Property(hash_t id, unsigned long offset, int pEpsilon) :
    IProperty(id, PropertyType::Int, PropertyAttribute::None, offset, sizeof(int)), epsilon(pEpsilon) {}

template <>
Property<hash_t>::Property(hash_t id, unsigned long offset, hash_t pEpsilon) :
    IProperty(id, PropertyType::Hash, PropertyAttribute::None, offset, sizeof(hash_t)), epsilon(pEpsilon) {}

template <>
Property<int8_t>::Property(hash_t id, unsigned long offset, int8_t pEpsilon) :
    IProperty(id, PropertyType::Int8, PropertyAttribute::None, offset, sizeof(int8_t)), epsilon(pEpsilon) {}

template <>
Property<uint8_t>::Property(hash_t id, unsigned long offset, uint8_t pEpsilon) :
    IProperty(id, PropertyType::Int8, PropertyAttribute::None, offset, sizeof(int8_t)), epsilon(pEpsilon) {}

template <>
Property<bool>::Property(hash_t id, unsigned long offset, bool pEpsilon) :
    IProperty(id, PropertyType::Bool, PropertyAttribute::None, offset, sizeof(bool)), epsilon(pEpsilon) {}

template <>
Property<Color>::Property(hash_t id, unsigned long offset, Color pEpsilon) :
    IProperty(id, PropertyType::Color, PropertyAttribute::None, offset, sizeof(Color)), epsilon(pEpsilon) {}

template <>
Property<glm::vec2>::Property(hash_t id, unsigned long offset, glm::vec2 pEpsilon) :
    IProperty(id, PropertyType::Vec2, PropertyAttribute::None, offset, sizeof(glm::vec2)), epsilon(pEpsilon) {}

template <class T>
Property<T>::Property(hash_t id, unsigned long offset, T pEpsilon) :
    IProperty(id, PropertyType::Unsupported, PropertyAttribute::None, offset, sizeof(T)), epsilon(pEpsilon) {}

template <class T>
Property<T>::Property(hash_t id, PropertyType::Enum type, unsigned long offset, T pEpsilon) :
    IProperty(id, type, PropertyAttribute::None, offset, sizeof(T)), epsilon(pEpsilon) {}

template <>
IntervalProperty<float>::IntervalProperty(hash_t id, unsigned long offset) : IProperty(id, PropertyType::Float, PropertyAttribute::Interval, offset, 2 * sizeof(float)) {}

template <>
IntervalProperty<int>::IntervalProperty(hash_t id, unsigned long offset) : IProperty(id, PropertyType::Int, PropertyAttribute::Interval, offset, 2 * sizeof(int)) {}

template <>
IntervalProperty<int8_t>::IntervalProperty(hash_t id, unsigned long offset) : IProperty(id, PropertyType::Int8, PropertyAttribute::Interval, offset, 2 * sizeof(int8_t)) {}

template <>
IntervalProperty<uint8_t>::IntervalProperty(hash_t id, unsigned long offset) : IProperty(id, PropertyType::Int8, PropertyAttribute::Interval, offset, 2 * sizeof(int8_t)) {}

template <>
IntervalProperty<Color>::IntervalProperty(hash_t id, unsigned long offset) : IProperty(id, PropertyType::Color, PropertyAttribute::Interval, offset, 2 * sizeof(Color)) {}

template <>
IntervalProperty<glm::vec2>::IntervalProperty(hash_t id, unsigned long offset) : IProperty(id, PropertyType::Vec2, PropertyAttribute::Interval, offset, 2 * sizeof(glm::vec2)) {}

template <typename T>
IntervalProperty<T>::IntervalProperty(hash_t id, unsigned long offset) : IProperty(id, PropertyType::Unsupported, PropertyAttribute::Interval, offset, 2 * sizeof(T)) {}

template <>
VectorProperty<std::string>::VectorProperty(hash_t id, unsigned long offset) : IProperty(id, PropertyType::String, PropertyAttribute::Vector, offset, 0) {}

template <>
VectorProperty<Color>::VectorProperty(hash_t id, unsigned long offset) : IProperty(id, PropertyType::Color, PropertyAttribute::Vector, offset, 0) {}

template <>
VectorProperty<float>::VectorProperty(hash_t id, unsigned long offset) : IProperty(id, PropertyType::Float, PropertyAttribute::Vector, offset, 0) {}

template <>
VectorProperty<Entity>::VectorProperty(hash_t id, unsigned long offset) : IProperty(id, PropertyType::Entity, PropertyAttribute::Vector, offset, 0) {}

template <typename T>
VectorProperty<T>::VectorProperty(hash_t id, unsigned long offset) : IProperty(id, PropertyType::Unsupported, PropertyAttribute::Vector, offset, 0) {}


template <>
bool Property<Color>::different(void* object, void* refObject) const {
    Color* a = (Color*) ((uint8_t*)object + offset);
    Color* b = (Color*) ((uint8_t*)refObject + offset);
    return *a != *b;
}

template <>
bool Property<glm::vec2>::different(void* object, void* refObject) const {
    glm::vec2* a = (glm::vec2*) ((uint8_t*)object + offset);
    glm::vec2* b = (glm::vec2*) ((uint8_t*)refObject + offset);
    //~ return (*a - *b).LengthSquared() > epsilon.LengthSquared();
    return glm::length(*a - *b) > glm::length(epsilon);
}

template <typename T>
bool Property<T>::different(void* object, void* refObject) const {
    T* a = (T*) ((uint8_t*)object + offset);
    T* b = (T*) ((uint8_t*)refObject + offset);
    return (glm::abs(*a - *b) - epsilon > 0);
}

template <>
unsigned VectorProperty<std::string>::size(void* object) const {
    std::vector<std::string>* v = (std::vector<std::string>*) ((uint8_t*)object + offset);
    unsigned s = sizeof(unsigned);
    for (unsigned i=0; i<v->size(); i++) {
        s += sizeof(unsigned) + (*v)[i].size(); // length + data
    }
    return s;
}

template <typename T>
unsigned VectorProperty<T>::size(void* object) const {
    std::vector<T>* v = (std::vector<T>*) ((uint8_t*)object + offset);
    return sizeof(unsigned) + v->size() * sizeof(T);
}

template <typename T>
bool VectorProperty<T>::different(void* object, void* refObject) const {
    std::vector<T>* v = (std::vector<T>*) ((uint8_t*)object + offset);
    std::vector<T>* w = (std::vector<T>*) ((uint8_t*)refObject + offset);
    if (v->size() != w->size())
        return true;
    for (unsigned i=0; i<v->size(); i++) {
        if ((*v)[i] != (*w)[i])
            return true;
    }
    return false;
}

template <>
int VectorProperty<std::string>::serialize(uint8_t* out, void* object) const {
    std::vector<std::string>* v = (std::vector<std::string>*) ((uint8_t*)object + offset);
    unsigned size = v->size();
    int idx = 0;
    memcpy(out, &size, sizeof(unsigned));
    idx += sizeof(unsigned);
    for (unsigned i=0; i<size; i++) {
        unsigned s = (*v)[i].size();
        memcpy(&out[idx], &s, sizeof(unsigned));
        idx += sizeof(unsigned);
        memcpy(&out[idx], (*v)[i].c_str(), s);
        idx += s;
    }
    return idx;
}

template <typename T>
int VectorProperty<T>::serialize(uint8_t* out, void* object) const {
    std::vector<T>* v = (std::vector<T>*) ((uint8_t*)object + offset);
    unsigned size = v->size();
    int idx = 0;
    memcpy(out, &size, sizeof(unsigned));
    idx += sizeof(unsigned);
    for (unsigned i=0; i<size; i++) {
        memcpy(&out[idx], &(*v)[i], sizeof(T));
        idx += sizeof(T);
    }
    return idx;
}

template <>
int VectorProperty<std::string>::deserialize(const uint8_t* in, void* object) const {
    std::vector<std::string>* v = (std::vector<std::string>*) ((uint8_t*)object + offset);
    v->clear();
    unsigned size;
    memcpy(&size, in, sizeof(unsigned));
    int idx = sizeof(unsigned);
    v->reserve(size);
    char tmp[1024];
    for (unsigned i=0; i<size; i++) {
        unsigned s;
        memcpy(&s, &in[idx], sizeof(unsigned));
        idx += sizeof(unsigned);
        LOGF_IF(s >= 1024, "Static tmp buffer to small to store: '" << s << "' bytes");
        memcpy(tmp, &in[idx], s);
        tmp[s] = '\0';
        v->push_back(std::string(tmp));
        idx += s;
    }
    return idx;
}

template <typename T>
int VectorProperty<T>::deserialize(const uint8_t* in, void* object) const {
    std::vector<T>* v = (std::vector<T>*) ((uint8_t*)object + offset);
    v->clear();
    unsigned size;
    memcpy(&size, in, sizeof(unsigned));
    int idx = sizeof(unsigned);
    v->reserve(size);
    for (unsigned i=0; i<size; i++) {
        T t;
        memcpy(&t, &in[idx], sizeof(T));
        v->push_back(t);
        idx += sizeof(T);
    }
    return idx;
}

template <typename T>
bool IntervalProperty<T>::different(void* object, void* refObject) const {
    Interval<T>* v = (Interval<T>*) ((uint8_t*)object + offset);
    Interval<T>* w = (Interval<T>*) ((uint8_t*)refObject + offset);
    return v->t1 != w->t1 || v->t2 != w->t2;
}

template <typename T>
int IntervalProperty<T>::serialize(uint8_t* out, void* object) const {
    Interval<T>* v = (Interval<T>*) ((uint8_t*)object + offset);
    memcpy(out, &v->t1, sizeof(T));
    memcpy(out + sizeof(T), &v->t2, sizeof(T));
    return 2 * sizeof(T);
}

template <typename T>
int IntervalProperty<T>::deserialize(const uint8_t* in, void* object) const {
    Interval<T>* v = (Interval<T>*) ((uint8_t*)object + offset);
    memcpy(&v->t1, in, sizeof(T));
    memcpy(&v->t2, in + sizeof(T), sizeof(T));
    return 2 * sizeof(T);
}

template <>
MapProperty<Entity, bool>::MapProperty(hash_t id, unsigned long offset) : IProperty(id, PropertyType::Bool, PropertyAttribute::None, offset, 0) {}

template <typename T, typename U>
MapProperty<T,U>::MapProperty(hash_t id, unsigned long offset) : IProperty(id, PropertyType::Unsupported, PropertyAttribute::None, offset, 0) {}

template <typename T, typename U>
unsigned MapProperty<T,U>::size(void* object) const {
    std::map<T, U>* v = (std::map<T, U>*) ((uint8_t*)object + offset);
    return sizeof(unsigned) + v->size() * (sizeof(T) + sizeof(U));
}

template <typename T, typename U>
bool MapProperty<T,U>::different(void* object, void* refObject) const {
    std::map<T, U>* v = (std::map<T, U>*) ((uint8_t*)object + offset);
    std::map<T, U>* w = (std::map<T, U>*) ((uint8_t*)refObject + offset);
    if (v->size() != w->size())
        return true;
    for (typename std::map<T, U>::iterator it=v->begin(), jt=w->begin(); it!=v->end(); ++it, ++jt) {
        if (v->find(jt->first) == v->end())
            return true;
        if (w->find(it->first) == w->end())
            return true;
    }
    return false;
}

template <typename T, typename U>
int MapProperty<T,U>::serialize(uint8_t* out, void* object) const {
    std::map<T, U>* v = (std::map<T, U>*) ((uint8_t*)object + offset);
    unsigned size = v->size();
    int idx = 0;
    memcpy(out, &size, sizeof(unsigned));
    idx += sizeof(unsigned);
    for (typename std::map<T, U>::iterator it=v->begin(); it!=v->end(); ++it) {
        memcpy(&out[idx], &(it->first), sizeof(T));
        idx += sizeof(T);
        memcpy(&out[idx], &(it->second), sizeof(U));
        idx += sizeof(U);
    }
    return idx;
}

template <typename T, typename U>
int MapProperty<T,U>::deserialize(const uint8_t* in, void* object) const {
    std::map<T, U>* v = (std::map<T, U>*) ((uint8_t*)object + offset);
    v->clear();
    unsigned size;
    memcpy(&size, in, sizeof(unsigned));
    int idx = sizeof(unsigned);
    for (unsigned i=0; i<size; i++) {
        T t;
        memcpy(&t, &in[idx], sizeof(T));
        idx += sizeof(T);
        U u;
        memcpy(&u, &in[idx], sizeof(U));
        idx += sizeof(U);
        v->insert(std::make_pair(t, u));
    }
    return idx;
}

template <>
unsigned MapProperty<std::string, float>::size(void* object) const {
    std::map<std::string, float>* v = (std::map<std::string, float>*) ((uint8_t*)object + offset);
    int size = sizeof(unsigned);
    for (std::map<std::string, float>::iterator it=v->begin(); it!=v->end(); ++it) {
        size += 1 + it->first.size() + sizeof(float);
    }
    return size;
}


template <>
int MapProperty<std::string, float>::serialize(uint8_t* out, void* object) const {
    std::map<std::string, float>* v = (std::map<std::string, float>*) ((uint8_t*)object + offset);
    unsigned size = v->size();
    int idx = 0;
    memcpy(out, &size, sizeof(unsigned));
    idx += sizeof(unsigned);
    for ( std::map<std::string, float>::iterator it=v->begin(); it!=v->end(); ++it) {
        uint8_t length = (uint8_t)it->first.length();
        memcpy(&out[idx++], &length, 1);
        memcpy(&out[idx], it->first.c_str(), length);
        idx += length;
        memcpy(&out[idx], &(it->second), sizeof(float));
        idx += sizeof(float);
    }
    return idx;
}

template <>
int MapProperty<std::string, float>::deserialize(const uint8_t* in, void* object) const {
    std::map<std::string, float>* v = (std::map<std::string, float>*) ((uint8_t*)object + offset);
    v->clear();
    unsigned size;
    char tmp[256];
    memcpy(&size, in, sizeof(unsigned));
    int idx = sizeof(unsigned);
    for (unsigned i=0; i<size; i++) {
        uint8_t length;
        memcpy(&length, &in[idx++], 1);
        memcpy(tmp, &in[idx], length);
        tmp[length] = '\0';
        idx += length;
        float f;
        memcpy(&f, &in[idx], sizeof(float));
        idx += sizeof(float);
        v->insert(std::make_pair(std::string(tmp), f));
    }
    return idx;
}
