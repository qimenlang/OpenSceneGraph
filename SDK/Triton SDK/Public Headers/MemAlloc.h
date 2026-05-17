// Copyright (c) 2010-2016 Sundog Software, LLC. All rights reserved worldwide.


/** \file MemAlloc.h
   \brief Memory allocation interface for SilverLining.
 */

#ifndef TRITON_MEMALLOC_H
#define TRITON_MEMALLOC_H

#include <cstddef>
#include <string>

#ifdef _WIN32
#define TRITONAPI __cdecl
#else
#define TRITONAPI
#endif

#define TRITON_NEW new
#define TRITON_DELETE delete
#define TRITON_MALLOC Allocator::GetAllocator()->alloc
#define TRITON_FREE Allocator::GetAllocator()->dealloc

namespace Triton
{
/** You may extend the Allocator class to hook your own memory management scheme into Triton.
   Instantiate your own implementation of Allocator, and pass it into Allocator::SetAllocator prior to
   calling any other Triton methods or instantiating any Triton objects.

   Each object in Triton overloads the new and delete operators, and routes memory management through
   the Allocator as well.
 */
class Allocator
{
public:
    Allocator();
    virtual ~Allocator();

    /// Allocate a block of memory; defaults to malloc()
    virtual void * TRITONAPI alloc(size_t bytes);

    /// Free a block of memory; defaults to free()
    virtual void TRITONAPI dealloc(void *p);

    /// Retrieves the static allocator object
    static Allocator * TRITONAPI GetAllocator() {
        return sAllocator;
    }

    /// Sets a new static allocator object. If this is not called, the default
    /// implementation using malloc and free is used.
    static void TRITONAPI SetAllocator(Allocator *a) {
        if( a )
            sAllocator = a;
        else
            sAllocator = &sDefaultAllocator;
    }

protected:
    static Allocator *sAllocator;
    static Allocator sDefaultAllocator;

private:
    void *heap;
};

/** This base class for all Triton objects intercepts the new and delete operators,
   routing them through Triton::Allocator(). */
class MemObject
{
public:
#if (!(_MSC_VER < 1300))

    void *operator new(size_t bytes) {
        return TRITON_MALLOC(bytes);
    }

    void operator delete(void *p) {
        TRITON_FREE(p);
    }
#endif
};
}

// intercepting STL allocation under VC6 is just hopeless.
#if (_MSC_VER < 1300)

#define TRITON_VECTOR(T) std::vector< T >
#define TRITON_MAP(A, B) std::map< A, B >
#define TRITON_SET(T) std::set< T >
#define TRITON_LIST(T) std::list< T >
#define TRITON_STRING std::string
#define TRITON_STACK(T) std::stack< T >

#else

// Custom STL allocator

#define MAX_ALLOCATOR_SIZE 100E6

template <class T>
class TritonAllocator
{
public:
    // type definitions
    typedef T value_type;
    typedef T*       pointer;
    typedef const T* const_pointer;
    typedef T&       reference;
    typedef const T& const_reference;

    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    // rebind allocator to type U
    template <class U >
    struct rebind {
        typedef TritonAllocator< U > other;
    };

    // return address of values
    pointer address (reference value) const {
        return &value;
    }
    const_pointer address (const_reference value) const {
        return &value;
    }

    /* constructors and destructor
     * - nothing to do because the allocator has no state
     */
    TritonAllocator() throw() {
    }
    TritonAllocator(const TritonAllocator& ) throw() {
    }

    template <class U >
    TritonAllocator (const TritonAllocator< U > &) throw() {
    }

    ~TritonAllocator() throw() {
    }

    // return maximum number of elements that can be allocated
    size_type max_size () const throw() {
        return (size_type)(MAX_ALLOCATOR_SIZE / sizeof(T));
    }

    // allocate but don't initialize num elements of type T
    pointer allocate (size_type num, const void* = 0) {
        pointer ret = reinterpret_cast<T*>(::Triton::Allocator::GetAllocator()->alloc(num * sizeof(T)));
        return ret;
    }

    // initialize elements of allocated storage p with value value
    void construct (pointer p, const T& value) {
        // initialize memory with placement new
        ::new ((void*)p)T(value);
    }

    // destroy elements of initialized storage p
    void destroy (pointer p) {
        // destroy objects by calling their destructor
        p->~T();
    }

    // deallocate storage p of deleted elements
    void deallocate (pointer p, size_type ) {
        ::Triton::Allocator::GetAllocator()->dealloc(p);
    }
};

// return that all specializations of this allocator are interchangeable
template <class T1, class T2>
bool operator== (const TritonAllocator<T1>&, const TritonAllocator<T2>&) throw()
{
    return true;
}
template <class T1, class T2>
bool operator!= (const TritonAllocator<T1>&, const TritonAllocator<T2>&) throw()
{
    return false;
}

// Convenience macros for STL object using our allocator

#define TRITON_VECTOR(T) std::vector<T, TritonAllocator< T > >
#define TRITON_MAP(A, B) std::map<A, B, std::less< A >, TritonAllocator< std::pair<A const, B > > >
#define TRITON_SET(T) std::set<T, std::less< T >, TritonAllocator< T > >
#define TRITON_LIST(T) std::list<T, TritonAllocator< T > >
#define TRITON_STRING std::basic_string<char, std::char_traits<char>, TritonAllocator<char> >
#define TRITON_STACK(T) std::stack<T, std::deque< T, TritonAllocator< T > > >

#endif

// "Lazy" wrapper for handling STL objects that were static
template <class T>
class TRITON_LAZY
{
public:
    TRITON_LAZY() : obj(0) {}
    virtual ~TRITON_LAZY() {
        if (obj) TRITON_DELETE obj;
    }

    T& Get() {
        if (!obj) obj = TRITON_NEW T;
        return *obj;
    }

private:
    T* obj;
};

#endif
