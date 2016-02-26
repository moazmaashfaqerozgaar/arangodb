////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
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
/// @author Kaveh Vahedipour
////////////////////////////////////////////////////////////////////////////////

#ifndef __ARANGODB_CONSENSUS_AGENCY_COMMON__
#define __ARANGODB_CONSENSUS_AGENCY_COMMON__

#include <Basics/Logger.h>
#include <Basics/VelocyPackHelper.h>

#include <chrono>
#include <initializer_list>
#include <string>
#include <vector>

namespace arangodb {
namespace consensus {

typedef enum AGENCY_STATUS {
  OK = 0,
  RETRACTED_CANDIDACY_FOR_HIGHER_TERM, // Vote for higher term candidate
                                       // while running. Strange!
  RESIGNED_LEADERSHIP_FOR_HIGHER_TERM  // Vote for higher term candidate
                                       // while leading. Very bad!
} status_t;

typedef uint64_t term_t;                           // Term type
typedef uint32_t id_t;                             // Id type

enum role_t {                                      // Role
  FOLLOWER, CANDIDATE, LEADER
};

/**
 * @brief Agent configuration
 */
template<class T> struct Config {               
  T min_ping;
  T max_ping;
  T election_timeout;
  T append_entries_retry_interval;
  id_t id;
  std::vector<std::string> end_points;
  Config () : min_ping(.15), max_ping(.3) {};
  Config (uint32_t i, T min_p, T max_p, T appent_i,
          std::vector<std::string> const& end_p) :
    id(i), min_ping(min_p), max_ping(max_p),
    append_entries_retry_interval(appent_i), end_points(end_p) {}
/*    void print (arangodb::LoggerStream& l) const {
      l << "Config: "
        << "min_ping(" << min_ping << ")"
        << "max_ping(" << max_ping << ")"
        << "size(" << end_points.size() << ")"
        << end_points;
        }*/
  inline size_t size() const {return end_points.size();}
};

using config_t = Config<double>;                      // Configuration type

struct  constituent_t {                               // Constituent type
  id_t id;
  std::string endpoint;
};

typedef std::vector<constituent_t>    constituency_t; // Constituency type
typedef uint32_t                      state_t;        // State type
typedef std::chrono::duration<double> duration_t;     // Duration type

using query_t = std::shared_ptr<arangodb::velocypack::Builder>;

struct vote_ret_t {
  query_t result;
  vote_ret_t (query_t res) : result(res) {}
};

struct read_ret_t {
  bool accepted;  // Query processed
  id_t redirect;  // Otherwise redirect to
  query_t result; // Result
  read_ret_t (bool a, id_t id, query_t res = nullptr) :
    accepted(a), redirect(id), result(res) {}
};

typedef uint64_t index_t;

typedef std::initializer_list<index_t> index_list_t;
struct write_ret_t {
  bool accepted;  // Query processed
  id_t redirect;  // Otherwise redirect to
  std::vector<index_t> lindices; // Indices of log entries (if any) to wait for
  write_ret_t (bool a, id_t id, index_list_t const& idx = index_list_t()) :
    accepted(a), redirect(id), lindices(idx) {}
};

using namespace std::chrono;
/**
 * @brief State entry
 */
struct log_t {
  term_t      term; 
  id_t        leaderId;
  index_t     index;
  std::string entry;
  std::vector<bool> ack;
  milliseconds timestamp;
  log_t (term_t t, id_t lid, index_t idx, std::string const& e,
         std::vector<bool> const& r) :
    term(t), leaderId(lid), index(idx), entry(e), ack(r), timestamp (
      duration_cast<milliseconds>(system_clock::now().time_since_epoch())) {}
};
  

enum AGENCY_EXCEPTION {
  QUERY_NOT_APPLICABLE
};

struct append_entries_t {
  term_t term;
  bool success;
  append_entries_t (term_t t, bool s) : term(t), success(s) {}
};

}}

inline arangodb::LoggerStream& operator<< (
  arangodb::LoggerStream& l, arangodb::consensus::config_t const& c) {
  return l;  
}

#endif // __ARANGODB_CONSENSUS_AGENT__

