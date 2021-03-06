/**
 * @file bob/visioner/model/trainers/lutproblems/lut_problem.h
 * @date Fri 27 Jul 13:58:57 2012 CEST
 * @author Andre Anjos <andre.anjos@idiap.ch>
 *
 * @brief This file was part of Visioner and originally authored by "Cosmin
 * Atanasoaei <cosmin.atanasoaei@idiap.ch>". It was only modified to conform to
 * Bob coding standards and structure.
 *
 * Copyright (C) 2011-2013 Idiap Research Institute, Martigny, Switzerland
 */

#ifndef BOB_VISIONER_LUT_PROBLEM_H
#define BOB_VISIONER_LUT_PROBLEM_H

#include "bob/visioner/model/dataset.h"
#include "bob/visioner/model/model.h"
#include "bob/visioner/model/loss.h"

namespace bob { namespace visioner {

  /**
   * LUTProblem: greedily train std::vector<LUT> in boosting rounds.
   */
  class LUTProblem {

    public:

      // Constructor
      LUTProblem(const DataSet& dataset, const param_t& param, size_t threads);

      // Destructor
      virtual ~LUTProblem() {}

      // Update loss values and derivatives
      virtual void update_loss_deriv() = 0;
      virtual void update_loss() = 0;

      // Select the optimal feature
      virtual void select() = 0;

      // Compute the loss value/error
      virtual double value() const = 0;
      virtual double error() const = 0;

      // Compute the gradient <g> and the function value in the <x> point
      //      (used during linesearch)
      virtual double linesearch(const double* x, double* g) = 0;

      // Access functions
      uint64_t n_entries() const { return m_data.n_fvalues(); }
      uint64_t n_features() const { return m_data.n_features(); }
      uint64_t n_samples() const { return m_data.n_samples(); }
      uint64_t n_outputs() const { return m_data.n_outputs(); }

      uint16_t fvalue(uint64_t f, uint64_t s) const { return m_data.value(f, s); }
      const double* target(uint64_t s) const { return m_data.targets()[s]; }
      double cost(uint64_t s) const { return m_data.cost(s); }

      const std::vector<std::vector<LUT> >& mluts() const { return m_mluts; }
      const std::vector<LUT>& luts() const { return m_luts; }

      // Update predictions
      void update_scores(const std::vector<LUT>& luts);

      // Optimize the LUT entries for the selected feature
      bool line_search();

    private: //multi-threading

      // Update predictions
      void update_scores_mt(const std::vector<LUT>& luts, 
          const std::pair<uint64_t,uint64_t>& range);

      // Optimize the LUT entries for the selected feature
      void line_search_mt(const std::pair<uint64_t,uint64_t>& range);

    protected:

      // Update current scores
      void update_cscores(const double* x);

    private:
 
      // Update scores (worker thread)
      void update_cscores_mt(const double* x,
          const std::pair<uint64_t,uint64_t>& range);

    protected:

      // Attributes
      const DataSet&          m_data;         // Dataset
      param_t                 m_param;

      const boost::shared_ptr<Loss>           m_rloss;        // Base loss
      const Loss&             m_loss;         

      FeatureSharingType      m_sharing;      // Feature sharing method

      std::vector<std::vector<LUT> >		m_mluts;	// Trained model
      std::vector<LUT>			m_luts;		// Buffered std::vector<LUT>

      Matrix<double>            m_sscores;	// Strong learner's score: (sample, output)
      Matrix<double>		m_wscores;	// Weak learner's score: (sample, output)
      Matrix<double>            m_cscores;      // Current (strong + scale * weak) scores: (sample, output)

      Matrix<double>            m_umasks;       // Entries mask [0/1]: (feature, entry)

      size_t m_threads; ///< number of threads to use
  };

}}

#endif // BOB_VISIONER_LUT_PROBLEM_H
