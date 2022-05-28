///////////////////////////////////////////////////////////////////////////////
//
// fDebuglevel.fTestCoverage = 1: print all
// fDebuglevel.fTestCoverage = 2: print missed
///////////////////////////////////////////////////////////////////////////////
#include <algorithm>
#include "TMath.h"
#include "TCanvas.h"
#include "TROOT.h"

#include "Stntuple/stat/crow_gardner.hh"

ClassImp(stntuple::crow_gardner)

namespace stntuple {
//-----------------------------------------------------------------------------
crow_gardner::crow_gardner(const char* Name, double Mu, double CL, int Type) : TNamed(Name,Name) {

  double alpha = 1-TMath::Erf(5./sqrt(2));
  
  if (CL > 0) fCL = CL;
  else        fCL = 1-alpha/2; // always one-sided: 5.7330314e-07/2 = 2.8665157e-07

  fType = Type;

  fIxMin = MaxNx;
  fIxMax = -1;
  
  fHist.fProb       = nullptr;
  fHist.fInterval   = nullptr;
  fHist.fBelt       = nullptr;
  fHist.fBeltLo     = nullptr;
  fHist.fBeltHi     = nullptr;
  fHist.fCoverage   = nullptr;

  fDebug.fConstructBelt = 0;
  fDebug.fTestCoverage  = 0;
  fDebug.fUpperLimit    = 0;
                                        // no printout by default
  fDebug.fMuMin         = 1.;
  fDebug.fMuMax         = 0.;

  fNExp  = 100000;
  fNObs  = -1;

  fBelt.fFillColor      = kRed+2;
  fBelt.fFillStyle      = 3005;
//-----------------------------------------------------------------------------
// calculate factorials, do that only once
// assume MaxNx to be large enough, so having N! values up to MaxNx-1 included is enough
//-----------------------------------------------------------------------------
  fFactorial[0] = 1; for (int i=1; i<MaxNx; i++) { fFactorial[i] = fFactorial[i-1]*i; }

  init_poisson_dist(0,Mu,fNObs);
}

//-----------------------------------------------------------------------------
// mu = MuB+MuS
// 'NObs' is the number of observed events - it constrains the background fluctuations
// NObs<0 means no prior knowledge 
// array 'Prob' should have at least MaxNx elements
// declare NObs as double to be able to scan
// 'nature': given by MuB+Mus - sampled distribution
//-----------------------------------------------------------------------------
int crow_gardner::init_poisson_dist(double MuB, double MuS, int NObs) {
//-----------------------------------------------------------------------------
// the length of 'Prob' should be at least N
// CumProb[N]: probability, for a given Mean, to have rn <= N
//-----------------------------------------------------------------------------
  fMean  = MuB+MuS;
  fMuS   = MuS;
  fMuB   = MuB;
  
  if (NObs < 0) {
//-----------------------------------------------------------------------------
// standard Poisson distribution
//-----------------------------------------------------------------------------
    fProb[0] = TMath::Exp(-fMean);

    double pmax = fProb[0];
    fIPMax      = 0;

    for (int i=1; i<MaxNx; i++) {
      fProb[i] = fProb[i-1]*fMean/i;
      if (fProb[i] > pmax) {
	pmax   = fProb[i];
	fIPMax = i;
      }
    }
  }
  else {
//-----------------------------------------------------------------------------
// background probability constrained by the measurement of N events (Zech'1989)
//-----------------------------------------------------------------------------
    double pbn = 0; for (int k=0; k<=NObs; k++) { pbn += TMath::Exp(-MuB)*pow(MuB,k)/fFactorial[k]; }
    
    double pb[MaxNx];
    for (int i=0; i<MaxNx; i++) {
      if (i <= NObs) pb[i] = TMath::Exp(-MuB)*pow(MuB,i)/fFactorial[i]/pbn;
      else           pb[i] = 0;
    }
					// 'i' - bin in the constrained Poisson distribution
    double exp_mus = TMath::Exp(-MuS);

    double pmax = -1;
    fIPMax      = -1;
    for (int i=0; i<MaxNx; i++) {
					// experiment observed Nobs events, use pb[k]
      double pi = 0;
      for (int k=0; k<=i; k++) {
	double ps = exp_mus*pow(MuS,i-k)/fFactorial[i-k];
	pi        = pi + pb[k]*ps;
      }
      fProb[i] = pi;

      if (fProb[i] > pmax) {
	pmax   = fProb[i];
	fIPMax = i;
      }
    }
  }

  return 0;
}

//-----------------------------------------------------------------------------
// two parameters for uniformity of the interface
//-----------------------------------------------------------------------------
int crow_gardner::construct_interval(double MuB, double MuS, int NObs) {
  int rc(0);
					// find max bin
  init_poisson_dist(MuB,MuS,NObs);

  if (fMean > MaxNx-5) {
    printf("fMean = %12.5e is TOO MUCH. EXIT.\n",fMean);
    return -1;
  }
                                        // find bin with P(max), fMean = MuB+MuS
  int    imx  = fIPMax;
  double pmax = fProb[imx];
                                        // max bin found;
  double sp = pmax;
                                        // set both to the bin with max 
  int i1 = imx-1;
  int i2 = imx+1;

  int imin   = imx;
  int imax   = imx;

  if (sp < fCL) {
    while (1) {
    
      if ((i1 >= 0) and (i2 < MaxNx)) {
        if (fProb[i1] > fProb[i2]) {

          double sp1 = sp + fProb[i1];
          if (sp1 >= fCL) {
            sp   = sp1;
            imin = i1;
            break;
          }
          else {
            sp   = sp1;
            imin = i1;
            i1   = i1-1;
          }
        }
        else {
          // fProb[i2] is the largest
          double sp2 = sp + fProb[i2];
          if (sp2 >= fCL) {
            sp   = sp2;
            imax = i2;
            break;
          }
          else {
            // below the threshold
            imax = i2;
            i2   = i2+1;
            sp   = sp2;
          }
        }
      }
      else if (i1 < 0) {
        //-----------------------------------------------------------------------------
        // consider only upper part, look at i2
        //-----------------------------------------------------------------------------
        if (i2 < MaxNx) {
          // fProb[i2] is the largest
          sp   = sp + fProb[i2];
          imax = i2;
          if (sp >= fCL) {
            // done
            break;
          }
          else {
            // continue
            i2    = i2+1;
          }
        }
        else {
          // i2 = MaxNx
          printf("[crow_gardner::construct_interval] ERROR:  MuB:%10.3f MuS:%10.3f",MuB,MuS);
          printf(" sp = %12.5e , i2 =  %3i BREAK\n",sp,i2);
	  rc = -2;
          break;
        }
      }
      else if (i2 >= MaxNx) {
        //-----------------------------------------------------------------------------
        // consider only lower part , i1
        //-----------------------------------------------------------------------------
        if (i1 >= 0) {
          sp   = sp + fProb[i1];
          imin = i1;
          if (sp >= fCL) {
            // done
            break;
          }
          else {
            // continue
            i1 = i1-1;
          }
        }
        else {
          // i1 = 0
          printf("[crow_gardner::construct_interval] ERROR:  MuB:%10.3f MuS:%10.3f",MuB,MuS);
	  printf(" didnt converge, i1 = %3i, sp = %12.5e , BREAK\n",i1,sp);
	  rc = -3;
          break;
        }
      }
    }
  }

  if (rc < 0) {
    fIxMin = imin;
    fIxMax = imax;
    fSump  = sp;
    return rc;
  }
//-----------------------------------------------------------------------------
// see, if can move to the left
//-----------------------------------------------------------------------------
  while (imax < MaxNx) {
    double sp1 = sp + fProb[imax+1]-fProb[imin];
    if (sp1 > fCL) {
      sp   = sp1;
      imin = imin+1;
      imax = imax+1;
    }
    else {
      break;
    }
  }
//-----------------------------------------------------------------------------
// IMin and IMax - the lower and the higher bin of the interval, could be the same
//-----------------------------------------------------------------------------
  fIxMin = imin;
  fIxMax = imax;
  fSump  = sp;

  return 0;
}

//-----------------------------------------------------------------------------
// vary signal from SMin to SMax in NPoints, construct FC belt, fill belt histogram
// fBelt is the FC belt histogram
// avoid multiple useless re-initializations
//-----------------------------------------------------------------------------
int crow_gardner::construct_belt(double MuB, double SMin, double SMax, int NPoints, int NObs) {

  fMuB = MuB;
  
  fBelt.fSMin = SMin;
  fBelt.fSMax = SMax;
  fBelt.fDy   = (NPoints > 1) ? (SMax-SMin)/(NPoints-1) : 1;

  for (int ix=0; ix<MaxNx; ix++) {
    fBelt.fSign[ix][0] = 1.e6;
    fBelt.fSign[ix][1] = -1;
  }

  for (int iy=0; iy<NPoints; iy++) {
    double mus = SMin+iy*fBelt.fDy;

    int rc     = construct_interval(MuB,mus,NObs);
    if (rc == 0) {
      for (int ix=fIxMin; ix<=fIxMax; ix++) {
        if (mus > fBelt.fSign[ix][1]) fBelt.fSign[ix][1] = mus+fBelt.fDy/2;
        if (mus < fBelt.fSign[ix][0]) fBelt.fSign[ix][0] = mus-fBelt.fDy/2;

        if (fDebug.fConstructBelt == 2) {
          if ((mus >= fDebug.fMuMin) and (mus <= fDebug.fMuMax)) {
            printf("crow_gardner::construct_belt: iy = %5i ix:%3i mus=%8.4f MuB=%5.3f IxMin:%3i IxMax:%3i fSign[ix][0]:%8.4f fSign[ix][1]:%8.4f\n",
                   iy,ix,mus,MuB,fIxMin,fIxMax,fBelt.fSign[ix][0],fBelt.fSign[ix][0]);
          }
        }
      }
    }
    else {
      if (fDebug.fConstructBelt > 0) {
	printf("TFeldmanCousinsB::ConstructBelt: ERROR: mus=%10.3f MuB=%10.3f iy = %3i interval not defined\n",
	       mus,MuB,iy);
      }
    }
  }

  for (int ix=0; ix<MaxNx; ix++) {
    if (fBelt.fSign[ix][0] == 1.e6) fBelt.fSign[ix][0] = 0;
    if (fBelt.fSign[ix][0] <  0   ) fBelt.fSign[ix][0] = 0;
    if (fBelt.fSign[ix][1] <  0   ) fBelt.fSign[ix][1] = 0;
  }

  if (fDebug.fConstructBelt > 0) {
    for (int ix=0; ix<MaxNx; ix++) {
      printf("%32s %3i %12.5f %12.5f\n","",ix,fBelt.fSign[ix][0],fBelt.fSign[ix][1]);
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
int crow_gardner::make_prob_hist() {

  if (fHist.fProb    ) delete fHist.fProb;
  if (fHist.fInterval) delete fHist.fInterval;
  
  // fHist.fProb     = new  TH1D("h_prob","prob"    ,MaxNx,-0.5,MaxNx-.5);
  // fHist.fInterval = new  TH1D("h_intr","interval",MaxNx,-0.5,MaxNx-.5);

  TString title = Form("prob vs N, muB:%5.2f muS:%7.4f CL:%5.3f Nobs:%3i",fMuB,fMuS,fCL,fNObs);
  fHist.fProb     = new  TH1D(Form("h_prob_%s",GetName()),title.Data(),MaxNx,-0.5,MaxNx-0.5);

  title = Form("Interval muB:%5.2f muS:%7.4f CL:%5.3f Nobs:%3i",fMuB,fMuS,fCL,fNObs);
  fHist.fInterval = new  TH1D(Form("h_intr_%s",GetName()),title.Data(),MaxNx,-0.5,MaxNx-0.5);

  title = Form("log(Prob) muB:%5.2f muS:%7.4f CL:%5.3f Nobs:%3i",fMuB,fMuS,fCL,fNObs);
  fHist.fLh       = new  TH1D(Form("h_llh_%s" ,GetName()),"log(Prob)",5000,-10,40);
  
  fHist.fInterval->SetFillStyle(3004);
  fHist.fInterval->SetFillColor(kRed+2);
  fHist.fInterval->SetLineColor(kRed+2);

  for (int i=0; i<MaxNx; i++) {
    fHist.fProb->SetBinContent(i+1,fProb[i]);
    fHist.fProb->SetBinError  (i+1,0);

    if (i < fMean) fHist.fLh->Fill( log(fProb[i]),fProb[i]);
    else           fHist.fLh->Fill(-log(fProb[i]),fProb[i]);

    if ((i >= fIxMin) and (i<= fIxMax)) {
      fHist.fInterval->SetBinContent(i+1,fProb[i]);
      fHist.fInterval->SetBinError  (i+1,0);
    }
  }

  return 0;
}

//-----------------------------------------------------------------------------
void crow_gardner::make_belt_hist() {
  if (fHist.fBelt) {
    delete fHist.fBelt;
    delete fHist.fBeltLo;
    delete fHist.fBeltHi;
  }
  
  fHist.fBeltLo   = new TH1D(Form("h_belt_lo_%s",GetName()),
                             Form("Crow-Gardner belt MuB = %10.3f CL = %5.2f Nobs:%3i",fMuB,fCL,fNObs),
                             MaxNx,-0.5,MaxNx-0.5);

  fHist.fBeltHi   = new TH1D(Form("h_belt_hi_%s",GetName()),
                             Form("Crow-Gardner belt MuB = %10.3f CL = %5.2f Nobs:%3i",fMuB,fCL,fNObs),
                             MaxNx,-0.5,MaxNx-0.5);

  for (int ix=0; ix<MaxNx; ix++) {
    double dx = fBelt.fSign[ix][1]-fBelt.fSign[ix][0];
    if (fBelt.fSign[ix][1] > 0) {    
      fHist.fBeltLo->SetBinContent(ix+1,fBelt.fSign[ix][0]);
      fHist.fBeltHi->SetBinContent(ix+1,dx);
    }
  }
  
  fHist.fBeltLo->SetLineColor(fBelt.fFillColor);

  fHist.fBeltHi->SetFillStyle(fBelt.fFillStyle);
  fHist.fBeltHi->SetFillColor(fBelt.fFillColor);
  fHist.fBeltHi->SetLineColor(fBelt.fFillColor);

  fHist.fBelt = new THStack(Form("hs_%s",GetName()),fHist.fBeltHi->GetTitle());
  fHist.fBelt->Add(fHist.fBeltLo);
  fHist.fBelt->Add(fHist.fBeltHi);
}


//-----------------------------------------------------------------------------
// assume S in 
//-----------------------------------------------------------------------------
int crow_gardner::test_coverage(double MuB, double SMin, double SMax, int NPoints) {
  
  int rc(0);
  rc = construct_belt(MuB,0,35,35001);
  if (rc < 0) return rc;

  float x[NPoints+2], y[NPoints+2];

  x[0] = SMin;
  y[0] = 0;
  double dy = (SMax-SMin)/(NPoints-1);
  for (int i=0; i<NPoints; i++) {
    double s = SMin + i*dy;
    if (s == 0) s = 1.e-10;          // deal with a numerical issue around zero
                                        // now generate pseudoexperiments
    int nmissed = 0;
    for (int k=0; k<fNExp; k++) {
      int nobs = fRn.Poisson(s+MuB);
                                        // assume SMin and SMax define a confidence interval
      double mus = nobs;
                                        // determine SMin and SMax;
      double smin = fBelt.fSign[nobs][0];
      double smax = fBelt.fSign[nobs][1];

      if ((s < smin) or (s > smax)) {
        nmissed += 1;
                                        // print only missed ones
        if (fDebug.fTestCoverage == 2) {
          printf("k, s, MuB, nobs, mus, smin, smax, nmissed : %10i %10.5f %10.3f %3i %10.3f %10.3f %10.3f %10i\n",
                 k, s, MuB, nobs, mus, smin, smax, nmissed);
        }
      }
      if (fDebug.fTestCoverage == 1) {
        printf("k, s, MuB, nobs, mus, smin, smax, nmissed : %10i %10.5f %10.3f %3i %10.3f %10.3f %10.3f %10i\n",
               k, s, MuB, nobs, mus, smin, smax, nmissed);
      }
    }
    float prob = 1.-nmissed/float(fNExp);
    x[i+1] = s;
    y[i+1] = prob;
  }

  x[NPoints+1] = SMax;
  y[NPoints+1] = 0;
  
  if (fHist.fCoverage) delete fHist.fCoverage;
  fHist.fCoverage = new TGraph(NPoints+2,x,y);
  fHist.fCoverage->SetTitle(Form("Crow-Gardner coverage test #mu_{B} = %10.3f",MuB));
  
  return rc;
}
}

