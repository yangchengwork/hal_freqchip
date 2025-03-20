#ifndef FR_COMMON_DBC_H
#define FR_COMMON_DBC_H

#ifdef __cplusplus
extern "C" {    // allow C++ to use these headers
#endif

// FR_NASSERT macro disables all contract validations
// (assertions, preconditions, postconditions, and invariants).
#ifndef FR_NASSERT 

// callback invoked in case of assertion failure
extern void onAssert__(char const *file, unsigned line);

#define FR_DEFINE_THIS_FILE static const char THIS_FILE__[] = __FILE__

#define FR_ASSERT(test_)   \
        if (test_) {    \
        }               \
        else onAssert__(THIS_FILE__, __LINE__)

#define FR_REQUIRE(test_)           FR_ASSERT(test_)
#define FR_ENSURE(test_)            FR_ASSERT(test_)
#define FR_INVARIANT(test_)         FR_ASSERT(test_)
#define FR_ALLEGE(test_)            FR_ASSERT(test_)

#else   // FR_NASSERT

#define FR_DEFINE_THIS_FILE extern const char THIS_FILE__[]
#define FR_ASSERT(test_)
#define FR_REQUIRE(test_)
#define FR_ENSURE(test_)
#define FR_INVARIANT(test_)
#define FR_ALLEGE(test_)   \
   if(test_) {          \
   }                    \
   else

#endif  // FR_NASSERT


#ifdef __cplusplus
}
#endif

#endif  // FR_COMMON_DBC_H

