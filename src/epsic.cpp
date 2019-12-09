/***************************************************************************
 *
 *   Copyright (C) 2016 - 2017 by Willem van Straten
 *   Licensed under the Academic Free License version 2.1
 *
 ***************************************************************************/

/*

  Simulates the higher order moments of polarized electromagnetic radiation.
  This software was written to verify the equations presented in

  van Straten & Tiburzi 2017, The Astrophysical Journal, 835:293
  http://dx.doi.org/10.3847/1538-4357/835/2/293

*/

#include "Pauli.h"
#include "Dirac.h"
#include "Jacobi.h"

#include "mode.h"
#include "modulated.h"
#include "smoothed.h"
#include "sample.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <exception>
#include <fstream>

// #define _DEBUG 1

using namespace std;

void usage ()
{
  cout <<

    "simpol: simulate polarized noise and compute statistics \n" 
    " \n"
    "options: \n"
    " \n"
    " -N Msamp    number of Mega (2^20) Stokes samples [default:1]\n"
    " -n Nint     number of instances in each Stokes sample [default:1] \n"
    //" -m Nsamp    box-car smooth over Nsamp samples before detection \n"
    //" -M Nsamp    box-car smooth over Nsamp samples after detection \n"
    " -S          superposed modes \n"
    " -C f_A      composite modes with fraction of instances in mode A \n"
    " -D F_A      disjoint modes with fraction of samples in mode A \n"
    " -c cov      coherent superposition of modes \n"
    " -s i,q,u,v  population mean Stokes parameters [default:1,0,0,0]\n"
    " -l beta     modulation index of log-normal amplitude modulation \n"
    " -b Nsamp    box-car smooth the amplitude modulation function \n"
    " -r Nsamp    use rectangular impulse amplitude modulation function \n"
    " -X Nlag     compute cross-covariance matrices up to Nlag-1 \n"
    " -t          report only theoretical predictions \n"
    " -d          report the means and variances of the Stokes parameters \n"
       << endl;
}

class mode_setup
{
public:
  // population mean Stokes parameters
  Stokes<double> mean;
  // box-car smoothing width pre-detection
  unsigned smooth_before;
  // box-car smoothing of modulation function
  unsigned smooth_modulator;
  // width of square modulation function
  unsigned square_modulator;
  // modulation index of log-normal modulation function
  double beta;

  mode_setup () : mean (1,0,0,0)
  {
    smooth_before = 0;
    smooth_modulator = 0;
    square_modulator = 0;
    beta = 0;
  }

  mode* setup_mode (mode* s)
  {
    modulated_mode* mod = 0;

    if (s)
      s->set_Stokes (mean);
    
    if (beta)
      s = mod = new lognormal_mode (s, beta);

    if (smooth_modulator > 1 && mod)
      s = new boxcar_modulated_mode (mod, smooth_modulator);

    if (square_modulator > 1 && mod)
      s = new square_modulated_mode (mod, square_modulator);

    if (smooth_before > 1)
      s = new boxcar_mode (s, smooth_before);

    return s;
  }
};

double sqr (double x) { return x*x; }

int main (int argc, char** argv)
{
  bool run_simulation = true;
  
  uint64_t Mega = 1024 * 1024;
  uint64_t nsamp = Mega;       // number of Stokes samples
  unsigned nint = 1;           // number of instances in each Stokes sample
  unsigned nlag = 0;           // number of lags to compute in ACF
  unsigned smooth_after = 0;   // box-car smoothing width post-detection

  Stokes<double> stokes = 1.0;
  bool subtract_outer_population_mean = false;

  mode source;
  combination* dual = NULL;
  sample* stokes_sample = NULL;

  mode_setup setup_A;
  mode_setup setup_B;

  bool print = false;
  bool rho_stats = false;
  bool variances_and_means = false;
  
  int c;
  while ((c = getopt(argc, argv, "hN:n:Sc:C:dD:s:l:b:r:X:t")) != -1)
  {
    const char* usearg = optarg;
    mode_setup* setup = &setup_A;
    
    if (optarg && optarg[0] == 'B')
    {
      setup = &setup_B;
      usearg ++;
    }
    
    switch (c)
    {

    case 'h':
      usage ();
      return 0;

    case 'N':
      nsamp = nsamp * atof (optarg);
      break;

    case 'n':
      nint = atoi (optarg);
      break;

    case 'S':
      dual = new superposed;
      break;

    case 'C':
      dual = new composite( atof(optarg) );
      break;

    case 'D':
      dual = new disjoint( atof(optarg) );
      break;

    case 'c':
      dual = new coherent( atof(optarg) );
      break;
      
    case 's':
    {
      double i,q,u,v;
      if (sscanf (usearg, "%lf,%lf,%lf,%lf", &i,&q,&u,&v) != 4)
      {
	cerr << "Error parsing " << usearg << " as 4-vector" << endl;
	return -1;
      }
      stokes = Stokes<double> (i,q,u,v);

      if (stokes.abs_vect() > i) {
	cerr << "Invalid Stokes parameters (p>I) " << stokes << endl;
	return -1;
      }

      setup->mean = stokes;

      break;
    }

    case 'l':
      setup->beta = atof (usearg);
      break;
      
    case 'b':
      setup->smooth_modulator = atoi (usearg);
      break;

    case 'r':
      setup->square_modulator = atoi (usearg);
      break;

    case 'X':
      nlag = atoi (optarg);
      break;

    /* undocumented and currently unavailable features */

    case 'M':
      smooth_after = atoi (optarg);
      break;

    case 'm':
      setup->smooth_before = atoi (usearg);
      break;

    case 'o':
      subtract_outer_population_mean = true;
      break;

    case 'p':
      print = true;
      break;

    case 'R':
      rho_stats = true;
      break;

    case 'd':
      variances_and_means = true;
      break;
      
    case 't':
      run_simulation = false;
      break;
    }
  }

  source.set_Stokes (stokes);

  if (dual)
  {
    stokes_sample = dual;

    dual->A = setup_A.setup_mode (dual->A);
    dual->B = setup_B.setup_mode (dual->B);
  }
  else
  {
    mode* s = setup_A.setup_mode(&source);

    if (smooth_after > 1)
      stokes_sample = new boxcar_sample (s, smooth_after);
    else
      stokes_sample = new single(s);
  }

  stokes_sample->sample_size = nint;

  if (run_simulation)
    cerr << "Simulating " << nsamp << " Stokes samples" << endl;

  random_init ();
  BoxMuller gasdev (time(NULL));

  if (dual)
    dual->set_normal (&gasdev);
  else
    source.set_normal (&gasdev);
    
  uint64_t ntot = 0;
  uint64_t ntot_lag = 0;
  
  double totp = 0;
  Vector<4, double> tot;
  Matrix<4,4, double> totsq;

  Matrix<2,2, complex<double> > tot_rho;
  Matrix<4,4, complex<double> > totsq_rho;

  vector< Matrix<4,4, double> > acf (nlag);
  vector< Vector<4, double> > samples (nlag);
  unsigned current_sample = 0;
  
  for (uint64_t idat=0; run_simulation && idat<nsamp; idat++)
  {
    Vector<4, double> mean_stokes;

    mean_stokes = stokes_sample->get_Stokes();

    tot += mean_stokes;
    totsq += outer(mean_stokes, mean_stokes);
    ntot ++;

    if (nlag)
    {
      samples[current_sample] = mean_stokes;
      current_sample ++;
      if (current_sample == nlag)
	current_sample = 0;

      if (ntot >= nlag)
      {
	Vector<4, double> Sj = samples[current_sample];
	for (unsigned ilag=0; ilag<nlag; ilag++)
	{
	  Vector<4, double> Si = samples[(current_sample+ilag)%nlag];
	  acf[ilag] += outer(Si,Sj);
	}

	ntot_lag++;
      }
    }
    
    double psq = sqr(mean_stokes[1])+sqr(mean_stokes[2])+sqr(mean_stokes[3]);
    totp += sqrt(psq)/mean_stokes[0];
      
    if (rho_stats)
    {
      Matrix<2,2, complex<double> > rho = convert (Stokes<double>(mean_stokes));
    
      tot_rho += rho;
      totsq_rho += direct (rho, rho);
    }
  }

  totp /= ntot;
  tot /= ntot;
  totsq /= ntot;

  if (subtract_outer_population_mean)
    totsq -= outer(stokes,stokes);
  else
    totsq -= outer(tot,tot);

  if (variances_and_means)
  {
    for (unsigned i=0; i<4; i++)
    {
      cout << "mean[" << i << "] = " << tot[i] << endl;
      cout << "var[" << i << "] = " << totsq[i][i] << endl;
    }
    return 0;
  }

  Vector<4, double> expected_mean;
  Matrix<4,4, double> expected_covariance;

  expected_mean = stokes_sample->get_mean ();
  expected_covariance = stokes_sample->get_covariance();

  cerr << "\n"
    " ******************************************************************* \n"
    "\n"
    " STOKES PARAMETERS \n"
    "\n"
    " ******************************************************************* \n"
       << endl;

  if (run_simulation)
  {
    cerr << "mean sample dop=" << totp << endl << endl;

    cerr << "modulation index=" << sqrt(totsq[0][0])/tot[0] << endl << endl;

    cerr << "mean=" << tot << endl;
    cerr << "expected=" << expected_mean << endl;

    cerr << "\ncovar=\n" << totsq << endl;
    cerr << "expected=\n" << expected_covariance << endl;
  }
  
  if (nlag)
  {
    ofstream out ("acf.txt");
    ofstream plot ("acf_plot.txt");
    
    for (unsigned ilag=0; ilag<nlag; ilag++)
    {
      acf[ilag] /= ntot_lag;
      acf[ilag] -= outer(tot,tot);

      Matrix<4,4,double> exp = stokes_sample->get_crosscovariance(ilag);
      
      out << "============================================================\n"
	"lag=" << ilag << endl;
      if (run_simulation)
	out << "mean=" << acf[ilag] << endl;
      out << "expected=" << exp << endl;

      plot << ilag << " ";
      for (unsigned i=0; i<4; i++)
	for (unsigned j=0; j<4; j++)
	{
	  plot << exp[i][j] << " ";
	  if (run_simulation)
	    plot  << acf[ilag][i][j] << " ";
	}
      plot << endl;
    }
  }
  
  if (!rho_stats)
    return 0;

  cerr << "\n"
    " ******************************************************************* \n"
    "\n"
    " COHERENCY MATRIX \n"
    "\n"
    " ******************************************************************* \n"
       << endl;

  tot_rho /= nsamp;
  totsq_rho /= nsamp;

  cerr << "rho sq=\n" << totsq_rho << endl;

  totsq_rho -= direct(tot_rho,tot_rho);

  cerr << "rho mean=\n" << tot_rho << endl;
  cerr << "rho covar=\n" << totsq_rho << endl;

  Matrix<4,4, complex<double> > candidate;
  for (unsigned i=0; i<4; i++)
    for (unsigned j=0; j<4; j++)
      {
	Matrix<4,4,complex<double> > temp = Dirac::matrix (i,j);
	temp *= expected_covariance[i][j] * 0.25;

	candidate += temp;
      }

  cerr << "candidate=\n" << candidate << endl;

  Matrix<4, 4, complex<double> > eigenvectors;
  Vector<4, double> eigenvalues;

  Matrix<4, 4, complex<double> > temp = candidate;
  Jacobi (temp, eigenvectors, eigenvalues);

  for (unsigned i=0; i<4; i++)
    cerr << "e_" << i << "=" << eigenvalues[i] << "  v=" << eigenvectors[i] << endl;

  return 0;
}
