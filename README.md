
| OSX/Linux                                                                                                       | Windows                                                                                                                                  |
| --------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| [![Build Status](https://travis-ci.org/Srekel/sralloc.svg?branch=master)](https://travis-ci.org/Srekel/sralloc) | [![Build Status](https://ci.appveyor.com/api/projects/status/9vxpg5jccc3pas2j?svg=true)](https://ci.appveyor.com/project/Srekel/sralloc) |
****

# sralloc

**sralloc** is a set of memory allocators for C (possibly with C++ wrappers in the future). They are intended to be more or less **interchangeable**, **stackable**, and **swappable**, in addition to being **performant** and **visualizable**.

### What does that mean and what is it good for, you ask?

The main things sralloc will give you are:
- Heap allocation performance that is very near something like rpmalloc (because it *wraps* rpmalloc, dlmalloc, or malloc, or any other generic heap allocator you prefer, with little overhead).
- Special case performant allocators, like stackframe or slot.
- Memory tracking in a hierarcical manner, so can find memory leaks and easily see which parts of your application are using memory.
- Debuggability, in case you're having use-after-free or buffer overrun bugs.

### How to use

Let's say you're just starting out and you want the simplest thing possible. For now you just want to use malloc as your main **backing allocator**. This is the gist of using it:

```
    srallocator_t* mallocalloc = sralloc_create_malloc_allocator( "root" );
    void* ptr = sralloc_alloc( mallocalloc, 1024 );

    // ...do things with ptr...

    sralloc_dealloc( allocator, res.ptr );
    sralloc_destroy_malloc_allocator( mallocalloc );
```

Lots of ado for nothing, right? You could just use malloc and free and lose all the cruft. Well yes, but... Let's say that you're writing a game, and you have all these systems, and you'd like to know how much memory the different paths use. I mean, you have a rough idea of course, but it's nice to know for sure, especially since you're targetting a limited-memory platform like a console or smart phone.

Let's see what that might look like.

```
    srallocator_t* mallocalloc = sralloc_create_malloc_allocator( "root" );

    Physics* physics = SRALLOC_OBJECT( mallocalloc, 1024 );

    // ...do things with ptr...

    sralloc_dealloc( allocator, res.ptr );
    sralloc_destroy_malloc_allocator( mallocalloc );
```



## License

MIT/PD
