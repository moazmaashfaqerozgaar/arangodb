////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2018 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Michael Hackstein
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGOD_AQL_TRAVERSAL_STATS_H
#define ARANGOD_AQL_TRAVERSAL_STATS_H

#include <cstddef>

#include "ExecutionStats.h"


namespace arangodb {
namespace aql {

class TraversalStats {
 public:
  TraversalStats() noexcept : _filtered(0) {}

  void setFiltered(std::size_t filtered) noexcept { _filtered = filtered; }

  void addFiltered(std::size_t filtered) noexcept { _filtered += filtered; }

  void incrFiltered() noexcept { _filtered++; }

  std::size_t getFiltered() const noexcept { return _filtered; }

 private:
  std::size_t _filtered;

};

inline ExecutionStats& operator+=(ExecutionStats& executionStats,
                           TraversalStats const& traversalStats) noexcept {
  executionStats.filtered += traversalStats.getFiltered();
  return executionStats;
}


} // aql
} // arangodb

#endif
