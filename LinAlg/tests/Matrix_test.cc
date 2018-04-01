#include "gtest/gtest.h"
#include "LinAlg/Vector.hpp"
#include "LinAlg/VectorView.hpp"
#include "LinAlg/Matrix.hpp"
#include "LinAlg/SpdMatrix.hpp"
#include "distributions.hpp"
#include "cpputil/math_utils.hpp"

#include "test_utils/test_utils.hpp"
#include <fstream>

namespace {
  using namespace BOOM;
  using std::endl;
  using std::cout;
  
  class MatrixTest : public ::testing::Test {
   protected:
    MatrixTest() {
      GlobalRng::rng.seed(8675309);
    }
  };

  TEST_F(MatrixTest, Constructors) {
    Matrix empty;
    EXPECT_EQ(0, empty.nrow());
    EXPECT_EQ(0, empty.ncol());

    Matrix single(1, 1);
    EXPECT_EQ(1, single.nrow());
    EXPECT_EQ(1, single.ncol());
    EXPECT_DOUBLE_EQ(0.0, single(0, 0));
  }

  TEST_F(MatrixTest, Multiplication) {
    Matrix M(3, 4);
    M.randomize();

    Vector v(4);
    v.randomize();
    Vector product = M * v;
    EXPECT_EQ(3, product.size());
    EXPECT_DOUBLE_EQ(M.row(0).dot(v), product[0]);
    EXPECT_DOUBLE_EQ(M.row(1).dot(v), product[1]);
    EXPECT_DOUBLE_EQ(M.row(2).dot(v), product[2]);

    VectorView view(v);
    product = M * view;
    EXPECT_EQ(3, product.size());
    EXPECT_DOUBLE_EQ(M.row(0).dot(v), product[0]);
    EXPECT_DOUBLE_EQ(M.row(1).dot(v), product[1]);
    EXPECT_DOUBLE_EQ(M.row(2).dot(v), product[2]);

    product = M * ConstVectorView(v);
    EXPECT_EQ(3, product.size());
    EXPECT_DOUBLE_EQ(M.row(0).dot(v), product[0]);
    EXPECT_DOUBLE_EQ(M.row(1).dot(v), product[1]);
    EXPECT_DOUBLE_EQ(M.row(2).dot(v), product[2]);

    Vector v3(3);
    v3.randomize();
    product = M.Tmult(v3);
    EXPECT_EQ(product.size(), 4);
    EXPECT_EQ(product[0], M.col(0).dot(v3));
    EXPECT_EQ(product[1], M.col(1).dot(v3));
    EXPECT_EQ(product[2], M.col(2).dot(v3));
    EXPECT_EQ(product[3], M.col(3).dot(v3));

    VectorView v3_view(v3);
    EXPECT_TRUE(VectorEquals(product, M.Tmult(v3_view)));
    EXPECT_TRUE(VectorEquals(product, M.Tmult(ConstVectorView(v3))));
    
    Matrix M2(4, 4);
    M2.randomize();

    Matrix MM2 = M * M2;
    EXPECT_EQ(3, MM2.nrow());
    EXPECT_EQ(4, MM2.ncol());
    EXPECT_DOUBLE_EQ(MM2(0, 0), M.row(0).dot(M2.col(0)));
    EXPECT_DOUBLE_EQ(MM2(0, 1), M.row(0).dot(M2.col(1)));
    EXPECT_DOUBLE_EQ(MM2(0, 2), M.row(0).dot(M2.col(2)));
    EXPECT_DOUBLE_EQ(MM2(0, 3), M.row(0).dot(M2.col(3)));
    EXPECT_DOUBLE_EQ(MM2(1, 0), M.row(1).dot(M2.col(0)));
    EXPECT_DOUBLE_EQ(MM2(1, 1), M.row(1).dot(M2.col(1)));
    EXPECT_DOUBLE_EQ(MM2(1, 2), M.row(1).dot(M2.col(2)));
    EXPECT_DOUBLE_EQ(MM2(1, 3), M.row(1).dot(M2.col(3)));
    EXPECT_DOUBLE_EQ(MM2(2, 0), M.row(2).dot(M2.col(0)));
    EXPECT_DOUBLE_EQ(MM2(2, 1), M.row(2).dot(M2.col(1)));
    EXPECT_DOUBLE_EQ(MM2(2, 2), M.row(2).dot(M2.col(2)));
    EXPECT_DOUBLE_EQ(MM2(2, 3), M.row(2).dot(M2.col(3)));

    SpdMatrix V(4);
    V.randomize();
    Matrix MV = M * V;
    EXPECT_EQ(3, MV.nrow());
    EXPECT_EQ(4, MV.ncol());
    EXPECT_DOUBLE_EQ(MV(0, 0), M.row(0).dot(V.col(0)));
    EXPECT_DOUBLE_EQ(MV(0, 1), M.row(0).dot(V.col(1)));
    EXPECT_DOUBLE_EQ(MV(0, 2), M.row(0).dot(V.col(2)));
    EXPECT_DOUBLE_EQ(MV(0, 3), M.row(0).dot(V.col(3)));
    EXPECT_DOUBLE_EQ(MV(1, 0), M.row(1).dot(V.col(0)));
    EXPECT_DOUBLE_EQ(MV(1, 1), M.row(1).dot(V.col(1)));
    EXPECT_DOUBLE_EQ(MV(1, 2), M.row(1).dot(V.col(2)));
    EXPECT_DOUBLE_EQ(MV(1, 3), M.row(1).dot(V.col(3)));
    EXPECT_DOUBLE_EQ(MV(2, 0), M.row(2).dot(V.col(0)));
    EXPECT_DOUBLE_EQ(MV(2, 1), M.row(2).dot(V.col(1)));
    EXPECT_DOUBLE_EQ(MV(2, 2), M.row(2).dot(V.col(2)));
    EXPECT_DOUBLE_EQ(MV(2, 3), M.row(2).dot(V.col(3)));

    EXPECT_TRUE(MatrixEquals(MV, M.multT(V)));

    EXPECT_TRUE(MatrixEquals(M.inner(), M.t() * M));
    EXPECT_TRUE(MatrixEquals(M.outer(), M * M.t()));
  }

  TEST_F(MatrixTest, Inv) {
    Matrix M(4, 4);
    M.randomize();

    Matrix Minv = M.inv();
    SpdMatrix I(4, 1.0);

    EXPECT_TRUE(MatrixEquals(M * Minv, I))
        << "M = " << endl << M << endl
        << "Minv = " << endl << Minv << endl
        << "M * Minv = " << endl
        << M * Minv << endl;

    Matrix M_copy(M);
    EXPECT_TRUE(MatrixEquals(M, M_copy))
        << "M = " << endl << M << endl
        << "M_copy = " << endl << M_copy;
  }

  TEST_F(MatrixTest, Solve) {
    Matrix M(4, 4);
    M.randomize();

    Vector v(4);
    v.randomize();

    Vector x = M.solve(v);
    EXPECT_TRUE(VectorEquals(M * x, v));

    Matrix M2(4, 6);
    M2.randomize();
    Matrix X = M.solve(M2);
    EXPECT_TRUE(MatrixEquals(M * X, M2));
  }
  
  TEST_F(MatrixTest, AddOuter) {
    Matrix M(4, 4);
    M.randomize();
    Matrix original_matrix = M;

    Vector v(4);
    v.randomize();

    EXPECT_TRUE(MatrixEquals(
        M.add_outer(v, v, 1.7),
        original_matrix + 1.7 * v.outer()));

    cout << "Checking VectorView" << endl;
    M = original_matrix;
    VectorView view(v);
    EXPECT_TRUE(MatrixEquals(
        M.add_outer(view, view, 1.4),
        original_matrix + 1.4 * v.outer()));

    cout << "Checking ConstVectorView" << endl;
    M = original_matrix;
    const VectorView const_view(v);
    EXPECT_TRUE(MatrixEquals(
        M.add_outer(const_view, const_view, 1.9),
        original_matrix + 1.9 * v.outer()));

    cout << "Checking ConstVectorView" << endl;
    M = original_matrix;
    EXPECT_TRUE(MatrixEquals(
        M.add_outer(const_view, const_view, 1.9),
        original_matrix + 1.9 * v.outer()));

  }

  // TEST_F(SpdMatrixTest, AddInner) {
  //   SpdMatrix Sigma(4);
  //   Sigma.randomize();

  //   Matrix X(3, 4);
  //   X.randomize();
    
  //   SpdMatrix original_sigma = Sigma;
  //   EXPECT_TRUE(MatrixEquals(
  //       Sigma.add_inner(X, 1.1),
  //       original_sigma + X.t() * X * 1.1))
  //       << "Sigma = " << endl << Sigma << endl
  //       << "Direct = " << endl
  //       << original_sigma + X.t() * X * 1.1
  //       << endl;

  //   Vector weights(X.nrow());
  //   weights.randomize();
  //   Sigma = original_sigma;
  //   Matrix W(weights.size(), weights.size(), 0.0);
  //   W.diag() = weights;
  //   EXPECT_TRUE(MatrixEquals(
  //       Sigma.add_inner(X, weights),
  //       original_sigma + X.t() * W * X));

  //   Matrix Y(3, 4);
  //   Y.randomize();
  //   Sigma = original_sigma;
  //   EXPECT_TRUE(MatrixEquals(
  //       Sigma.add_inner2(X, Y, .3),
  //       original_sigma + .3 * (X.t() * Y + Y.t() * X)));

  // }

  TEST_F(MatrixTest, Operators) {
    Matrix M(3, 3);
    M.randomize();
    Matrix original_M = M;

    M *= 2.0;
    EXPECT_DOUBLE_EQ(M(1, 2), original_M(1, 2) * 2.0);

    M = original_M;
    ////////////////////////////////////
  }

  TEST_F(MatrixTest, Norms) {
    Matrix M(2, 2);
    M.randomize();

    EXPECT_DOUBLE_EQ(M.abs_norm(),
                     fabs(M(0, 0)) + fabs(M(0, 1)) + fabs(M(1, 0)) + fabs(M(1, 1)));

    EXPECT_DOUBLE_EQ(M.sumsq(), sum(el_mult(M, M)));
  }

  TEST_F(MatrixTest, LowerTriangular) {

    Matrix L(3, 3);
    L.randomize();
    L(0, 1) = L(0, 2) = L(1, 2) = 0.0;

    Vector v(3);
    EXPECT_TRUE(VectorEquals(Lmult(L, v), L * v));
    EXPECT_TRUE(VectorEquals(Lsolve(L, v), L.inv() * v));
    Vector original_v = v;
    EXPECT_TRUE(VectorEquals(LTsolve_inplace(L, v), L.t().inv() * original_v));
    v = original_v;
    EXPECT_TRUE(VectorEquals(Lsolve_inplace(L, v), L.inv() * original_v));
    v = original_v;

    Matrix B(3, 3);
    B.randomize();
    EXPECT_TRUE(MatrixEquals(Lsolve(L, B), L.inv() * B));
    Matrix original_B = B;
    EXPECT_TRUE(MatrixEquals(Lsolve_inplace(L, B), L.inv() * original_B));
    B = original_B;
    EXPECT_TRUE(MatrixEquals(LTsolve_inplace(L, B), L.t().inv() * original_B));
    B = original_B;
    EXPECT_TRUE(MatrixEquals(Linv(L), L.inv()));

    Matrix U = L.t();
    EXPECT_TRUE(VectorEquals(Umult(U, v), U * v));
    EXPECT_TRUE(MatrixEquals(Umult(U, B), U * B));
    EXPECT_TRUE(VectorEquals(Usolve(U, v), U.inv() * v));
    EXPECT_TRUE(VectorEquals(Usolve_inplace(U, v), U.inv() * original_v));
    v = original_v;

    EXPECT_TRUE(MatrixEquals(Usolve(U, B), U.inv() * B));
    EXPECT_TRUE(MatrixEquals(Usolve_inplace(U, B), U.inv() * original_B));
    B = original_B;
    EXPECT_TRUE(MatrixEquals(Uinv(U), U.inv()));
  }
  
}  // namespace