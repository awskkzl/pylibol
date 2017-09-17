/*********************************************************************************
*     File Name           :     expression.h
*     Created By          :     yuewu
*     Description         :     expression base templates
**********************************************************************************/
#ifndef SOL_MATH_EXPRESSION_H__
#define SOL_MATH_EXPRESSION_H__

#include <stdexcept>
#include <sstream>

#include <sol/util/types.h>
#include <sol/math/operator.h>
#include <sol/math/shape.h>

namespace sol {
namespace math {
namespace expr {

/// \brief  type of expression
namespace ExprType {
/// \brief  value expression
const int kValue = 0;
/// \brief  dense expression
const int kDense = 1;
/// \brief  sparse expression
const int kSparse = 3;
}

/// \brief  base class for expression templates
///
/// \tparam SubType inherited class type
/// \tparam DType data type of each element in the expression
/// \tparam expr_type expression type
template <typename SubType, typename DType, int expr_type>
struct Exp {
  /// \brief  pointer to the current class instance
  inline SubType *selfptr() { return static_cast<SubType *>(this); }
  inline const SubType *selfptr() const {
    auto tmp = static_cast<const SubType *>(this);
    return static_cast<const SubType *>(this);
  }
  /// \brief  subtype instance of the current class
  inline SubType &self() { return *(static_cast<SubType *>(this)); }
  inline const SubType &self() const {
    return *(static_cast<const SubType *>(this));
  }

  inline int type() const { return expr_type; }
};

/// \brief  expression for value classes
///
/// \tparam CType class type
/// \tparam DType element data type
template <typename CType, typename DType>
struct MatrixBaseExp;

template <typename OP, typename CType, typename DType, typename EType,
          int expr_type>
void CalcExp(MatrixBaseExp<CType, DType> &dst,
             const Exp<EType, DType, expr_type> exp);

/// \brief  engine to execute the expressions, this engine works to map from
// expression templates to class-specific operations
///
/// \tparam OP operator
/// \tparam CType the actual class type to save the result
/// \tparam DType element data type of CType
template <typename OP, typename CType, typename DType>
struct ExpEngine {
  template <typename EType, int expr_type>
  inline static void Calc(CType &dst, const Exp<EType, DType, expr_type> &exp) {
    CalcExp<OP>(dst, exp);
  }
};

//---------------
// common expressions
// --------------

/// \brief  scalar expression
///
/// \tparam DType data type of the scalar
template <typename DType>
struct ScalarExp : public Exp<ScalarExp<DType>, DType, ExprType::kValue> {
  DType value_;
  /// \brief  implicit constructor, MUST NOT BE explicit
  ScalarExp(DType scalar) : value_(scalar) {}

  inline DType operator()(size_t x, size_t y) const { return value_; }
  inline DType operator[](size_t idx) const { return value_; }
  inline index_t index(size_t idx) const { return 0; }
  inline DType value(size_t idx) const { return value_; }

  inline Shape<2> shape() const { return Shape<2>(); }
};

/// \brief  make a scalar expression
template <typename DType>
inline ScalarExp<DType> MakeExp(DType val) {
  return ScalarExp<DType>(val);
}

//---------------
// BinaryMapExp
// --------------
/// \brief  binary map expression lhs [op] rhs
///
/// \tparam OP operator
/// \tparam exptype type of resulted expression
/// \tparam EType1 expression type of lhs
/// \tparam exptype1 type of expression 1
/// \tparam EType2 expression type of rhs
/// \tparam exptype2 type of expression 2
/// \tparam DType data element type
template <typename OP, int exptype, typename EType1, int exptype1,
          typename EType2, int exptype2, typename DType>
struct BinaryMapExp
    : public Exp<
          BinaryMapExp<OP, exptype, EType1, exptype1, EType2, exptype2, DType>,
          DType, exptype> {
  /// \brief  left operand
  const EType1 &lhs;
  /// \brief  right operand
  const EType2 &rhs;
  /*! \brief constructor */
  BinaryMapExp(const Exp<EType1, DType, exptype1> &_lhs,
               const Exp<EType2, DType, exptype2> &_rhs)
      : lhs(_lhs.self()), rhs(_rhs.self()) {}

  /// accessing elements
  inline DType operator()(size_t x, size_t y) const {
    return OP::map(lhs(x, y), rhs(x, y));
  }
  inline DType operator[](size_t idx) const {
    return OP::map(lhs[idx], rhs[idx]);
  }

  inline Shape<2> shape() const {
    const Shape<2> &shape1 = lhs.shape();
    const Shape<2> &shape2 = rhs.shape();
    if (shape1.size() == 0) return shape2;
    if (shape2.size() == 0) return shape1;
    if (shape1 == shape2) {
      return shape1;
    }

    std::ostringstream oss;
    oss << "BinaryMapExp: shapes of operands are different (" << shape1
        << " vs " << shape2 << ")";
    throw std::runtime_error(oss.str());
  }
};

template <typename OP, int exptype, typename EType1, typename EType2,
          int exptype2, typename DType>
struct BinaryMapExp<
    OP, exptype, EType1, ExprType::kSparse, EType2, exptype2,
    DType> : public Exp<BinaryMapExp<OP, exptype, EType1, ExprType::kSparse,
                                     EType2, exptype2, DType>,
                        DType, exptype> {
  /// \brief  left operand
  const EType1 &lhs;
  /// \brief  right operand
  const EType2 &rhs;
  /*! \brief constructor */
  BinaryMapExp(const Exp<EType1, DType, ExprType::kSparse> &_lhs,
               const Exp<EType2, DType, exptype2> &_rhs)
      : lhs(_lhs.self()), rhs(_rhs.self()) {}

  /// accessing elements
  inline index_t index(size_t idx) const { return lhs.index(idx); }

  inline DType value(size_t idx) const {
    return OP::map(lhs.value(idx), rhs[lhs.index(idx)]);
  }

  inline Shape<2> shape() const { return lhs.shape(); }
};

template <typename OP, int exptype, typename EType1, int exptype1,
          typename EType2, typename DType>
struct BinaryMapExp<
    OP, exptype, EType1, exptype1, EType2, ExprType::kSparse,
    DType> : public Exp<BinaryMapExp<OP, exptype, EType1, exptype1, EType2,
                                     ExprType::kSparse, DType>,
                        DType, exptype> {
  /// \brief  left operand
  const EType1 &lhs;
  /// \brief  right operand
  const EType2 &rhs;
  /*! \brief constructor */
  BinaryMapExp(const Exp<EType1, DType, exptype1> &_lhs,
               const Exp<EType2, DType, ExprType::kSparse> &_rhs)
      : lhs(_lhs.self()), rhs(_rhs.self()) {}

  /// accessing elements
  inline index_t index(size_t idx) const { return rhs.index(idx); }

  inline DType value(size_t idx) const {
    return OP::map(lhs[rhs.index(idx)], rhs.value(idx));
  }

  inline Shape<2> shape() const { return rhs.shape(); }
};

/// \brief  make a binary expression
template <typename OP, typename EType1, typename EType2, typename DType,
          int exptype1, int exptype2, typename = typename std::enable_if<
                                          exptype1 != ExprType::kSparse ||
                                          exptype2 != ExprType::kSparse>::type>
inline BinaryMapExp<OP, (exptype1 | exptype2), EType1, exptype1, EType2,
                    exptype2, DType>
MakeExp(const Exp<EType1, DType, exptype1> &lhs,
        const Exp<EType2, DType, exptype2> &rhs) {
  // static_assert(exptype1 == ExprType::kSparse && exptype2 ==
  // ExprType::kSparse,
  //	"sparse with sparse operation is not suported yet");
  return BinaryMapExp<OP, (exptype1 | exptype2), EType1, exptype1, EType2,
                      exptype2, DType>(lhs.self(), rhs.self());
}

/// arithmetic operator expressions
#define ArithmeticBinaryMapExpTpl(opx, opr)                                    \
  template <typename EType1, typename EType2, typename DType, int exptype1,    \
            int exptype2>                                                      \
  inline BinaryMapExp<op::opr, (exptype1 | exptype2), EType1, exptype1,        \
                      EType2, exptype2, DType>                                 \
  operator opx(const Exp<EType1, DType, exptype1> &lhs,                        \
               const Exp<EType2, DType, exptype2> &rhs) {                      \
    return MakeExp<op::opr>(lhs, rhs);                                         \
  }                                                                            \
                                                                               \
  template <typename EType, int exptype>                                       \
  inline BinaryMapExp<op::opr, (ExprType::kValue | exptype), ScalarExp<int>,   \
                      ExprType::kValue, EType, exptype, int>                   \
  operator opx(const ScalarExp<int> &lhs,                                      \
               const Exp<EType, int, exptype> &rhs) {                          \
    return MakeExp<op::opr>(lhs, rhs);                                         \
  }                                                                            \
                                                                               \
  template <typename EType, int exptype>                                       \
  inline BinaryMapExp<op::opr, (exptype | ExprType::kValue), EType, exptype,   \
                      ScalarExp<int>, ExprType::kValue, int>                   \
  operator opx(const Exp<EType, int, exptype> &lhs,                            \
               const ScalarExp<int> &rhs) {                                    \
    return MakeExp<op::opr>(lhs, rhs);                                         \
  }                                                                            \
                                                                               \
  template <typename EType, int exptype>                                       \
  inline BinaryMapExp<op::opr, (ExprType::kValue | exptype), ScalarExp<float>, \
                      ExprType::kValue, EType, exptype, float>                 \
  operator opx(const ScalarExp<float> &lhs,                                    \
               const Exp<EType, float, exptype> &rhs) {                        \
    return MakeExp<op::opr>(lhs, rhs);                                         \
  }                                                                            \
                                                                               \
  template <typename EType, int exptype>                                       \
  inline BinaryMapExp<op::opr, (exptype | ExprType::kValue), EType, exptype,   \
                      ScalarExp<float>, ExprType::kValue, float>               \
  operator opx(const Exp<EType, float, exptype> &lhs,                          \
               const ScalarExp<float> &rhs) {                                  \
    return MakeExp<op::opr>(lhs, rhs);                                         \
  }                                                                            \
                                                                               \
  template <typename EType, int exptype>                                       \
  inline BinaryMapExp<op::opr, (ExprType::kValue | exptype),                   \
                      ScalarExp<double>, ExprType::kValue, EType, exptype,     \
                      double>                                                  \
  operator opx(const ScalarExp<double> &lhs,                                   \
               const Exp<EType, double, exptype> &rhs) {                       \
    return MakeExp<op::opr>(lhs, rhs);                                         \
  }                                                                            \
                                                                               \
  template <typename EType, int exptype>                                       \
  inline BinaryMapExp<op::opr, (exptype | ExprType::kValue), EType, exptype,   \
                      ScalarExp<double>, ExprType::kValue, double>             \
  operator opx(const Exp<EType, double, exptype> &lhs,                         \
               const ScalarExp<double> &rhs) {                                 \
    return MakeExp<op::opr>(lhs, rhs);                                         \
  }

ArithmeticBinaryMapExpTpl(+, plus);
ArithmeticBinaryMapExpTpl(-, minus);
ArithmeticBinaryMapExpTpl(*, mul);
ArithmeticBinaryMapExpTpl(/, div);

// truncate operations
template <typename EType1, typename EType2, typename DType, int exptype1,
          int exptype2>
inline BinaryMapExp<op::trunc, (exptype1 | exptype2), EType1, exptype1, EType2,
                    exptype2, DType>
truncate(const Exp<EType1, DType, exptype1> &lhs,
         const Exp<EType2, DType, exptype2> &rhs) {
  return MakeExp<op::trunc>(lhs, rhs);
}

template <typename EType, int exptype>
inline BinaryMapExp<op::trunc, (exptype | ExprType::kValue), EType, exptype,
                    ScalarExp<float>, ExprType::kValue, float>
truncate(const Exp<EType, float, exptype> &lhs, const ScalarExp<float> &rhs) {
  return MakeExp<op::trunc>(lhs, rhs);
}

template <typename EType, int exptype>
inline BinaryMapExp<op::trunc, (ExprType::kValue | exptype), ScalarExp<float>,
                    ExprType::kValue, EType, exptype, float>
truncate(const ScalarExp<float> &lhs, const Exp<EType, float, exptype> &rhs) {
  return MakeExp<op::trunc>(lhs, rhs);
}

template <typename EType, int exptype>
inline BinaryMapExp<op::trunc, (exptype | ExprType::kValue), EType, exptype,
                    ScalarExp<double>, ExprType::kValue, double>
truncate(const Exp<EType, double, exptype> &lhs, const ScalarExp<double> &rhs) {
  return MakeExp<op::trunc>(lhs, rhs);
}

template <typename EType, int exptype>
inline BinaryMapExp<op::trunc, (ExprType::kValue | exptype), ScalarExp<double>,
                    ExprType::kValue, EType, exptype, double>
truncate(const ScalarExp<double> &lhs, const Exp<EType, double, exptype> &rhs) {
  return MakeExp<op::trunc>(lhs, rhs);
}

inline float truncate(float lhs, float rhs) {
  return op::trunc::map<float>(lhs, rhs);
}

inline double truncate(double lhs, double rhs) {
  return op::trunc::map<double>(lhs, rhs);
}

template <typename EType1, typename EType2, typename DType>
inline BinaryMapExp<op::left, (ExprType::kDense | ExprType::kSparse), EType1,
                    ExprType::kDense, EType2, ExprType::kSparse, DType>
slice(const Exp<EType1, DType, ExprType::kDense> &lhs,
      const Exp<EType2, DType, ExprType::kSparse> &rhs) {
  return MakeExp<op::left>(lhs, rhs);
}

/***dot operations***/
template <typename OP, typename EType1, typename EType2, typename DType,
          int exptype1, int exptype2>
inline DType dot(const Exp<EType1, DType, exptype1> &lhs,
                 const Exp<EType2, DType, exptype2> &rhs);

template <typename EType1, typename EType2, typename DType, int exptype1,
          int exptype2>
inline DType dotplus(const Exp<EType1, DType, exptype1> &lhs,
                     const Exp<EType2, DType, exptype2> &rhs) {
  return dot<op::plus>(lhs, rhs);
}

template <typename EType1, typename EType2, typename DType, int exptype1,
          int exptype2>
inline DType dotminus(const Exp<EType1, DType, exptype1> &lhs,
                      const Exp<EType2, DType, exptype2> &rhs) {
  return dot<op::minus>(lhs, rhs);
}

template <typename EType1, typename EType2, typename DType, int exptype1,
          int exptype2>
inline DType dotmul(const Exp<EType1, DType, exptype1> &lhs,
                    const Exp<EType2, DType, exptype2> &rhs) {
  return dot<op::mul>(lhs, rhs);
}

template <typename EType1, typename EType2, typename DType, int exptype1,
          int exptype2>
inline DType dotdiv(const Exp<EType1, DType, exptype1> &lhs,
                    const Exp<EType2, DType, exptype2> &rhs) {
  return dot<op::div>(lhs, rhs);
}

//---------------
// UnaryMapExp
// --------------

/// \brief  unary map expression op(src)
///
/// \tparam OP operator
/// \tparam EType expression type
/// \tparam DType data element type
template <typename OP, typename EType, typename DType, int exptype>
struct UnaryMapExp
    : public Exp<UnaryMapExp<OP, EType, DType, exptype>, DType, exptype> {
  /*! \brief source expression */
  const EType exp;
  /*! \brief constructor */
  explicit UnaryMapExp(const EType &src) : exp(src) {}

  /// accessing elements
  inline DType operator()(size_t x, size_t y) const {
    return OP::map(exp(x, y));
  }
  inline DType operator[](size_t idx) const { return OP::map(exp[idx]); }

  inline index_t index(size_t idx) const { return exp.index(idx); }

  inline DType value(size_t idx) const { return OP::map(exp.value(idx)); }

  inline Shape<2> shape() const { return exp.shape(); }
};

/*! \brief make expression */
template <typename OP, typename EType, typename DType, int exptype>
inline UnaryMapExp<OP, EType, DType, exptype> MakeExp(
    const Exp<EType, DType, exptype> &src) {
  return UnaryMapExp<OP, EType, DType, exptype>(src.self());
}

template <typename EType, typename DType, int exptype>
inline UnaryMapExp<op::sqrt, EType, DType, exptype> Sqrt(
    const Exp<EType, DType, exptype> &src) {
  return UnaryMapExp<op::sqrt, EType, DType, exptype>(src.self());
}

template <typename EType, typename DType, int exptype>
inline UnaryMapExp<op::abs, EType, DType, exptype> L1(
    const Exp<EType, DType, exptype> &src) {
  return UnaryMapExp<op::abs, EType, DType, exptype>(src.self());
}

template <typename EType, typename DType, int exptype>
inline UnaryMapExp<op::square, EType, DType, exptype> L2(
    const Exp<EType, DType, exptype> &src) {
  return UnaryMapExp<op::square, EType, DType, exptype>(src.self());
}

}  // namespace expr
}  // namespace math
}  // namespace sol
#endif
