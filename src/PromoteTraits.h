//-*-C++-*-

/*
 * Please do not edit this file.  It is automatically generated by
 * soft_swin/utils/units/generate_PromoteTraits.C, which is where you
 * can also find further details.
 *
 * Willem van Straten - 21 December 2004
 *  */

#ifndef __MEAL_PromoteTraits_h
#define __MEAL_PromoteTraits_h

class PromoteTraits_not_specialized_for_this_case { };

//! Empty template class requires specialization 
template<typename A, typename B>
class PromoteTraits {
  typedef PromoteTraits_not_specialized_for_this_case promote_type;
};

//! Partial template specialization for template types
template<typename A, typename B, template<typename> class C>
class PromoteTraits< C<A>, B > {
public:
  typedef C<typename PromoteTraits<A,B>::promote_type> promote_type;
};

//! Partial template specialization for template types
template<typename A, typename B, template<typename> class C>
class PromoteTraits< A, C<B> > {
public:
  typedef C<typename PromoteTraits<A,B>::promote_type> promote_type;
};

//! Partial template specialization for template types
template<typename A, typename B, template<typename> class C>
class PromoteTraits< C<A>, C<B> > {
public:
  typedef C<typename PromoteTraits<A,B>::promote_type> promote_type;
};

#include <complex>

// forward declaration
namespace MEAL {
  class ScalarMath;
}

template<>
class PromoteTraits< float, float > {
public:
  typedef float promote_type;
};

template<>
class PromoteTraits< float, double > {
public:
  typedef double promote_type;
};

template<>
class PromoteTraits< float, long double > {
public:
  typedef long double promote_type;
};

template<>
class PromoteTraits< float, MEAL::ScalarMath > {
public:
  typedef MEAL::ScalarMath promote_type;
};

template<>
class PromoteTraits< double, float > {
public:
  typedef double promote_type;
};

template<>
class PromoteTraits< double, double > {
public:
  typedef double promote_type;
};

template<>
class PromoteTraits< double, long double > {
public:
  typedef long double promote_type;
};

template<>
class PromoteTraits< double, MEAL::ScalarMath > {
public:
  typedef MEAL::ScalarMath promote_type;
};

template<>
class PromoteTraits< long double, float > {
public:
  typedef long double promote_type;
};

template<>
class PromoteTraits< long double, double > {
public:
  typedef long double promote_type;
};

template<>
class PromoteTraits< long double, long double > {
public:
  typedef long double promote_type;
};

template<>
class PromoteTraits< long double, MEAL::ScalarMath > {
public:
  typedef MEAL::ScalarMath promote_type;
};

template<>
class PromoteTraits< MEAL::ScalarMath, float > {
public:
  typedef MEAL::ScalarMath promote_type;
};

template<>
class PromoteTraits< MEAL::ScalarMath, double > {
public:
  typedef MEAL::ScalarMath promote_type;
};

template<>
class PromoteTraits< MEAL::ScalarMath, long double > {
public:
  typedef MEAL::ScalarMath promote_type;
};

template<>
class PromoteTraits< MEAL::ScalarMath, MEAL::ScalarMath > {
public:
  typedef MEAL::ScalarMath promote_type;
};

#define PROMOTE_TRAITS_SPECIALIZE 1

#endif // !__MEAL_PromoteTraits_h

