/**************\
**  Random.h  ***************************************************\
**                                                              **
**    Random.h provides a number of Random Number Generators,   **
**    (RNG).  These are based on a posting by George Marsaglia  **
**    post to Sci.Stat.Math on 1/12/99.                         **
**                                                              **
**    For more complete info from George Marsaglia, read the    **
**    comments at the ent of the file.  All names are prefaced  **
**    with 'RND_' to prevent conflicts with common variables.   **
**                                                              **
**    Usage:                                                    **
**      Call RND_Init() to initialize the seeds.                **
**      All generators are expanded inline for optimization.    **
**      You can use the following random number generators to   **
**      produce a random 32 bit number.                         **
**        - RND_MWC   =  multiply-with-carry generator          **
**        - RND_SHR3  =  3-shift-register generator             **
**        - RND_CONG  =  congruential generator                 **
**        - RND_KISS  =  a combination of the 3 above           **
**      If USE_RND_TABLES is defined, these are also defined:   **
**        - RND_LFIB4 =  lagged Fibonacci generators            **
**        - RND_SWB   =  subtract-with-borrow generator         **
**                       (does not work on all compilers)       **
**                                                              **
**      For floating point numbers, use the following:          **
**        - RND_UNI   = Float from 0.0 to 1.0                   **
**        - RND_VNI   = Float from -1.0 to 1.0                  **
**                                                              **
\****************************************************************/

#ifndef _RANDOM_H_
#define _RANDOM_H_

#include <cstdlib>
#include <time.h>

// Allows additional RND that use a random table.  If not using LFIB4 or SWB, then
//   not including these means no table is needed, saving space and init time.
#define USE_RND_TABLES

#define RND_UL unsigned long
#define RND_znew  ((RND_z=36969*(RND_z&65535)+(RND_z>>16))<<16)
#define RND_wnew  ((RND_w=18000*(RND_w&65535)+(RND_w>>16))&65535)
#define RND_MWC   (RND_znew+RND_wnew)
#define RND_SHR3  (RND_jsr=(RND_jsr=(RND_jsr=RND_jsr^(RND_jsr<<17))^(RND_jsr>>13))^(RND_jsr<<5))
#define RND_CONG  (RND_jcong=69069*RND_jcong+1234567)
#define RND_KISS  ((RND_MWC^RND_CONG)+RND_SHR3)
#define RND_UNI   (RND_KISS*2.328306e-10)
#define RND_VNI   ((long) RND_KISS)*4.656613e-10

// If using random tables, include those routines
#ifdef USE_RND_TABLES
  #define RND_LFIB4 (RND_t[RND_c]=RND_t[RND_c]+RND_t[RND_c+58]+RND_t[RND_c+119]+RND_t[++RND_c+178])
  #define RND_SWB   (RND_t[RND_c+237]=(RND_x=RND_t[RND_c+15])-(RND_y=RND_t[++RND_c]+(RND_x<RND_y)))
  static RND_UL RND_t[256];
#endif

/*  Global static variables: */
 static RND_UL RND_z=362436069, RND_w=521288629, RND_jsr=123456789, RND_jcong=380116160;
 static RND_UL RND_x=0,RND_y=0; static unsigned char RND_c=0;


void RND_Init (void)
{ srand((unsigned)time(NULL));
  RND_z = rand(); RND_w = rand(); RND_jsr = rand(); RND_jcong = rand();
  #ifdef USE_RND_TABLES
    for(int i=0;i<256;i++) RND_t[i]=rand();
  #endif
}

#endif



// Any one of KISS, MWC, LFIB4, SWB, SHR3, or CONG
// can be used in an expression to provide a random
// 32-bit integer, and UNI in an expression will
// provide a random uniform in (01), or VNI in (-1,1).
// For example, for int i, float v; i=(MWC>>24); will
// provide a random byte, while v=4.+3.*UNI; will
// provide a uniform v in the interval 4. to 7.
// For the super cautious, (KISS+SWB) in an expression
// would provide a random 32-bit integer from
// a sequence with period > 27700, and would only
// add some 300 nanoseconds to the computing
// time for that expression.

// The KISS generator, (Keep It Simple Stupid), is
// designed to combine the two multiply-with-carry
// generators in MWC with the 3-shift register SHR3
// and the congruential generator CONG, using
// addition and exclusive-or. Period about 2123. It
// is one of my favorite generators.

// The  MWC generator concatenates two 16-bit
// multiply-with-carry generators, x(n)=36969x(n-1)+carry,
// y(n)=18000y(n-1)+carry  mod 216, has  period  about
// 260 and seems to pass all tests of randomness. A favorite
// stand-alone generator---faster than KISS, which contains it.

// SHR3 is a 3-shift-register generator with
// period 232-1. It uses
// y(n)=y(n-1)(I+L17)(I+R13)(I+L5), with the
// y's viewed as binary vectors, L the 32x32
// binary matrix that shifts a vector left 1, and
// R its transpose.  SHR3 seems to pass all except
// the binary rank test, since 32 successive
// values, as binary vectors, must be linearly
// independent, while 32 successive truly random
// 32-bit integers, viewed as binary vectors, will
// be linearly independent only about 29% of the time.

// CONG is a congruential generator with the
// widely used 69069 as multiplier:
// x(n)=69069x(n-1)+1234567.  It has period 232.
// The leading half of its 32 bits seem to pass
// all tests, but bits in the last half are too
// regular.

// LFIB4 is an extension of the class that I have
// previously defined as  lagged Fibonacci
// generators: x(n)=x(n-r) op x(n-s), with the x's
// in a finite set over which there is a binary
// operation op, such as +,- on integers mod 232,
// * on odd such integers, exclusive-or (xor) on
// binary vectors. Except for those using
// multiplication, lagged Fibonacci generators
// fail various tests of randomness, unless the
// lags are very long.  To see if more than two
// lags would serve to overcome the problems of 2-
// lag generators using +,- or xor, I have
// developed the 4-lag generator LFIB4:
// x(n)=x(n-256)+x(n-179)+x(n-119)+x(n-55) mod 232.
// Its period is 231*(2256-1), about 2287, and
// it seems to pass all tests---in particular,
// those of the kind for which 2-lag generators
// using +,-,xor seem to fail.  For even more
// confidence in its suitability,  LFIB4 can be
// combined with KISS, with a resulting period of
// about 2410: just use (KISS+LFIB4) in any C
// expression.

// SWB is a subtract-with-borrow generator that I
// developed to give a simple method for producing
// extremely long periods:
// x(n)=x(n-222)-x(n-237)-borrow mod 232.
// The 'borrow' is 0 unless set to 1 if computing
// x(n-1) caused overflow in 32-bit integer
// arithmetic. This generator has a very long
// period, 27098(2480-1), about 27578. It seems
// to pass all tests of randomness, but,
// suspicious of a generator so simple and fast
// (62 nanosecs at 300MHz), I would suggest
// combining SWB with KISS, MWC, SHR3, or CONG.

// Finally, because many simulations call for
// uniform random variables in 0<v<1 or -1<v<1, I
// use #define statements that permit inclusion of
// such variates directly in expressions:  using
// UNI will provide a uniform random real (float)
// in (0,1), while VNI will provide one in (-1,1).

// All of these: MWC, SHR3, CONG, KISS, LFIB4,
// SWB, UNI and VNI, permit direct insertion of
// the desired random quantity into an expression,
// avoiding the time and space costs of a function
// call. I call these in-line-define functions.
// To use them, static variables z,w,jsr and
// jcong should be assigned seed values other than
// their initial values.  If LFIB4 or SWB are
// used, the static table t[256] must be
// initialized.  A sample procedure follows.

// A note on timing:  It is difficult to provide
// exact time costs for inclusion of one of these
// in-line-define functions in an expression.
// Times may differ widely for different
// compilers, as the C operations may be deeply
// nested and tricky. I suggest these rough
// comparisons, based on averaging ten runs of a
// routine that is essentially a long loop:
// for(i=1;i<10000000;i++) L=KISS; then with KISS
// replaced with SHR3, CONG,... or KISS+SWB, etc.
// The times on my home PC, a Pentium 300MHz, in
// nanoseconds: LFIB4=64; CONG=90; SWB=100;
// SHR3=110; KISS=209; KISS+LFIB4=252; KISS+SWB=310.
