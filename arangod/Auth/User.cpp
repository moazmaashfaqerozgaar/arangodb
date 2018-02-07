////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2018 ArangoDB GmbH, Cologne, Germany
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
/// @author
/// @author Dr. Frank Celler
////////////////////////////////////////////////////////////////////////////////

#include "Auth/User.h"
#include "Basics/ReadLocker.h"
#include "Basics/StaticStrings.h"
#include "Basics/StringRef.h"
#include "Basics/VelocyPackHelper.h"
#include "Basics/WriteLocker.h"
#include "Basics/tri-strings.h"
#include "Cluster/ServerState.h"
#include "GeneralServer/GeneralServerFeature.h"
#include "Logger/Logger.h"
#include "Random/UniformCharacter.h"
#include "RestServer/DatabaseFeature.h"
#include "Ssl/SslInterface.h"
#include "Transaction/Helpers.h"
#include "VocBase/LogicalCollection.h"
#include "VocBase/Methods/Collections.h"
#include "VocBase/Methods/Databases.h"

#include <velocypack/Iterator.h>
#include <velocypack/velocypack-aliases.h>

using namespace arangodb;
using namespace arangodb::basics;
using namespace arangodb::rest;

// private hash function
static int HexHashFromData(std::string const& hashMethod,
                           std::string const& str, std::string& outHash) {
  char* crypted = nullptr;
  size_t cryptedLength;
  char* hex;

  try {
    if (hashMethod == "sha1") {
      arangodb::rest::SslInterface::sslSHA1(str.data(), str.size(), crypted,
                                            cryptedLength);
    } else if (hashMethod == "sha512") {
      arangodb::rest::SslInterface::sslSHA512(str.data(), str.size(), crypted,
                                              cryptedLength);
    } else if (hashMethod == "sha384") {
      arangodb::rest::SslInterface::sslSHA384(str.data(), str.size(), crypted,
                                              cryptedLength);
    } else if (hashMethod == "sha256") {
      arangodb::rest::SslInterface::sslSHA256(str.data(), str.size(), crypted,
                                              cryptedLength);
    } else if (hashMethod == "sha224") {
      arangodb::rest::SslInterface::sslSHA224(str.data(), str.size(), crypted,
                                              cryptedLength);
    } else if (hashMethod == "md5") {  // WFT?!!!
      arangodb::rest::SslInterface::sslMD5(str.data(), str.size(), crypted,
                                           cryptedLength);
    } else {
      // invalid algorithm...
      LOG_TOPIC(DEBUG, arangodb::Logger::FIXME)
          << "invalid algorithm for hexHashFromData: " << hashMethod;
      return TRI_ERROR_BAD_PARAMETER;
    }
  } catch (...) {
    // SslInterface::ssl....() allocate strings with new, which might throw
    // exceptions
    return TRI_ERROR_FAILED;
  }

  if (crypted == nullptr || cryptedLength == 0) {
    delete[] crypted;
    return TRI_ERROR_OUT_OF_MEMORY;
  }

  size_t hexLen;
  hex = TRI_EncodeHexString(crypted, cryptedLength, &hexLen);
  delete[] crypted;

  if (hex == nullptr) {
    return TRI_ERROR_OUT_OF_MEMORY;
  }

  TRI_DEFER(TRI_FreeString(hex));
  outHash = std::string(hex, hexLen);
  return TRI_ERROR_NO_ERROR;
}

static void AddSource(VPackBuilder& builder, auth::Source source) {
  switch (source) {
    case auth::Source::LOCAL: // used to be collection
      builder.add("source", VPackValue("LOCAL"));
      break;
    case auth::Source::LDAP:
      builder.add("source", VPackValue("LDAP"));
      break;
    default:
      TRI_ASSERT(false);
  }
}

static void AddAuthLevel(VPackBuilder& builder, auth::Level lvl) {
  if (lvl == auth::Level::RW) {
    builder.add("read", VPackValue(true));
    builder.add("write", VPackValue(true));
  } else if (lvl == auth::Level::RO) {
    builder.add("read", VPackValue(true));
    builder.add("write", VPackValue(false));
  } else {
    builder.add("read", VPackValue(false));
    builder.add("write", VPackValue(false));
  }
}

static auth::Level AuthLevelFromSlice(VPackSlice const& slice) {
  TRI_ASSERT(slice.isObject());
  VPackSlice v = slice.get("write");
  if (v.isBool() && v.isTrue()) {
    return auth::Level::RW;
  }
  v = slice.get("read");
  if (v.isBool() && v.isTrue()) {
    return auth::Level::RO;
  }
  return auth::Level::NONE;
}

// ============= static ==================

auth::User auth::User::newUser(std::string const& user,
                               std::string const& password,
                               auth::Source source) {
  auth::User entry("", 0);
  entry._active = true;
  entry._source = source;

  entry._username = user;
  entry._passwordMethod = "sha256";

  std::string salt = UniformCharacter(8, "0123456789abcdef").random();
  std::string hash;
  int res = HexHashFromData("sha256", salt + password, hash);
  if (res != TRI_ERROR_NO_ERROR) {
    THROW_ARANGO_EXCEPTION_MESSAGE(res,
                                   "Could not calculate hex-hash from data");
  }

  entry._passwordSalt = salt;
  entry._passwordHash = hash;

  // build authentication entry
  return entry;
}

/*void auth::User::fromDocumentRoles(auth::User& entry,
                                   VPackSlice const& rolesSlice) {
  for (auto const& it : VPackArrayIterator(rolesSlice)) {
    if (it.isString()) {
      std::string const role = it.copyString();

      entry._roles.insert(role);
    }
  }
}*/

void auth::User::fromDocumentDatabases(auth::User& entry,
                                       VPackSlice const& databasesSlice,
                                       VPackSlice const& userSlice) {
  for (auto const& obj : VPackObjectIterator(databasesSlice)) {
    std::string const dbName = obj.key.copyString();

    if (obj.value.isObject()) {
      auth::Level databaseAuth = auth::Level::NONE;

      auto const permissionsSlice = obj.value.get("permissions");

      if (permissionsSlice.isObject()) {
        databaseAuth = AuthLevelFromSlice(permissionsSlice);
      }

      try {
        entry.grantDatabase(dbName, databaseAuth);
      } catch (arangodb::basics::Exception const& e) {
        LOG_TOPIC(DEBUG, Logger::AUTHORIZATION) << e.message();
      }

      VPackSlice collectionsSlice = obj.value.get("collections");

      if (collectionsSlice.isObject()) {
        for (auto const& collection : VPackObjectIterator(collectionsSlice)) {
          std::string const cName = collection.key.copyString();
          auto const permissionsSlice = collection.value.get("permissions");

          if (permissionsSlice.isObject()) {
            try {
              entry.grantCollection(dbName, cName,
                                    AuthLevelFromSlice(permissionsSlice));
            } catch (arangodb::basics::Exception const& e) {
              LOG_TOPIC(DEBUG, Logger::AUTHORIZATION) << e.message();
            }
          }
        }
      }
    } else {
      LOG_TOPIC(DEBUG, arangodb::Logger::CONFIG)
          << "updating deprecated access rights struct for user '"
          << userSlice.copyString() << "'";
      VPackValueLength length;
      char const* value = obj.value.getString(length);

      if (TRI_CaseEqualString(value, "rw", 2)) {
        entry.grantDatabase(dbName, auth::Level::RW);
        entry.grantCollection(dbName, "*", auth::Level::RW);
      } else if (TRI_CaseEqualString(value, "ro", 2)) {
        entry.grantDatabase(dbName, auth::Level::RO);
        entry.grantCollection(dbName, "*", auth::Level::RO);
      }
    }
  }
}

auth::User auth::User::fromDocument(VPackSlice const& slice) {
  if (slice.isNone() || !slice.isObject()) {
    THROW_ARANGO_EXCEPTION(TRI_ERROR_BAD_PARAMETER);
  }

  VPackSlice const keySlice =
      transaction::helpers::extractKeyFromDocument(slice);
  if (!keySlice.isString()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_BAD_PARAMETER,
                                   "cannot extract _key");
  }

  TRI_voc_rid_t rev = transaction::helpers::extractRevFromDocument(slice);
  if (rev == 0) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_BAD_PARAMETER,
                                   "cannot extract _rev");
  }

  // extract "user" attribute
  VPackSlice const userSlice = slice.get("user");
  if (!userSlice.isString()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_BAD_PARAMETER,
                                   "cannot extract username");
  }

  VPackSlice const authDataSlice = slice.get("authData");

  if (!authDataSlice.isObject()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_BAD_PARAMETER,
                                   "cannot extract authData");
  }

  VPackSlice const simpleSlice = authDataSlice.get("simple");

  if (!simpleSlice.isObject()) {
    LOG_TOPIC(DEBUG, arangodb::Logger::FIXME) << "cannot extract simple";
    return auth::User("", 0);
  }

  VPackSlice const methodSlice = simpleSlice.get("method");
  VPackSlice const saltSlice = simpleSlice.get("salt");
  VPackSlice const hashSlice = simpleSlice.get("hash");

  if (!methodSlice.isString() || !saltSlice.isString() ||
      !hashSlice.isString()) {
    LOG_TOPIC(DEBUG, arangodb::Logger::FIXME)
        << "cannot extract password internals";
    return auth::User("", 0);
  }

  // extract "active" attribute
  VPackSlice const activeSlice = authDataSlice.get("active");

  if (!activeSlice.isBoolean()) {
    LOG_TOPIC(DEBUG, arangodb::Logger::FIXME) << "cannot extract active flag";
    return auth::User("", 0);
  }

  auth::User entry(keySlice.copyString(), rev);
  entry._active = activeSlice.getBool();
  entry._source = auth::Source::LOCAL;
  entry._username = userSlice.copyString();
  entry._passwordMethod = methodSlice.copyString();
  entry._passwordSalt = saltSlice.copyString();
  entry._passwordHash = hashSlice.copyString();

  // extract "databases" attribute
  VPackSlice const databasesSlice = slice.get("databases");

  if (databasesSlice.isObject()) {
    fromDocumentDatabases(entry, databasesSlice, userSlice);
  }

  // extract "roles" attribute
  /*VPackSlice const rolesSlice = slice.get("roles");
  if (rolesSlice.isArray()) {
    fromDocumentRoles(entry, rolesSlice);
  }*/

  VPackSlice userDataSlice = slice.get("userData");
  if (userDataSlice.isObject() && !userDataSlice.isEmptyObject()) {
    entry._userData.clear();
    entry._userData.add(userDataSlice);
  }

  VPackSlice userConfigSlice = slice.get("configData");
  if (userConfigSlice.isObject() && !userConfigSlice.isEmptyObject()) {
    entry._configData.clear();
    entry._configData.add(userConfigSlice);
  }

  // ensure the root user always has the right to change permissions
  if (entry._username == "root") {
    entry.grantDatabase(StaticStrings::SystemDatabase, auth::Level::RW);
    entry.grantCollection(StaticStrings::SystemDatabase, "*", auth::Level::RW);
  }

  // build authentication entry
  return entry;
}

// ===================== Constructor =======================

auth::User::User(std::string&& key, TRI_voc_rid_t rid)
    : _key(std::move(key)), _rev(rid), _loaded(TRI_microtime()) {}

// ======================= Methods ==========================

bool auth::User::checkPassword(std::string const& password) const {
  std::string hash;
  int res = HexHashFromData(_passwordMethod, _passwordSalt + password, hash);
  if (res != TRI_ERROR_NO_ERROR) {
    THROW_ARANGO_EXCEPTION_MESSAGE(res,
                                   "Could not calculate hex-hash from input");
  }
  return _passwordHash == hash;
}

void auth::User::updatePassword(std::string const& password) {
  std::string hash;
  int res = HexHashFromData(_passwordMethod, _passwordSalt + password, hash);
  if (res != TRI_ERROR_NO_ERROR) {
    THROW_ARANGO_EXCEPTION_MESSAGE(res,
                                   "Could not calculate hex-hash from input");
  }
  _passwordHash = hash;
}

VPackBuilder auth::User::toVPackBuilder() const {
  TRI_ASSERT(!_username.empty());

  VPackBuilder builder;
  VPackObjectBuilder o(&builder, true);

  if (!_key.empty()) {
    builder.add(StaticStrings::KeyString, VPackValue(_key));
  }
  if (_rev > 0) {
    builder.add(StaticStrings::RevString, VPackValue(TRI_RidToString(_rev)));
  }

  builder.add("user", VPackValue(_username));
  AddSource(builder, _source);

  // authData sub-object
  {
    VPackObjectBuilder o2(&builder, "authData", true);
    builder.add("active", VPackValue(_active));
    if (_source == auth::Source::LOCAL) {
      VPackObjectBuilder o3(&builder, "simple", true);
      builder.add("hash", VPackValue(_passwordHash));
      builder.add("salt", VPackValue(_passwordSalt));
      builder.add("method", VPackValue(_passwordMethod));
    }
  }

  {  // databases sub-object
    VPackObjectBuilder o2(&builder, "databases", true);
    for (auto const& dbCtxPair : _dbAccess) {
      VPackObjectBuilder o3(&builder, dbCtxPair.first, true);

      // permissions
      {
        VPackObjectBuilder o4(&builder, "permissions", true);
        auth::Level lvl = dbCtxPair.second._databaseAuthLevel;
        AddAuthLevel(builder, lvl);
      }

      // collections
      {
        VPackObjectBuilder o4(&builder, "collections", true);

        for (auto const& colAccessPair : dbCtxPair.second._collectionAccess) {
          VPackObjectBuilder o4(&builder, colAccessPair.first, true);
          VPackObjectBuilder o5(&builder, "permissions", true);
          AddAuthLevel(builder, colAccessPair.second);
        }
      }
    }
  }

  if (!_userData.isEmpty() && _userData.isClosed() &&
      _userData.slice().isObject()) {
    builder.add("userData", _userData.slice());
  }

  if (!_configData.isEmpty() && _configData.isClosed() &&
      _configData.slice().isObject()) {
    builder.add("configData", _configData.slice());
  }

  return builder;
}

void auth::User::grantDatabase(std::string const& dbname, auth::Level level) {
  if (dbname.empty()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_BAD_PARAMETER,
                                   "Cannot set rights for empty db name");
  }

  if (_username == "root" && dbname == StaticStrings::SystemDatabase &&
      level != auth::Level::RW) {
    THROW_ARANGO_EXCEPTION_MESSAGE(
        TRI_ERROR_FORBIDDEN, "Cannot lower access level of 'root' to _system");
  }

  auto it = _dbAccess.find(dbname);

  if (it != _dbAccess.end()) {
    it->second._databaseAuthLevel = level;
  } else {
    // grantDatabase is not supposed to change any rights on the
    // collection level code which relies on the old behaviour
    // will need to be adjusted

    _dbAccess.emplace(
        dbname,
        DBAuthContext(level, std::unordered_map<std::string, auth::Level>()));
  }
}

/// Removes the entry, returns true if entry existed
bool auth::User::removeDatabase(std::string const& dbname) {
  if (dbname.empty()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_BAD_PARAMETER,
                                   "Cannot remove rights for empty db name");
  }
  if (_username == "root" && dbname == StaticStrings::SystemDatabase) {
    THROW_ARANGO_EXCEPTION_MESSAGE(
        TRI_ERROR_FORBIDDEN, "Cannot remove access level of 'root' to _system");
  }
  return _dbAccess.erase(dbname) > 0;
}

void auth::User::grantCollection(std::string const& dbname,
                                 std::string const& coll,
                                 auth::Level const level) {
  if (dbname.empty() || coll.empty()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(
        TRI_ERROR_BAD_PARAMETER,
        "Cannot set rights for empty db / collection name");
  } else if (coll[0] == '_') {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_BAD_PARAMETER,
                                   "Cannot set rights for system collections");
  } else if (_username == "root" && dbname == StaticStrings::SystemDatabase &&
             coll == "*" && level != auth::Level::RW) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_FORBIDDEN,
                                   "Cannot lower access level of 'root' to "
                                   " a system collection");
  }
  auto it = _dbAccess.find(dbname);
  if (it != _dbAccess.end()) {
    it->second._collectionAccess[coll] = level;
  } else {
    // do not overwrite wildcard access to a database, by granting more
    // specific rights to a collection in a specific db
    auth::Level dbLevel = auth::Level::NONE;
    it = _dbAccess.find("*");
    if (it != _dbAccess.end()) {
      dbLevel = it->second._databaseAuthLevel;
    }
    _dbAccess.emplace(
        dbname,
        DBAuthContext(dbLevel, std::unordered_map<std::string, auth::Level>(
                                   {{coll, level}})));
  }
}

/// Removes the collection right, returns true if entry existed
bool auth::User::removeCollection(std::string const& dbname,
                                  std::string const& coll) {
  if (dbname.empty() || coll.empty()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(
        TRI_ERROR_BAD_PARAMETER,
        "Cannot set rights for empty db / collection name");
  }
  if (_username == "root" && dbname == StaticStrings::SystemDatabase &&
      (coll == "*")) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_FORBIDDEN,
                                   "Cannot lower access level of 'root' to "
                                   " a collection in _system");
  }
  auto const& it = _dbAccess.find(dbname);
  if (it != _dbAccess.end()) {
    return it->second._collectionAccess.erase(coll) > 0;
  }
  return false;
}

auth::Level auth::User::databaseAuthLevel(std::string const& dbname) const {
  auto it = _dbAccess.find(dbname);
  if (it != _dbAccess.end()) {
    return it->second._databaseAuthLevel;
  }
  it = _dbAccess.find("*");
  if (it != _dbAccess.end()) {
    return it->second._databaseAuthLevel;
  }
  return auth::Level::NONE;
}

/// Find the access level for a collection. Will automatically try to fall back
auth::Level auth::User::collectionAuthLevel(
    std::string const& dbname, std::string const& collectionName) const {
  // disallow access to _system/_users for everyone
  if (collectionName.empty()) {
    return auth::Level::NONE;
  }
  bool isSystem = collectionName[0] == '_';
  if (isSystem) {
    if (dbname == TRI_VOC_SYSTEM_DATABASE &&
        collectionName == TRI_COL_NAME_USERS) {
      return auth::Level::NONE;
    } else if (collectionName == "_queues") {
      return auth::Level::RO;
    } else if (collectionName == "_frontend") {
      return auth::Level::RW;
    }
  }

  bool notFound = false;
  auth::Level lvl = auth::Level::NONE;
  auto it = _dbAccess.find(dbname);
  if (it != _dbAccess.end()) {
    if (isSystem) {
      return it->second._databaseAuthLevel;
    }
    lvl = it->second.collectionAuthLevel(collectionName, notFound);
  } else {
    notFound = true;
  }
  // the lookup into the default database is only allowed if there were
  // no rights for it defined in the database
  if (notFound) {
    it = _dbAccess.find("*");
    if (it != _dbAccess.end()) {
      if (isSystem) {
        return it->second._databaseAuthLevel;
      }
      lvl = it->second.collectionAuthLevel(collectionName, notFound);
    }
  }

  return lvl;
}

bool auth::User::hasSpecificDatabase(std::string const& dbname) const {
  return _dbAccess.find(dbname) != _dbAccess.end();
}

bool auth::User::hasSpecificCollection(
    std::string const& dbname, std::string const& collectionName) const {
  auto it = _dbAccess.find(dbname);
  if (it != _dbAccess.end()) {
    return it->second._collectionAccess.find(collectionName) !=
           it->second._collectionAccess.end();
  }
  return false;
}

auth::Level auth::User::DBAuthContext::collectionAuthLevel(
    std::string const& collectionName, bool& notFound) const {
  std::unordered_map<std::string, auth::Level>::const_iterator pair =
      _collectionAccess.find(collectionName);
  if (pair != _collectionAccess.end()) {
    return pair->second;
  }
  pair = _collectionAccess.find("*");
  if (pair != _collectionAccess.end()) {
    return pair->second;
  }
  notFound = true;
  return _databaseAuthLevel;// auth::Level::NONE;
}
