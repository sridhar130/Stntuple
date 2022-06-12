///////////////////////////////////////////////////////////////////////////////
#ifndef __Stntuple_stat_upper_limit__
#define __Stntuple_stat_upper_limit__

#include "TNamed.h"
#include "TH1D.h"
#include "TH2D.h"
#include "THStack.h"
#include "TRandom3.h"
#include "TGraph.h"

namespace stntuple {
//-----------------------------------------------------------------------------
class upper_limit: public TNamed {
public:

  enum {
    MaxNx =   50
  };

  struct Belt_t {
    double fSMin;
    double fSMax;
    double fDy;
    double fSign[MaxNx][2];      // fBelt.Sign[ix][0] = SMin[ix], fBelt.fSign[ix][1] = SMax[ix]
    int    fFillColor;
    int    fFillStyle;
  } fBelt;

  struct Debug_t {
    int    fConstructBelt;
    int    fUpperLimit;
    int    fTestCoverage;
    double fMuMin;
    double fMuMax;
  } fDebug;
  
  struct Hist_t {
    TH1D*    fProb;
    TH1D*    fLh;
    TH1D*    fInterval;
    THStack* fBelt;
    TH1D*    fBeltLo;
    TH1D*    fBeltZr;
    TH1D*    fBeltHi;
    TH1D*    fSign[2];
    TGraph*  fCoverage;
  };

  Hist_t fHist;

  int    fType;                      // default:0 if=1, try to force monotonic left edge
  double fCL;
  double fSump;

  double fMean;
  double fMuB;
  double fMuS;

  int    fNObs;
  
  int    fIxMin;
  int    fIxMax;

  long int fNExp;

  TRandom3 fRn;

  int    fIPMax;                    // index corresponding max(fProb);
  double fProb     [MaxNx];
  double fCProb    [MaxNx];
  double fFactorial[MaxNx];

  void SetNExp(long int NExp) { fNExp = NExp; }

  upper_limit(const char* Name, double CL = -1, int TYpe = 0);

                                        // 'N' - number of measured events
  
  int init_poisson_dist(double MuB, double MuS, int NObs = -1);
  
                                        // in presence of background, mu = mus+mub
  
  int construct_interval(double MuB, double MuS, int NObs = -1);
  int construct_belt    (double MuB, double SMin, double SMax, int NPoints, int NObs = -1);

  int  make_prob_hist();
  void make_belt_hist();

  int test_coverage(double MuB, double SMin, double SMax, int NPoints);

  ClassDef(upper_limit,0)
};

}
#endif