//-*-C++-*-
/***************************************************************************
 *
 *   Copyright (C) 2016 by Willem van Straten
 *   Licensed under the Academic Free License version 2.1
 *
 ***************************************************************************/

// epsic/src/util/modulated.h

#ifndef __modulated_H
#define __modulated_H

/***************************************************************************
 *
 *  an amplitude modulated mode of electromagnetic radiation
 *
 ***************************************************************************/

class modulated_mode : public field_transformer
{
#if _DEBUG
  double tot, totsq;
  uint64_t count;
#endif

public:

  modulated_mode (mode* s) : field_transformer(s)
  { 
#if -DEBUG
    tot=0; totsq=0; count=0; 
#endif
}

  // return a random scalar modulation factor
  virtual double modulation () = 0;

  // return the mean of the scalar modulation factor
  virtual double get_mod_mean () const = 0;

  // return the variance of the scalar modulation factor
  virtual double get_mod_variance () const = 0;

  Spinor<double> transform (const Spinor<double>& field)
  {
    double mod = modulation();
#if _DEBUG
    tot+=mod;
    totsq+=mod*mod;
    count+=1;
#endif
    return sqrt(mod) * field;
  }

  Matrix<4,4,double> get_covariance () const
  {
    double mean = get_mod_mean();
    double var = get_mod_variance();

#if _DEBUG
    cerr << "modulated_mode::get_covariance expected mean=" << mean
	 << " var=" << var << endl;
    tot /= count;
    totsq /= count;
    totsq -= tot*tot;
    cerr << " measured mean=" << tot << " var=" << totsq << endl;
#endif

    Matrix<4,4,double> C = source->get_covariance();
    C *= (mean*mean + var);
    Matrix<4,4,double> o = outer (source->get_mean(), source->get_mean());
    o *= var;
    return C + o;
  }

  Stokes<double> get_mean () const
  {
    return get_mod_mean () * source->get_mean();
  }

};

class lognormal_mode : public modulated_mode
{
  // standard deviation of the logarithm of the random variate
  double log_sigma;

public:

  // initialize based on the modulation index beta
  lognormal_mode (mode* s, double beta) : modulated_mode (s) { set_beta (beta); }

  void set_beta (double beta)
  {
    log_sigma = sqrt( log( beta*beta + 1.0 ) );
  }

  double get_beta () const
  {
    return sqrt( get_mod_variance() );
  }

  // return a random scalar modulation factor
  double modulation ()
  {
    return exp ( log_sigma * (get_normal()->evaluate() - 0.5*log_sigma) ) ;
  }

  double get_mod_mean () const { return 1.0; }
  
  double get_mod_variance () const { return exp(log_sigma*log_sigma) - 1.0; }

};


class boxcar_modulated_mode : public modulated_mode
{
  std::vector< double > instances;
  unsigned smooth;
  unsigned current;

  void setup()
  {
    current = 0;
    instances.resize (smooth);
    for (unsigned i=1; i<smooth; i++)
      instances[i] = mod->modulation();
  }

  modulated_mode* mod;

public:

  boxcar_modulated_mode (modulated_mode* s, unsigned n)
    : modulated_mode(s->get_source()) { smooth = n; mod = s; }

  double modulation ()
  {
    if (instances.size() < smooth)
      setup ();

    instances[current] = mod->modulation();
    current = (current + 1) % smooth;

    double result = 0.0;
    for (unsigned i=0; i<smooth; i++)
      result += instances[i];

    result /= smooth;

    return result;
  }

  double get_mod_variance () const
  {
    return mod->get_mod_variance() / smooth;
  }

  double get_mod_mean () const
  {
    return mod->get_mod_mean ();
  }

  Matrix<4,4, double> get_crosscovariance (unsigned ilag) const
  {
    if (ilag >= smooth)
      return 0;

    Matrix<4,4, double> result = outer(source->get_mean(), source->get_mean());
    result *= double (smooth - ilag) / smooth * get_mod_variance();
    return result;
  }
  
};


class square_modulated_mode : public modulated_mode
{
  unsigned width;
  unsigned current;
  double value;

  modulated_mode* mod;

public:

  square_modulated_mode (modulated_mode* s, unsigned n)
    : modulated_mode(s->get_source()) { width = n; current = n; mod = s; }

  double modulation ()
  {
    if (current == width)
    {
      value = mod->modulation();
      current = 0;
    }

    current ++;
    return value;
  }

  double get_mod_variance () const
  {
    return mod->get_mod_variance();
  }

  double get_mod_mean () const
  {
    return mod->get_mod_mean ();
  }

  //! Return cross-covariance between Stokes parameters as a function of lag
  Matrix<4,4, double> get_crosscovariance (unsigned ilag) const
  {
    if (ilag >= width)
      return 0;

    Matrix<4,4, double> result = outer(source->get_mean(), source->get_mean());
    result *= /* double (width - ilag) / width * */ get_mod_variance();
    return result;
  }
};

#endif
