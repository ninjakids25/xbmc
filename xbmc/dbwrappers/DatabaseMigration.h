#pragma once

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

#include <memory>
#include <string>
#include <vector>


class CDatabaseMigration
{
protected:
  friend class CDatabaseManager;
  bool Update(const DatabaseSettings &db);

  /*! \brief Create database tables and analytics as needed.
    Calls CreateTables() and CreateAnalytics() on child classes.
    */
  bool CreateDatabase();

  /* \brief Create tables for the current database schema.
    Will be called on database creation.
    */
  virtual void CreateTables() = 0;

  /* \brief Create views, indices and triggers for the current database schema.
    Will be called on database creation and database update.
    */
  virtual void CreateAnalytics() = 0;

  /* \brief Update database tables to the current version.
    Note that analytics (views, indices, triggers) are not present during this
    function, so don't rely on them.
    */
  virtual void UpdateTables(int version) {};

  /* \brief The minimum schema version that we support updating from.
    */
  virtual int GetMinSchemaVersion() const { return 0; };

  /* \brief The current schema version.
    */
  virtual int GetSchemaVersion() const = 0;
  virtual const char *GetBaseDBName() const = 0;

  int GetDBVersion();
  bool UpdateVersion(const std::string &dbName);

private:
  void UpdateVersionNumber();

  };
