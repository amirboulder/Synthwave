//This file is AI generated.

module;

//=============================================================================
// Global module fragment.
//
// glm is header-only, so unlike Jolt/flecs there is no .lib to link against.
// The headers still MUST be included here, above "export module", for a
// different reason: pch.h includes glm textually into every non-module TU, and
// MathComponents.hpp puts glm::vec3 inside component structs. Including glm
// below "export module GLM;" would attach vec3 to this module, and a component
// as seen by a header TU would be a DIFFERENT TYPE from the same component as
// seen by an importing module. GMF + re-export keeps one global-module entity.
//
// MACROS DO NOT CROSS A MODULE BOUNDARY - and for glm that is not just an
// inconvenience, it is an ODR hazard. GLM_FORCE_* / GLM_ENABLE_EXPERIMENTAL
// change glm's DEFINITIONS. qualifier.hpp:26 makes defaultp == aligned_highp
// under GLM_FORCE_DEFAULT_ALIGNED_GENTYPES, i.e. a different vec3 LAYOUT.
// Since this module and pch.h produce the same entities, a macro defined in
// one and not the other is an undiagnosed ODR violation.
// => define glm config macros with target_compile_definitions, never in pch.h.
//=============================================================================

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/norm.hpp>        // length2 - reached only transitively today

export module GLM;

export namespace glm {

	//--- Primary templates and configuration ---------------------------------
	// vec3 etc. are typedefs for vec<3, float, defaultp>. The specialization is
	// reachable through the typedef, but the NAME glm::vec is not visible unless
	// exported - needed by any template taking vec<L, T, Q>.
	using glm::vec;
	using glm::mat;
	using glm::qua;
	using glm::length_t;
	using glm::qualifier;
	// Enumerators of an unscoped enum are namespace-scope names; exporting the
	// enum type alone does not bring them.
	using glm::defaultp;
	using glm::highp;
	using glm::mediump;
	using glm::lowp;

	//--- Vector / matrix / quaternion aliases --------------------------------
	using glm::vec1;  using glm::vec2;  using glm::vec3;  using glm::vec4;
	using glm::ivec1; using glm::ivec2; using glm::ivec3; using glm::ivec4;
	using glm::uvec1; using glm::uvec2; using glm::uvec3; using glm::uvec4;
	using glm::bvec1; using glm::bvec2; using glm::bvec3; using glm::bvec4;
	using glm::dvec1; using glm::dvec2; using glm::dvec3; using glm::dvec4;

	using glm::mat2;   using glm::mat3;   using glm::mat4;
	using glm::mat2x2; using glm::mat2x3; using glm::mat2x4;
	using glm::mat3x2; using glm::mat3x3; using glm::mat3x4;
	using glm::mat4x2; using glm::mat4x3; using glm::mat4x4;
	using glm::dmat3;  using glm::dmat4;

	using glm::quat;
	using glm::dquat;

	//--- Operators -----------------------------------------------------------
	// THE BLOCK THAT MAKES ARITHMETIC WORK. Naming an operator exports every
	// overload of it in namespace glm, so this covers vec, mat and qua at once.
	using glm::operator+;
	using glm::operator-;
	using glm::operator*;
	using glm::operator/;
	using glm::operator%;
	using glm::operator&;
	using glm::operator|;
	using glm::operator^;
	using glm::operator<<;
	using glm::operator>>;
	using glm::operator~;
	//using glm::operator!;
	using glm::operator&&;
	using glm::operator||;
	using glm::operator==;
	using glm::operator!=;

	//--- Trigonometry / angles -----------------------------------------------
	using glm::radians;
	using glm::degrees;
	using glm::sin;    using glm::cos;    using glm::tan;
	using glm::asin;   using glm::acos;   using glm::atan;

	//--- Exponential / common ------------------------------------------------
	using glm::pow;    using glm::exp;    using glm::log;
	using glm::exp2;   using glm::log2;
	using glm::sqrt;   using glm::inversesqrt;

	using glm::abs;    using glm::sign;
	using glm::floor;  using glm::ceil;   using glm::round;  using glm::trunc;
	using glm::fract;  using glm::mod;    using glm::modf;
	using glm::min;    using glm::max;    using glm::clamp;
	using glm::mix;    using glm::step;   using glm::smoothstep;
	using glm::isnan;  using glm::isinf;

	//--- Geometric -----------------------------------------------------------
	using glm::length;
	using glm::distance;
	using glm::dot;
	using glm::cross;
	using glm::normalize;
	using glm::faceforward;
	using glm::reflect;
	using glm::refract;
	using glm::length2;     // gtx/norm.hpp
	using glm::distance2;

	//--- Matrix --------------------------------------------------------------
	using glm::transpose;
	using glm::inverse;
	using glm::determinant;
	using glm::matrixCompMult;
	using glm::outerProduct;

	//--- Relational ----------------------------------------------------------
	using glm::equal;
	using glm::notEqual;
	using glm::lessThan;
	using glm::lessThanEqual;
	using glm::greaterThan;
	using glm::greaterThanEqual;
	using glm::all;
	using glm::any;
	using glm::not_;

	//--- Constants -----------------------------------------------------------
	// Function TEMPLATES, not variables - no internal-linkage problem, and they
	// keep their real values across the boundary. Call as glm::pi<float>().
	using glm::pi;
	using glm::two_pi;
	using glm::half_pi;
	using glm::epsilon;
	using glm::zero;
	using glm::one;

	//--- Quaternion ----------------------------------------------------------
	using glm::angleAxis;
	using glm::eulerAngles;
	using glm::angle;
	using glm::axis;
	using glm::conjugate;
	using glm::slerp;
	using glm::lerp;
	using glm::pitch;  using glm::yaw;   using glm::roll;
	using glm::quat_cast;
	using glm::mat3_cast;
	using glm::mat4_cast;
	using glm::toMat4;      // gtx/quaternion.hpp
	using glm::toMat3;
	using glm::toQuat;
	using glm::quatLookAt;
	using glm::quatLookAtRH;
	// glm::rotate is overloaded across matrix_transform AND gtx/quaternion;
	// one using-declaration exports both families.
	using glm::rotate;

	//--- Transform / projection ----------------------------------------------
	using glm::identity;
	using glm::translate;
	using glm::scale;
	using glm::lookAt;
	using glm::lookAtRH;
	using glm::lookAtLH;
	using glm::perspective;
	using glm::perspectiveRH_ZO;
	using glm::infinitePerspective;
	using glm::ortho;
	using glm::orthoRH_ZO;
	using glm::project;
	using glm::unProject;
	using glm::decompose;   // gtx/matrix_decompose.hpp

	//--- Raw pointer access --------------------------------------------------
	using glm::value_ptr;
	using glm::make_vec2; using glm::make_vec3; using glm::make_vec4;
	using glm::make_mat3; using glm::make_mat4;
	using glm::make_quat;
}


