//
// Created by Charles Galambos on 15/05/2021.
//

#pragma once

#include <cmath>
#include <array>
#include <tuple>
#include <limits>
#include <ostream>
#include <algorithm>
#include <vector>

namespace Ravl2 {

  //! @brief Keep a ranked array of results.
  //! KeyT = score value, higher values preferred over smaller ones.

  template<typename KeyT,typename DataT,typename ArrayT = std::vector<std::tuple<KeyT,DataT>> >
  class RankedScoreArray
  {
  public:
    RankedScoreArray() = default;

    //! Constructor.
    explicit RankedScoreArray(size_t maxSize,const KeyT &Threshold = -std::numeric_limits<KeyT>::infinity())
      : m_maxSize(maxSize),
        m_worseAcceptableScore(Threshold)
    {
      reset();
    }

    //! Support IO
    template<class Archive>
    void serialize(Archive & archive)
    { archive(m_results,m_maxSize,m_worseAcceptableScore); }

    //! Set the maximum number of results to accumulate
    void setMaxSize(size_t maxSize)
    {
      m_results.reserve(maxSize);
      m_maxSize = maxSize;
    }

    //! Clear array
    void reset() {
      m_results.clear();
      m_results.reserve(m_maxSize);
    }

    //! Reset with with a minimum Score to accept.
    void reset(const KeyT &worstScoreToAccept) {
      m_results.clear();
      m_results.reserve(m_maxSize);
      m_worseAcceptableScore = worstScoreToAccept;
    }

    //! Test a candidate for inclusion into the list, and add it if passes
    //! Returns true if worse candidate has been replaced.
    bool insert(const KeyT &score,const DataT &data) {
      if(m_maxSize > m_results.size()) // Have we filled the list yet?
        return addFill(score,data);
      // Will we displace any result?
      if(score <= std::get<0>(m_results.front()))
        return false;
      // Add pushing the worst out.
      auto x = m_results.begin();
      for(;;) {
        auto x0 = x+1;
        if(x0 == m_results.end() || std::get<0>(*x0) > score)
          break;
        *(x) = std::move(*x0);
        x = x0;
      }
      assert(x != m_results.end());
      *x = {score,data};
      return true;
    }

    //! Access worst accepted Score.
    const KeyT &worstAcceptedScore() const
    { return std::get<0>(m_results[0]); }

    //! Access number of entries.
    [[nodiscard]] size_t size() const
    { return m_results.size(); }

    //! Access nth results.
    // Results are ordered worst (the smallest score) to best (largest).
    const std::tuple<KeyT,DataT> &result(size_t nth) const {
      assert(nth < m_results.size());
      return m_results[nth];
    }

    //! Access result.
    const std::tuple<KeyT,DataT> &operator[](size_t nth) const
    { return result(nth); }

    //! Access best result.
    const DataT &bestResult() const {
      assert(m_results.size() > 0);
      return std::get<1>(m_results.back());
    }

    //! Access best result.
    const KeyT &bestScore() const {
      assert(m_results.size() > 0);
      return std::get<0>(m_results.back());
    }

    //! Test if result set is empty.
    [[nodiscard]] bool empty() const
    { return m_results.empty(); }

    //! Merge another set of results with this one.
    void merge(const RankedScoreArray<KeyT,DataT> &array) {
      for(const auto &x : array.array()) {
        include(x.first, x.second);
      }
    }

    //! Dump to stream in human readable form.
    void dump(std::ostream &out) const {
      out << "RankedScoreArray, " << m_results.size() << " Entries. \n";
      for(const auto &x : m_results) {
        out << " " << std::get<0>(x) << " " << std::get<1>(x) << "\n";
      }
    }

    //! Get maximum number of results.
    [[nodiscard]] size_t maxSize() const
    { return m_maxSize; }

    //! Get the current worst
    const KeyT& worstAcceptableScore() const
    { return m_worseAcceptableScore; }

    //! Get results as array.
    auto &array()
    { return m_results; }

  protected:
    //! Add to a partially filled list.
    //! Returns true if current worse has been replaced.
    bool addFill(const KeyT &score,const DataT &data) {
      if(score < m_worseAcceptableScore)
        return false;

      // Find where to put new entry.
      auto at = std::find_if(m_results.begin(), m_results.end(), [score](auto &x){ return std::get<0>(x) >= score; });

      // Make a space for the new entry.
      bool atEnd = (at == m_results.end());
      m_results.insert(at,{score,data});
      return atEnd;
    }

    size_t m_maxSize = 0;
    ArrayT m_results;
    KeyT m_worseAcceptableScore {};
  };

}

