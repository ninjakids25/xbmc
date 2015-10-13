#pragma once
/*
*      Copyright (C) 2005-2015 Team Kodi
*      http://kodi.tv
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

#include "dbwrappers/Database.h"

class CSearchDatabase : public CDatabase
{
public:
  CSearchDatabase();
  virtual ~CSearchDatabase();
  virtual bool Open();

  // TODO
  auto Search();
  bool AddItem();
  bool ClearIndex();

protected:
  virtual void CreateTables();
  virtual void CreateAnalytics();
  virtual void UpdateTables(int version);
  virtual int GetSchemaVersion() const { return 1; }
  const char *GetBaseDBName() const { return "Search"; }
};
