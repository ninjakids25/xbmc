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

#include "SearchDatabase.h"
#include "dbwrappers/dataset.h"
#include "utils/log.h"

CSearchDatabase::CSearchDatabase(void)
{ }

CSearchDatabase::~CSearchDatabase(void)
{ }

bool CSearchDatabase::Open()
{
  return CDatabase::Open();
}

void CSearchDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "create search table");
  m_pDS->exec("CREATE TABLE search ("
    "id integer primary key,"
    "objectId integer,"
    "objectType text,"
    "objectDatabase text,"
    "searchString text"
    ")");
}

void CSearchDatabase::CreateAnalytics()
{
  CLog::Log(LOGINFO, "%s - creating indicies", __FUNCTION__);
  m_pDS->exec("CREATE INDEX idxSearch ON search(searchString)");
}

void CSearchDatabase::UpdateTables(int version)
{
 
}
