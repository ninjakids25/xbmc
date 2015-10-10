/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DatabaseMigration.h"
#include "Database.h"
#include "settings/AdvancedSettings.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/File.h"
#include "profiles/ProfilesManager.h"
#include "utils/log.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "sqlitedataset.h"
#include "DatabaseManager.h"
#include "DbUrl.h"

#ifdef HAS_MYSQL
#include "mysqldataset.h"
#endif

using namespace dbiplus;

#define MAX_COMPRESS_COUNT 20

bool CDatabaseMigration::CreateDatabase()
{
  BeginTransaction();
  try
  {
    CLog::Log(LOGINFO, "creating version table");
    m_pDS->exec("CREATE TABLE version (idVersion integer, iCompressCount integer)\n");
    std::string strSQL = PrepareSQL("INSERT INTO version (idVersion,iCompressCount) values(%i,0)\n", GetSchemaVersion());
    m_pDS->exec(strSQL);

    CreateTables();
    CreateAnalytics();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to create database:%i", __FUNCTION__, (int)GetLastError());
    RollbackTransaction();
    return false;
  }
  CommitTransaction();
  return true;
}


bool CDatabaseMigration::Update(const DatabaseSettings &settings)
{
  DatabaseSettings dbSettings = settings;
  InitSettings(dbSettings);

  int version = GetSchemaVersion();
  std::string latestDb = dbSettings.name;
  latestDb += StringUtils::Format("%d", version);

  while (version >= GetMinSchemaVersion())
  {
    std::string dbName = dbSettings.name;
    if (version)
      dbName += StringUtils::Format("%d", version);

    if (Connect(dbName, dbSettings, false))
    {
      // Database exists, take a copy for our current version (if needed) and reopen that one
      if (version < GetSchemaVersion())
      {
        CLog::Log(LOGNOTICE, "Old database found - updating from version %i to %i", version, GetSchemaVersion());

        bool copy_fail = false;

        try
        {
          m_pDB->copy(latestDb.c_str());
        }
        catch(...)
        {
          CLog::Log(LOGERROR, "Unable to copy old database %s to new version %s", dbName.c_str(), latestDb.c_str());
          copy_fail = true;
        }

        Close();

        if ( copy_fail )
          return false;

        if (!Connect(latestDb, dbSettings, false))
        {
          CLog::Log(LOGERROR, "Unable to open freshly copied database %s", latestDb.c_str());
          return false;
        }
      }

      // yay - we have a copy of our db, now do our worst with it
      if (UpdateVersion(latestDb))
        return true;

      // update failed - loop around and see if we have another one available
      Close();
    }

    // drop back to the previous version and try that
    version--;
  }
  // try creating a new one
  if (Connect(latestDb, dbSettings, true))
    return true;

  // failed to update or open the database
  Close();
  CLog::Log(LOGERROR, "Unable to create new database");
  return false;
}

int CDatabaseMigration::GetDBVersion()
{
  m_pDS->query("SELECT idVersion FROM version\n");
  if (m_pDS->num_rows() > 0)
    return m_pDS->fv("idVersion").get_asInt();
  return 0;
}

bool CDatabaseMigration::UpdateVersion(const std::string &dbName)
{
  int version = GetDBVersion();
  if (version < GetMinSchemaVersion())
  {
    CLog::Log(LOGERROR, "Can't update database %s from version %i - it's too old", dbName.c_str(), version);
    return false;
  }
  else if (version < GetSchemaVersion())
  {
    CLog::Log(LOGNOTICE, "Attempting to update the database %s from version %i to %i", dbName.c_str(), version, GetSchemaVersion());
    bool success = true;
    BeginTransaction();
    try
    {
      // drop old analytics, update table(s), recreate analytics, update version
      m_pDB->drop_analytics();
      UpdateTables(version);
      CreateAnalytics();
      UpdateVersionNumber();
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Exception updating database %s from version %i to %i", dbName.c_str(), version, GetSchemaVersion());
      success = false;
    }
    if (!success)
    {
      CLog::Log(LOGERROR, "Error updating database %s from version %i to %i", dbName.c_str(), version, GetSchemaVersion());
      RollbackTransaction();
      return false;
    }
    CommitTransaction();
    CLog::Log(LOGINFO, "Update to version %i successful", GetSchemaVersion());
  }
  else if (version > GetSchemaVersion())
  {
    CLog::Log(LOGERROR, "Can't open the database %s as it is a NEWER version than what we were expecting?", dbName.c_str());
    return false;
  }
  else 
    CLog::Log(LOGNOTICE, "Running database version %s", dbName.c_str());
  return true;
}

bool CDatabaseMigration::UpdateVersion(const std::string &dbName)
{
  int version = GetDBVersion();
  if (version < GetMinSchemaVersion())
  {
    CLog::Log(LOGERROR, "Can't update database %s from version %i - it's too old", dbName.c_str(), version);
    return false;
  }
  else if (version < GetSchemaVersion())
  {
    CLog::Log(LOGNOTICE, "Attempting to update the database %s from version %i to %i", dbName.c_str(), version, GetSchemaVersion());
    bool success = true;
    BeginTransaction();
    try
    {
      // drop old analytics, update table(s), recreate analytics, update version
      m_pDB->drop_analytics();
      UpdateTables(version);
      CreateAnalytics();
      UpdateVersionNumber();
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Exception updating database %s from version %i to %i", dbName.c_str(), version, GetSchemaVersion());
      success = false;
    }
    if (!success)
    {
      CLog::Log(LOGERROR, "Error updating database %s from version %i to %i", dbName.c_str(), version, GetSchemaVersion());
      RollbackTransaction();
      return false;
    }
    CommitTransaction();
    CLog::Log(LOGINFO, "Update to version %i successful", GetSchemaVersion());
  }
  else if (version > GetSchemaVersion())
  {
    CLog::Log(LOGERROR, "Can't open the database %s as it is a NEWER version than what we were expecting?", dbName.c_str());
    return false;
  }
  else
    CLog::Log(LOGNOTICE, "Running database version %s", dbName.c_str());
  return true;
}

void CDatabaseMigration::UpdateVersionNumber()
{
  std::string strSQL = PrepareSQL("UPDATE version SET idVersion=%i\n", GetSchemaVersion());
  m_pDS->exec(strSQL);
}
