// Copyright 2018 Google LLC. All Rights Reserved.
/*
  Copyright (C) 2006 Steven L. Scott

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#include "Models/Glm/MultivariateRegression.hpp"
#include "Models/SufstatAbstractCombineImpl.hpp"
#include "LinAlg/Cholesky.hpp"
#include "cpputil/Constants.hpp"
#include "distributions.hpp"

namespace BOOM {

  //======================================================================

  uint MvRegSuf::xdim() const { return xtx().nrow(); }
  uint MvRegSuf::ydim() const { return yty().nrow(); }

  //======================================================================
  typedef NeMvRegSuf NS;

  NS::NeMvRegSuf(uint xdim, uint ydim)
      : yty_(ydim), xtx_(xdim), xty_(xdim, ydim), n_(0) {}

  NS::NeMvRegSuf(const Matrix &X, const Matrix &Y)
      : yty_(Y.ncol()), xtx_(X.ncol()), xty_(X.ncol(), Y.ncol()), n_(0) {
    QR qr(X);
    Matrix R = qr.getR();
    xtx_.add_inner(R);

    QR qry(Y);
    yty_.add_inner(qry.getR());

    xty_ = qr.getQ().Tmult(Y);
    xty_ = R.Tmult(xty_);
  }

  NS::NeMvRegSuf(const NS &rhs)
      : Sufstat(rhs),
        MvRegSuf(rhs),
        SufTraits(rhs),
        yty_(rhs.yty_),
        xtx_(rhs.xtx_),
        xty_(rhs.xty_),
        n_(rhs.n_) {}

  NS *NS::clone() const { return new NS(*this); }

  void NS::Update(const MvRegData &d) {
    const Vector &y(d.y());
    const Vector &x(d.x());
    double w = d.weight();
    update_raw_data(y, x, w);
  }

  void NS::update_raw_data(const Vector &y, const Vector &x, double w) {
    ++n_;
    sumw_ += w;
    xtx_.add_outer(x, w);
    xty_.add_outer(x, y, w);
    yty_.add_outer(y, w);
  }

  Matrix NS::beta_hat() const { return xtx_.solve(xty_); }
  Matrix NS::conditional_beta_hat(const SelectorMatrix &included) const {
    Matrix ans(xdim(), ydim());
    std::map<Selector, Cholesky> chol_map;
    for (int i = 0; i < ydim(); ++i) {
      const Selector &inc(included.col(i));
      auto it = chol_map.find(inc);
      if (it == chol_map.end()) {
        chol_map[it->first] = Cholesky(inc.select(xtx()));
        it = chol_map.find(inc);
      } 
      ans.col(i) = inc.expand(it->second.solve(inc.select(xty_.col(i))));
    }
    return ans;
  }
  
  SpdMatrix NS::SSE(const Matrix &B) const {
    SpdMatrix ans = yty();
    ans.add_inner2(B, xty(), -1);
    ans += sandwich(B.transpose(), xtx());
    return ans;
  }

  void NeMvRegSuf::clear() {
    yty_ = 0;
    xtx_ = 0;
    xty_ = 0;
    n_ = 0;
  }

  const SpdMatrix &NS::yty() const { return yty_; }
  const SpdMatrix &NS::xtx() const { return xtx_; }
  const Matrix &NS::xty() const { return xty_; }
  double NS::n() const { return n_; }
  double NS::sumw() const { return sumw_; }

  void NS::combine(const Ptr<MvRegSuf> &sp) {
    Ptr<NS> s(sp.dcast<NS>());
    xty_ += s->xty_;
    xtx_ += s->xtx_;
    yty_ += s->yty_;
    sumw_ += s->sumw_;
    n_ += s->n_;
  }

  void NS::combine(const MvRegSuf &sp) {
    const NS &s(dynamic_cast<const NS &>(sp));
    xty_ += s.xty_;
    xtx_ += s.xtx_;
    yty_ += s.yty_;
    sumw_ += s.sumw_;
    n_ += s.n_;
  }

  Vector NS::vectorize(bool minimal) const {
    Vector ans = yty_.vectorize(minimal);
    ans.concat(xtx_.vectorize(minimal));
    Vector tmp(xty_.begin(), xty_.end());
    ans.concat(tmp);
    ans.push_back(sumw_);
    ans.push_back(n_);
    return ans;
  }

  NeMvRegSuf *NS::abstract_combine(Sufstat *s) {
    return abstract_combine_impl(this, s);
  }

  Vector::const_iterator NS::unvectorize(Vector::const_iterator &v,
                                         bool minimal) {
    yty_.unvectorize(v, minimal);
    xtx_.unvectorize(v, minimal);
    uint xdim = xtx_.nrow();
    uint ydim = yty_.nrow();
    Matrix tmp(v, v + xdim * ydim, xdim, ydim);
    v += xdim * ydim;
    sumw_ = *v;
    ++v;
    n_ = *v;
    ++v;
    return v;
  }

  Vector::const_iterator NS::unvectorize(const Vector &v, bool minimal) {
    Vector::const_iterator it = v.begin();
    return unvectorize(it, minimal);
  }

  ostream &NS::print(ostream &out) const {
    out << "yty_ = " << yty_ << endl
        << "xty_ = " << xty_ << endl
        << "xtx_ = " << endl
        << xtx_;
    return out;
  }

  //======================================================================

  typedef QrMvRegSuf QS;
  QS::QrMvRegSuf(const Matrix &X, const Matrix &Y,
                 MultivariateRegressionModel *Owner)
      : qr(X),
        owner(Owner),
        current(false),
        yty_(Y.ncol()),
        xtx_(X.ncol()),
        xty_(X.ncol(), Y.ncol()),
        n_(0) {
    refresh(X, Y);
    current = true;
  }

  QS::QrMvRegSuf(const Matrix &X, const Matrix &Y, const Vector &W,
                 MultivariateRegressionModel *Owner)
      : qr(X),
        owner(Owner),
        current(false),
        yty_(Y.ncol()),
        xtx_(X.ncol()),
        xty_(X.ncol(), Y.ncol()),
        n_(0) {
    refresh(X, Y, W);
    current = true;
  }

  void QS::combine(const Ptr<MvRegSuf> &) {
    report_error("cannot combine QrMvRegSuf");
  }

  void QS::combine(const MvRegSuf &) {
    report_error("cannot combine QrMvRegSuf");
  }

  QrMvRegSuf *QS::abstract_combine(Sufstat *s) {
    return abstract_combine_impl(this, s);
  }

  Vector QS::vectorize(bool) const {
    report_error("cannot vectorize QrMvRegSuf");
    return Vector(1, 0.0);
  }

  Vector::const_iterator QS::unvectorize(Vector::const_iterator &v, bool) {
    report_error("cannot unvectorize QrMvRegSuf");
    return v;
  }

  Vector::const_iterator QS::unvectorize(const Vector &v, bool minimal) {
    report_error("cannot unvectorize QrMvRegSuf");
    Vector::const_iterator it = v.begin();
    return unvectorize(it, minimal);
  }

  ostream &QS::print(ostream &out) const {
    out << "yty_ = " << yty_ << endl
        << "xty_ = " << xty_ << endl
        << "xtx_ = " << endl
        << xtx_;
    return out;
  }

  QS *QS::clone() const { return new QS(*this); }

  const SpdMatrix &QS::xtx() const {
    if (!current) refresh();
    return xtx_;
  }
  const SpdMatrix &QS::yty() const {
    if (!current) refresh();
    return yty_;
  }
  const Matrix &QS::xty() const {
    if (!current) refresh();
    return xty_;
  }
  double QS::n() const {
    if (!current) refresh();
    return n_;
  }
  double QS::sumw() const {
    if (!current) refresh();
    return sumw_;
  }

  void QS::refresh(const Matrix &X, const Matrix &Y) const {
    y_ = Y;
    qr.decompose(X);
    Matrix R(qr.getR());
    xtx_ = 0;
    xtx_.add_inner(R);

    QR qry(Y);
    xty_ = 0;
    yty_.add_inner(qry.getR());

    n_ = X.nrow();

    xty_ = qr.getQ().Tmult(Y);
    xty_ = R.Tmult(xty_);
    current = true;
  }

  void QS::refresh(const Matrix &X, const Matrix &Y, const Vector &w) const {
    y_ = Y;
    Matrix x_(X);
    uint nr = X.nrow();
    sumw_ = 0;
    for (uint i = 0; i < nr; ++i) {
      sumw_ += w[i];
      double rootw = sqrt(w[i]);
      y_.row(i) *= rootw;
      x_.row(i) *= rootw;
    }
    qr.decompose(x_);
    Matrix R(qr.getR());
    xtx_ = 0;
    xtx_.add_inner(R);

    QR qry(y_);
    xty_ = 0;
    yty_.add_inner(qry.getR());

    n_ = X.nrow();

    xty_ = qr.getQ().Tmult(y_);
    xty_ = R.Tmult(xty_);
    current = true;
  }

  void QS::refresh(const std::vector<Ptr<MvRegData> > &dv) const {
    Ptr<MvRegData> dp = dv[0];
    uint n = dv.size();
    const Vector &x0(dp->x());
    const Vector &y0(dp->y());

    uint nx = x0.size();
    uint ny = y0.size();
    sumw_ = 0;
    Matrix X(n, nx);
    Matrix Y(n, ny);
    for (uint i = 0; i < n; ++i) {
      dp = dv[i];
      double w = dp->weight();
      sumw_ += w;
      if (w == 1.0) {
        X.set_row(i, dp->x());
        Y.set_row(i, dp->y());
      } else {
        double rootw = sqrt(w);
        X.set_row(i, dp->x() * rootw);
        Y.set_row(i, dp->y() * rootw);
      }
    }
    refresh(X, Y);
  }

  void QS::refresh() const { refresh(owner->dat()); }

  void QS::clear() {
    current = false;
    n_ = 0;
    xtx_ = 0;
    xty_ = 0;
    yty_ = 0;
  }

  void QS::Update(const MvRegData &) { current = false; }

  Matrix QS::beta_hat() const {
    Matrix ans = qr.getQ().Tmult(y_);
    ans = qr.Rsolve(ans);
    return ans;
  }

  Matrix QS::conditional_beta_hat(const SelectorMatrix &included) const {
    report_error("Not yet implemented.");
    return Matrix(0, 0);
  }
  
  SpdMatrix QS::SSE(const Matrix &B) const {
    Matrix RB = qr.getR() * B;
    SpdMatrix ans = yty();
    ans.add_inner(RB);

    Matrix Qty = qr.getQ().Tmult(y_);
    Matrix tmp = RB.Tmult(Qty);
    ans.add_inner2(RB, Qty, -1.0);

    return ans;
  }
  //======================================================================

  namespace {
    using MvReg = MultivariateRegressionModel;
  }
  
  MvReg::MultivariateRegressionModel(uint xdim, uint ydim)
      : ParamPolicy(new MatrixGlmCoefs(xdim, ydim), new SpdParams(ydim)),
        DataPolicy(new NeMvRegSuf(xdim, ydim)),
        PriorPolicy(),
        LoglikeModel() {}

  MvReg::MultivariateRegressionModel(const Matrix &X, const Matrix &Y)
      : ParamPolicy(),
        DataPolicy(new QrMvRegSuf(X, Y, this)),
        PriorPolicy(),
        LoglikeModel() {
    uint nx = X.ncol();
    uint ny = Y.ncol();
    set_params(new MatrixGlmCoefs(nx, ny), new SpdParams(ny));
    mle();
  }

  MvReg::MultivariateRegressionModel(const Matrix &B, const SpdMatrix &V)
      : ParamPolicy(new MatrixGlmCoefs(B), new SpdParams(V)),
        DataPolicy(new NeMvRegSuf(B.nrow(), B.ncol())),
        PriorPolicy(),
        LoglikeModel() {}

  MvReg *MvReg::clone() const { return new MvReg(*this); }

  uint MvReg::xdim() const { return Beta().nrow(); }
  uint MvReg::ydim() const { return Beta().ncol(); }

  const Matrix &MvReg::Beta() const { return Beta_prm()->value(); }
  const SpdMatrix &MvReg::Sigma() const { return Sigma_prm()->var(); }
  const SpdMatrix &MvReg::Siginv() const { return Sigma_prm()->ivar(); }
  const Matrix &MvReg::residual_precision_cholesky() const {
    return Sigma_prm()->ivar_chol();
  }
  double MvReg::ldsi() const { return Sigma_prm()->ldsi(); }

  Ptr<MatrixGlmCoefs> MvReg::Beta_prm() { return prm1(); }
  const Ptr<MatrixGlmCoefs> MvReg::Beta_prm() const { return prm1(); }
  Ptr<SpdParams> MvReg::Sigma_prm() { return prm2(); }
  const Ptr<SpdParams> MvReg::Sigma_prm() const { return prm2(); }

  void MvReg::set_Beta(const Matrix &B) { Beta_prm()->set(B); }

  void MvReg::set_Sigma(const SpdMatrix &V) { Sigma_prm()->set_var(V); }

  void MvReg::set_Siginv(const SpdMatrix &iV) { Sigma_prm()->set_ivar(iV); }

  void MvReg::mle() {
    set_Beta(suf()->beta_hat());
    set_Sigma(suf()->SSE(Beta()) / suf()->n());
  }

  double MvReg::log_likelihood(const Matrix &Beta,
                               const SpdMatrix &Sigma) const {
    Cholesky Sigma_cholesky(Sigma);
    double qform = trace(suf()->SSE(Beta) * Sigma_cholesky.inv());
    double ldsi = -1 * Sigma_cholesky.logdet();
    double n = suf()->n();
    double normalizing_constant = -.5 * (n * ydim()) * Constants::log_2pi;
    return normalizing_constant + .5 * n * ldsi - .5 * qform;
  }

  // The likelihood is \prod root(2pi)^-d |siginv|^{n/2} exp{-1/2 * trace(qform)}
  double MvReg::log_likelihood_ivar(const Matrix &Beta,
                                    const SpdMatrix &Siginv) const {
    double qform = trace(suf()->SSE(Beta) * Siginv);
    double n = suf()->n();
    double normalizing_constant = -.5 * (n * ydim()) * Constants::log_2pi;
    return normalizing_constant + .5 * n * Siginv.logdet() - .5 * qform;
  }
        
  double MvReg::loglike(const Vector &beta_siginv) const {
    Matrix Beta(xdim(), ydim());
    Vector::const_iterator it = beta_siginv.cbegin();
    std::copy(it, it + Beta.size(), Beta.begin());
    it += Beta.size();
    SpdMatrix siginv(ydim());
    siginv.unvectorize(it, true);
    return log_likelihood_ivar(Beta, siginv);
  }

  double MvReg::log_likelihood() const {
    return log_likelihood_ivar(Beta(), Siginv());
  }
  
  double MvReg::pdf(const Ptr<Data> &dptr, bool logscale) const {
    Ptr<MvRegData> dp = DAT(dptr);
    Vector mu = predict(dp->x());
    return dmvn(dp->y(), mu, Siginv(), ldsi(), logscale);
  }

  Vector MvReg::predict(const Vector &x) const { return x * Beta(); }

  MvRegData *MvReg::simdat(RNG &rng) const {
    Vector x = simulate_fake_x(rng);
    return simdat(x, rng);
  }

  MvRegData *MvReg::simdat(const Vector &x, RNG &rng) const {
    Vector mu = predict(x);
    Vector y = rmvn_mt(rng, mu, Sigma());
    return new MvRegData(y, x);
  }

  Vector MvReg::simulate_fake_x(RNG &rng) const {
    uint p = xdim();
    Vector x(p, 1.0);
    for (uint i = 1; i < p; ++i) x[i] = rnorm_mt(rng);
    return x;
  }

}  // namespace BOOM
