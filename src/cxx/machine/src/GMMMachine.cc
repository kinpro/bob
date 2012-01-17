/**
 * @file src/cxx/machine/src/GMMMachine.cc
 * @author <a href="mailto:Roy.Wallace@idiap.ch">Roy Wallace</a> 
 * @author <a href="mailto:Francois.Moulin@idiap.ch">Francois Moulin</a>
 * @author <a href="mailto:Laurent.El-Shafey@idiap.ch">Laurent El Shafey</a> 
 *
 * Copyright (C) 2011 Idiap Reasearch Institute, Martigny, Switzerland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "machine/GMMMachine.h"
#include "core/array_assert.h"
#include "core/logging.h"
#include "machine/Exception.h"

namespace ca = bob::core::array;
namespace TLog = bob::machine::Log;
namespace mach = bob::machine;

mach::GMMMachine::GMMMachine(): m_gaussians(0) {
  resize(0,0);
}

mach::GMMMachine::GMMMachine(size_t n_gaussians, size_t n_inputs): 
  m_gaussians(0)
{
  resize(n_gaussians,n_inputs);
}

mach::GMMMachine::GMMMachine(bob::io::HDF5File& config): 
  m_gaussians(0)
{
  load(config);
}

mach::GMMMachine::GMMMachine(const GMMMachine& other): 
  Machine<blitz::Array<double,1>, double>(other), m_gaussians(0)
{
  copy(other);
}

mach::GMMMachine& mach::GMMMachine::operator=(const mach::GMMMachine &other) {
  // protect against invalid self-assignment
  if (this != &other) 
    copy(other);
  
  // by convention, always return *this
  return *this;
}

bool mach::GMMMachine::operator==(const mach::GMMMachine& b) const {
  if(m_n_gaussians != b.m_n_gaussians || m_n_inputs != b.m_n_inputs) 
    return false;

  for(size_t i=0; i<m_n_gaussians; ++i) {
    if(!(*(m_gaussians[i]) == *(b.m_gaussians[i])))
      return false;
  }

  if(blitz::any(m_weights != b.m_weights))
    return false;

  return true;
}

void mach::GMMMachine::copy(const GMMMachine& other) {
  m_n_gaussians = other.m_n_gaussians;
  m_n_inputs = other.m_n_inputs;

  // Initialise weights
  m_weights.resize(m_n_gaussians);
  m_weights = other.m_weights;

  // Initialise Gaussians
  m_gaussians.clear();
  for(size_t i=0; i<m_n_gaussians; ++i) {
    boost::shared_ptr<mach::Gaussian> g(new mach::Gaussian(*(other.m_gaussians[i])));
    m_gaussians.push_back(g);
  }

  // Initialise cache
  initCache();
}

mach::GMMMachine::~GMMMachine() { }

void mach::GMMMachine::setNInputs(size_t n_inputs) {
  resize(m_n_gaussians,n_inputs);
}

void mach::GMMMachine::resize(size_t n_gaussians, size_t n_inputs) {
  m_n_gaussians = n_gaussians;
  m_n_inputs = n_inputs;

  // Initialise weights
  m_weights.resize(m_n_gaussians);
  m_weights = 1.0 / m_n_gaussians;

  // Initialise Gaussians
  m_gaussians.clear();
  for(size_t i=0; i<m_n_gaussians; ++i) 
    m_gaussians.push_back(boost::shared_ptr<mach::Gaussian>(new mach::Gaussian(n_inputs)));

  // Initialise cache arrays
  initCache();
}


void mach::GMMMachine::setWeights(const blitz::Array<double,1> &weights) {
  ca::assertSameShape(weights, m_weights);
  m_weights = weights;
}

void mach::GMMMachine::setMeans(const blitz::Array<double,2> &means) {
  ca::assertSameDimensionLength(means.extent(0), m_n_gaussians);
  ca::assertSameDimensionLength(means.extent(1), m_n_inputs);
  for(size_t i=0; i<m_n_gaussians; ++i)
    m_gaussians[i]->updateMean() = means(i,blitz::Range::all());
  m_cache_supervector = false;
}

void mach::GMMMachine::getMeans(blitz::Array<double,2> &means) const {
  ca::assertSameDimensionLength(means.extent(0), m_n_gaussians);
  ca::assertSameDimensionLength(means.extent(1), m_n_inputs);
  for(size_t i=0; i<m_n_gaussians; ++i) 
    means(i,blitz::Range::all()) = m_gaussians[i]->getMean(); 
}

void mach::GMMMachine::setMeanSupervector(const blitz::Array<double,1> &mean_supervector) {
  ca::assertSameDimensionLength(mean_supervector.extent(0), m_n_gaussians*m_n_inputs);
  for(size_t i=0; i<m_n_gaussians; ++i) 
    m_gaussians[i]->updateMean() = mean_supervector(blitz::Range(i*m_n_inputs, (i+1)*m_n_inputs-1));
  m_cache_supervector = false;
}

void mach::GMMMachine::getMeanSupervector(blitz::Array<double,1> &mean_supervector) const {
  ca::assertSameDimensionLength(mean_supervector.extent(0), m_n_gaussians*m_n_inputs);
  for(size_t i=0; i<m_n_gaussians; ++i)
    mean_supervector(blitz::Range(i*m_n_inputs, (i+1)*m_n_inputs-1)) = m_gaussians[i]->getMean(); 
}

void mach::GMMMachine::setVariances(const blitz::Array<double, 2 >& variances) {
  ca::assertSameDimensionLength(variances.extent(0), m_n_gaussians);
  ca::assertSameDimensionLength(variances.extent(1), m_n_inputs);
  for(size_t i=0; i<m_n_gaussians; ++i) {
    m_gaussians[i]->updateVariance() = variances(i,blitz::Range::all());
    m_gaussians[i]->applyVarianceThresholds();
  }
  m_cache_supervector = false;
}

void mach::GMMMachine::getVariances(blitz::Array<double, 2 >& variances) const {
  ca::assertSameDimensionLength(variances.extent(0), m_n_gaussians);
  ca::assertSameDimensionLength(variances.extent(1), m_n_inputs);
  for(size_t i=0; i<m_n_gaussians; ++i) 
    variances(i,blitz::Range::all()) = m_gaussians[i]->getVariance();
}

void mach::GMMMachine::setVarianceSupervector(const blitz::Array<double,1> &variance_supervector) {
  ca::assertSameDimensionLength(variance_supervector.extent(0), m_n_gaussians*m_n_inputs);
  for(size_t i=0; i<m_n_gaussians; ++i) {
    m_gaussians[i]->updateVariance() = variance_supervector(blitz::Range(i*m_n_inputs, (i+1)*m_n_inputs-1));
    m_gaussians[i]->applyVarianceThresholds();
  }
  m_cache_supervector = false;
}

void mach::GMMMachine::getVarianceSupervector(blitz::Array<double,1> &variance_supervector) const {
  ca::assertSameDimensionLength(variance_supervector.extent(0), m_n_gaussians*m_n_inputs);
  for(size_t i=0; i<m_n_gaussians; ++i) {
    variance_supervector(blitz::Range(i*m_n_inputs, (i+1)*m_n_inputs-1)) = m_gaussians[i]->getVariance(); 
  }
}

void mach::GMMMachine::setVarianceThresholds(double factor) {
  for(size_t i=0; i<m_n_gaussians; ++i) 
    m_gaussians[i]->setVarianceThresholds(factor);
  m_cache_supervector = false;
}

void mach::GMMMachine::setVarianceThresholds(blitz::Array<double, 1> variance_thresholds) {
  ca::assertSameDimensionLength(variance_thresholds.extent(0), m_n_inputs);
  for(size_t i=0; i<m_n_gaussians; ++i) 
    m_gaussians[i]->setVarianceThresholds(variance_thresholds);
  m_cache_supervector = false;
}

void mach::GMMMachine::setVarianceThresholds(const blitz::Array<double, 2>& variance_thresholds) {
  ca::assertSameDimensionLength(variance_thresholds.extent(0), m_n_gaussians);
  ca::assertSameDimensionLength(variance_thresholds.extent(1), m_n_inputs);
  for(size_t i=0; i<m_n_gaussians; ++i)
    m_gaussians[i]->setVarianceThresholds(variance_thresholds(i,blitz::Range::all())); 
  m_cache_supervector = false;
}

void mach::GMMMachine::getVarianceThresholds(blitz::Array<double, 2>& variance_thresholds) const {
  ca::assertSameDimensionLength(variance_thresholds.extent(0), m_n_gaussians);
  ca::assertSameDimensionLength(variance_thresholds.extent(1), m_n_inputs);
  for(size_t i=0; i<m_n_gaussians; ++i) 
    variance_thresholds(i,blitz::Range::all()) = m_gaussians[i]->getVarianceThresholds();
}

double mach::GMMMachine::logLikelihood(const blitz::Array<double, 1> &x, 
  blitz::Array<double,1> &log_weighted_gaussian_likelihoods) const 
{
  // Initialise variables
  ca::assertSameDimensionLength(log_weighted_gaussian_likelihoods.extent(0), m_n_gaussians);
  ca::assertSameDimensionLength(x.extent(0), m_n_inputs);
  double log_likelihood = TLog::LogZero;

  // Accumulate the weighted log likelihoods from each Gaussian
  for(size_t i=0; i<m_n_gaussians; ++i) {
    double l = log(m_weights(i)) + m_gaussians[i]->logLikelihood_(x);
    log_weighted_gaussian_likelihoods(i) = l;
    log_likelihood = TLog::LogAdd(log_likelihood, l);
  }

  // Return log(p(x|GMMMachine))
  return log_likelihood;
}

double mach::GMMMachine::logLikelihood(const blitz::Array<double, 1> &x) const {
  // Call the other logLikelihood (overloaded) function
  // (log_weighted_gaussian_likelihoods will be discarded)
  return logLikelihood(x,m_cache_log_weighted_gaussian_likelihoods);
}

void mach::GMMMachine::forward(const blitz::Array<double,1>& input, double& output) const {
  if(static_cast<size_t>(input.extent(0)) != m_n_inputs) {
    throw NInputsMismatch(m_n_inputs, input.extent(0));
  }

  forward_(input,output);
}

void mach::GMMMachine::forward_(const blitz::Array<double,1>& input, double& output) const {
  output = logLikelihood(input);
}

void mach::GMMMachine::accStatistics(const bob::io::Arrayset& ar, mach::GMMStats& stats) const {
  // iterate over data
  for(size_t i=0; i<ar.size(); ++i) {
    // Get example
    blitz::Array<double,1> x(ar.get<double,1>(i));

    // Accumulate statistics
    accStatistics(x,stats);
  }
}

void mach::GMMMachine::accStatistics(const blitz::Array<double, 1>& x, mach::GMMStats& stats) const {

  // Calculate Gaussian and GMM likelihoods
  // - m_cache_log_weighted_gaussian_likelihoods(i) = log(weight_i*p(x|gaussian_i))
  // - log_likelihood = log(sum_i(weight_i*p(x|gaussian_i)))
  double log_likelihood = logLikelihood(x, m_cache_log_weighted_gaussian_likelihoods);

  // Calculate responsibilities
  m_cache_P.resize(m_n_gaussians);
  m_cache_P = blitz::exp(m_cache_log_weighted_gaussian_likelihoods - log_likelihood);

  // Accumulate statistics
  // - total likelihood
  stats.log_likelihood += log_likelihood;

  // - number of samples
  stats.T++;

  // - responsibilities
  stats.n += m_cache_P;

  // - first order stats
  m_cache_Px.resize(m_n_gaussians,m_n_inputs);
  blitz::firstIndex i;
  blitz::secondIndex j;
  
  m_cache_Px = m_cache_P(i) * x(j);
  
  /*
  std::cout << "P:" << m_cache_P << std::endl;
  std::cout << "x:" << x << std::endl;
  std::cout << "Px:" << m_cache_Px << std::endl;
  std::cout << "sumPx:" << stats.sumPx << std::endl;
  */
  stats.sumPx += m_cache_Px;
  //std::cout << "sumPx:" << stats.sumPx << std::endl;

  // - second order stats
  m_cache_Pxx.resize(m_n_gaussians,m_n_inputs);
  m_cache_Pxx = m_cache_Px(i,j) * x(j);
  stats.sumPxx += m_cache_Pxx;
}


boost::shared_ptr<mach::Gaussian> mach::GMMMachine::getGaussian(size_t i) {
  if(i<0 || i>=m_n_gaussians) 
    throw bob::machine::Exception();
  return m_gaussians[i];
}

void mach::GMMMachine::save(bob::io::HDF5File& config) const {
  int64_t v = static_cast<int64_t>(m_n_gaussians);
  config.set("m_n_gaussians", v);
  v = static_cast<int64_t>(m_n_inputs);
  config.set("m_n_inputs", v);

  for(size_t i=0; i<m_n_gaussians; ++i) {
    std::ostringstream oss;
    oss << "m_gaussians" << i;
    
    config.cd(oss.str());
    m_gaussians[i]->save(config);
    config.cd("..");
  }

  config.setArray("m_weights", m_weights);
}

void mach::GMMMachine::load(bob::io::HDF5File& config) {
  int64_t v;
  v = config.read<int64_t>("m_n_gaussians");
  m_n_gaussians = static_cast<size_t>(v);
  v = config.read<int64_t>("m_n_inputs");
  m_n_inputs = static_cast<size_t>(v);
  
  m_gaussians.clear();
  for(size_t i=0; i<m_n_gaussians; ++i) {
    m_gaussians.push_back(boost::shared_ptr<mach::Gaussian>(new mach::Gaussian(m_n_inputs)));
    std::ostringstream oss;
    oss << "m_gaussians" << i;
    config.cd(oss.str());
    m_gaussians[i]->load(config);
    config.cd("..");
  }

  m_weights.resize(m_n_gaussians);
  config.readArray("m_weights", m_weights);

  // Initialise cache
  initCache();
}

void mach::GMMMachine::updateCacheSupervectors() const
{
  m_cache_mean_supervector.resize(m_n_gaussians*m_n_inputs);
  m_cache_variance_supervector.resize(m_n_gaussians*m_n_inputs);
  
  for(size_t i=0; i<m_n_gaussians; ++i) {
    blitz::Range range(i*m_n_inputs, (i+1)*m_n_inputs-1);
    m_cache_mean_supervector(range) = m_gaussians[i]->getMean();
    m_cache_variance_supervector(range) = m_gaussians[i]->getVariance();
  }
  m_cache_supervector = true;
}

void mach::GMMMachine::initCache() const {
  // Initialise cache arrays
  m_cache_log_weighted_gaussian_likelihoods.resize(m_n_gaussians);
  m_cache_P.resize(m_n_gaussians);
  m_cache_Px.resize(m_n_gaussians,m_n_inputs);
  m_cache_Pxx.resize(m_n_gaussians,m_n_inputs);
  m_cache_supervector = false;
}

void mach::GMMMachine::reloadCacheSupervectors() const {
  if(!m_cache_supervector)
    updateCacheSupervectors();
}

const blitz::Array<double,1>& mach::GMMMachine::getMeanSupervector() const {
  if(!m_cache_supervector)
    updateCacheSupervectors();
  return m_cache_mean_supervector;
} 

const blitz::Array<double,1>& mach::GMMMachine::getVarianceSupervector() const {
  if(!m_cache_supervector)
    updateCacheSupervectors();
  return m_cache_variance_supervector;
} 

namespace bob {
  namespace machine {
    std::ostream& operator<<(std::ostream& os, const GMMMachine& machine) {
      os << "Weights = " << machine.m_weights << std::endl;
      for(size_t i=0; i < machine.m_n_gaussians; ++i) {
        os << "Gaussian " << i << ": " << std::endl << *(machine.m_gaussians[i]);
      }

      return os;
    }
  }
}
