
| OSX/Linux                                                                                                       | Windows                                                                                                                                  |
| --------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| [![Build Status](https://travis-ci.org/Srekel/sralloc.svg?branch=master)](https://travis-ci.org/Srekel/sralloc) | [![Build Status](https://ci.appveyor.com/api/projects/status/9vxpg5jccc3pas2j?svg=true)](https://ci.appveyor.com/project/Srekel/sralloc) |
****

# sralloc

**sralloc** is a set of memory allocators written in **politely coded C99** (possibly with C++ wrappers in the future). They are intended to be more or less **interchangeable**, **stackable**, and **swappable**, in addition to being **performant** and **visualizable**.

## What does that mean and what is it good for, you ask?

The main things sralloc will give you are:

- Heap allocation performance that is very near something like rpmalloc (don't be alarmed, I'm not reinventing the wheel - sralloc *wraps* rpmalloc, dlmalloc, or malloc, or any other generic heap allocator you prefer, with little overhead).
- Special case performant allocators, like stackframe or slot.
- Memory tracking in a hierarcical manner, so can find memory leaks and easily see which parts of your application are using memory.
- Debuggability, in case you're having use-after-free or buffer overrun bugs.

sralloc is mainly intended for game development but there isn't really anything game-specific in it.

## Introduction to allocators

This section serves as both a usage guide and a beginner's tutorial for people who don't know anything about memory allocators, how they can be used, and why they are common in the games industry. If you want to get straight to the description of the allocators in sralloc, skip this section.

(By the way, much of this library is inspired by the Stingray engine's take on allocators, so in addition to reading this, I also recommend checking out the Bitsquid (same thing, different name) blog and Niklas Gray's videos on Stingray. References can be found at the bottom of this document.)

### The bare minimum

Let's say you're just starting out and you want the simplest thing possible. For now you just want to use malloc as your main **backing allocator**. This is how you would do that:

```C
#define SRALLOC_IMPLEMENTATION // Standard single-header-library detail
#include <sralloc.h>

void main() {
    srallocator_t* mallocalloc = sralloc_create_malloc_allocator( "root" );
    void* ptr = sralloc_alloc( mallocalloc, 1024 );

    // ...do things with ptr...

    sralloc_dealloc( allocator, ptr );
    sralloc_destroy_malloc_allocator( mallocalloc );
}
```

The **malloc allocator** is stupid simple - it simply calls `malloc` and `free` to manage its allocations. It also collects some statistics and handles memory alignment - more on that later.

Lots of ado about nothing, right? You could just call malloc and free directly and lose all the cruft. Well yes, but...

### The basics

Let's say that you're writing a game, and you've just set up a basic simulation - sim simulation + rendering, and you'd like to use separate allocators for them so that you can know how much memory the different parts use, and what their allocation patterns looks like.

Let's see what that might look like.

```C
void main() {
    srallocator_t* mallocalloc = sralloc_create_malloc_allocator( "root" );
    srallocator_t* simalloc = sralloc_create_proxy_allocator( mallocalloc, "sim" );
    srallocator_t* graphicsalloc = sralloc_create_proxy_allocator( mallocalloc, "graphics" );

    Simulation* sim = SRALLOC_OBJECT( simalloc, Simulation );
    Graphics* graphics = SRALLOC_OBJECT( graphicsalloc, Graphics );
    sim->allocator = simalloc;
    graphics->allocator = graphicsalloc;

    setup_sim(sim);
    setup_graphics(graphics);

    bool quit = false;
    while (!quit) {
        quit = update_simulation(sim);
        render_game(sim, graphics);
    }

    destroy_sim(sim)
    destroy_graphics(graphics)

    sralloc_dealloc( allocator, sim );
    sralloc_dealloc( allocator, graphics );
    sralloc_destroy_proxy_allocator( simalloc );
    sralloc_destroy_proxy_allocator( graphicsalloc );
    sralloc_destroy_malloc_allocator( mallocalloc );
}

void setup_sim(Simulation* sim) {
    srallocator_t* sys1alloc = sralloc_create_proxy_allocator( sim->allocator, "sys1" );
    sim->system1 = create_sys1(sysalloc1);
}

void destroy_sim(Simulation* sim) {
    srallocator_t* sys1alloc = sralloc_create_proxy_allocator( sim->allocator, "sys1" );
    sralloc_destroy_proxy_allocator(sim->system1->allocator)
}

```

It's still not very exciting, but one perk you get immediately is that if you haven't cleaned up properly - if one of the allocators hasn't freed **exactly** the same amount of memory that it has allocated, it will assert, letting you know which allocator failed, and how many allocations and the amount of memory it still had allocated. Memory leaks begone, just like that.

Of course, allocation tracking and statistics is something you can disable, since it has some performance and memory overhead. Simply `#define SRALLOC_DISABLE_STATS` before including the header.

So what does the "proxy" allocator do? Simple - it forwards any allocations to its **backing allocator** - in this case, the malloc allocator (we pass it in to the create function, see?). And like every other allocator, it collects stats and aligns memory if you so wish.

As you can see, we pass in a "parent", or backing allocator to the proxy allocators (most allocators have this). This creates a hiearchy of allocators that you can use if you would like to show some statistics. More on that later.

#### A note on the macros and API

We used the utility macro `SRALLOC_OBJECT`. It takes a type as the second parameter, allocates something of its size, and casts the return value. It does NOT initialize the value, and obviously, does not do inplace new (that's C++ after all) but I am intending to add utility macros for that too, since it can be quite nice.

There's another utility macro `SRALLOC_ALIGNED_ARRAY` which takes a type and a count and allocates an array of that size.

For consistency, there's also a `SRALLOC_BYTES` that simply wraps `sralloc_alloc`, and `SRALLOC_DEALLOC` that wraps `sralloc_dealloc`.

For those macros, there are also matching macros that returns aligned pointers, `SRALLOC_ALIGNED_BYTES` and so on.

As you may have guessed, `sralloc_alloc` and `sralloc_alloc_aligned` are the "core" functions that you will call to allocate memory.

There's one more detail that's worth mentioning here. There's also `sralloc_alloc_with_size` (and aligned) that gives you a struct result back:

```C
typedef struct {
    void*   ptr;
    srint_t size;
} sr_result_t;
```

Sometimes, an allocator can return **more** memory than you requested. In some cases you may be able to take advantage of this, and that's where this comes in.

### Adding a Frame allocator

In Sweden we say that "a beloved child has many names". I'm not sure where it comes from but it's certainly true for the Frame allocator. I've heard "arena allocator", "scratch allocator", "stack allocator", "frame allocator", "stack frame allocator", and "temp allocator", and as far as I know, they all mean the same thing.

Let's see how one might be useful.

In the example above, there's an update loop. A common pattern in games is that memory needs to be allocated during a frame and then only gets used during that frame. This fact can be abused.

Here are some issues.
- Sometimes, the lifetime of a variable on the stack is not enough.
- Or the stack may not be big enough to allocate as much memory as you need.
- Perhaps you need to allocate a pointer in one system, then pass it off to another system that doesn't get updated until later in the update frame.
- If it's too much data, it's not possible or desireable to pass it by value.
- Sometimes the receiving system can't immediately copy the incoming data to an internal buffer.

One solution to this is to malloc some memory, pass the pointer and rely on the receiving system to free it. This is generally a very bad solution - unless you like memory leaks. It's almost always the case that you want the place in the code that deallocates memory to be very close to the code that allocated it.

Another solution is to go full C++ and use something like `shared_ptr`, but as you well know, **you never go full C++**. Joking aside, the games industry seem to be moving away from shared_ptrs and the like. We used it on Just Cause 2 and didn't realize the performance, compile-time, and executable-size implications until too late (don't ask me for numbers, it was many years ago, I can only say that for a period of time, rebuilding the JC2 code base took 40+ minutes. I don't know how much of that was due to shared_ptr, but hopefully you can appreciate the operational cost of 50 programmers rolling their thumbs for hours each day).

Even if you **can** malloc and free the pointer inside your system, like this....

```C
void sys1_update(System1* sys) {
    for (i = 0; i < 1000) {
        MyObject* obj = malloc(sizeof(MyObject));
        init_obj(sys, i, obj);
        sys2_api(obj);
        free(obj);
    }
}
```

... it might not be a good solution because heap allocations can be relatively expensive.

Here's how using a frame allocator would fix the problem:

```C
void main() {
    // ...
    srallocator_t* stack_allocator = sralloc_create_stack_allocator("my_frame", mallocalloc, 10000);

    bool quit = false;
    while (!quit) {
        sralloc_stack_allocator_clear(stack_allocator);
        quit = update_simulation(sim, stack_allocator);
        render_game(sim, graphics, stack_allocator);
    }

    sralloc_destroy_stack_allocator(stack_allocator);
    // ...
}

void sys1_update(System1* sys, srallocator_t* stack_allocator) {
    for (i = 0; i < 1000) {
        MyObject* obj = SRALLOC_OBJECT(stack_allocator, System2Object);
        init_obj(sys, i, obj);
        syst2_api(obj);
    }
}

```

We create a frame allocator with a predetermined size. Systems are free to allocate memory from it and don't have to care about freeing it. (In fact, deallocing a stack allocated pointer does nothing!)

So a frame allocator is faster for two reasons:
- You don't have deallocate memory.
- Allocations themselves are very fast.



 We still have the problem that if the memory runs out, it'll crash. There are a few reasons why this is still a better solution:
- You're sharing the allocator between multiple systems - one system can use more memory one frame while another uses less.
-
- It will assert, which is a bit nicer

## License

MIT/PD

## TODO
- Optional assert on allocation fail (instead of return 0)
- Rename stack allocator
- Mutex allocator
- rpmalloc wrapper
- Ensure as much overhead as possible can be disabled in release builds
- C++ API

## References
- Stingray Engine Code Video Walkthrough: [#2 Memory](https://www.youtube.com/watch?v=pGXEsVasv_o)
- Bitsquid blog: [Custom Memory Allocation in C++](http://bitsquid.blogspot.se/2010/09/custom-memory-allocation-in-c.html)
- Nicholas Frechette blog: [Stack Frame Allocators]( https://nfrechette.github.io/2016/05/08/stack_frame_allocators/)
