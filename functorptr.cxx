// Cross platform Functor to Function pointer conversion.
//
// Works unmodified on:
//    * CentOS 4.x 32 or 64bit (Even w/ NX support)
//    * Win32 /GS + /O
//    * Fedora (ExecShield and/or grsec segment-based proection)
//    * Free,NetBSD 32bit
//    * MacOS intel
// Broken Under:
//    * Grsec boxes with real NX support
// Minor issues:
//    * 64bit g++ requires at least -O1 to use 64bit immediates
//      (required for this to work)
#include <iostream>

typedef unsigned long u_long;
typedef unsigned char u_char;

// Well, getting the brain out was the easy part.
// The hard part was getting the brain /out/.

using namespace std;

typedef bool (*fptr_t)();

// This unfortunately must be global static so that Win32 doesn't use
// the IAT.
static bool closure();

// -1 is sign extended on x64. Need unsigned max
#define MAX_INT (((u_long)-1)>>1)

#define MAX_FCN_SIZE (128)

class Functor
{
public:
    int a_;
    int b_;

    Functor(int a, int b) : a_(a), b_(b) { }

    // Need virtual so there is no relative call or g++ thunk BS
    virtual bool operator()(void) {
        cerr << "A: " << a_ << ", B: " << b_ << endl;
        return true;
    }

    fptr_t gen_closure() {
        // Crazy anding is b/c of alignment issues.. Will break free, but
        // is needed on CentOS.. Could roll my own malloc aligner, but meh.
        u_char *start_ptr = (u_char*)(((u_long)malloc(MAX_FCN_SIZE+16)) & ~(0xF));
        memcpy(start_ptr, (void*)closure, MAX_FCN_SIZE);
        cerr << "start is " << (void *)start_ptr << " closure is "
            << (void*)closure << endl;

        for(u_char *end_ptr = start_ptr; end_ptr - start_ptr < MAX_FCN_SIZE; end_ptr++) {
            if(*(Functor**)end_ptr == (Functor*)MAX_INT) {
                *(Functor**)end_ptr = this;
                cerr << "Found at " << (void*)end_ptr << endl;
            }
        }

        return (fptr_t)start_ptr;
    }

};

/* This is where the magic happens. The code of this function is actually
 * copied in gen_closure, which looks for the MAX_INT marker to replace
 * it in malloc'd copies with 'this'.
 *
 * (Yes, I am a black wizard with no soul to speak of ;)
 */
static bool closure() {
    Functor *thisptr = (Functor *)MAX_INT;
    return (*thisptr)();
}



int main(int argc, char **argv)
{
    Functor a(2,3);
    Functor *b = &a;
    fptr_t ptr = a.gen_closure();
    a();
    (*b)();

    a.a_ = 42;
    a.b_ = 23;

    ptr();
}
