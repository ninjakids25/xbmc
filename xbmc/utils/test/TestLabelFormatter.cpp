/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ServiceBroker.h"
#include "utils/LabelFormatter.h"
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "FileItem.h"

#include "test/TestUtils.h"

#include "gtest/gtest.h"

/* Set default settings used by CLabelFormatter. */
class TestLabelFormatter : public testing::Test
{
protected:
  TestLabelFormatter()
  {
    //! @todo implement
    /* TODO
    CSettingsCategory* fl = CServiceBroker::GetSettings().AddCategory(7, "filelists", 14081);
    CServiceBroker::GetSettings().AddBool(fl, CSettings::SETTING_FILELISTS_SHOWPARENTDIRITEMS, 13306, true);
    CServiceBroker::GetSettings().AddBool(fl, CSettings::SETTING_FILELISTS_SHOWEXTENSIONS, 497, true);
    CServiceBroker::GetSettings().AddBool(fl, CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING, 13399, true);
    CServiceBroker::GetSettings().AddBool(fl, CSettings::SETTING_FILELISTS_ALLOWFILEDELETION, 14071, false);
    CServiceBroker::GetSettings().AddBool(fl, CSettings::SETTING_FILELISTS_SHOWADDSOURCEBUTTONS, 21382,  true);
    CServiceBroker::GetSettings().AddBool(fl, CSettings::SETTING_FILELISTS_SHOWHIDDEN, 21330, false);
    */
  }

  ~TestLabelFormatter() override
  {
    CServiceBroker::GetSettings().Unload();
  }
};

TEST_F(TestLabelFormatter, FormatLabel)
{
  XFILE::CFile *tmpfile;
  std::string tmpfilepath, destpath;
  LABEL_MASKS labelMasks;
  CLabelFormatter formatter("", labelMasks.m_strLabel2File);

  ASSERT_NE(nullptr, (tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);

  formatter.FormatLabel(item.get());

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
}

TEST_F(TestLabelFormatter, FormatLabel2)
{
  XFILE::CFile *tmpfile;
  std::string tmpfilepath, destpath;
  LABEL_MASKS labelMasks;
  CLabelFormatter formatter("", labelMasks.m_strLabel2File);

  ASSERT_NE(nullptr, (tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
  item->Select(true);

  formatter.FormatLabel2(item.get());

  EXPECT_TRUE(XBMC_DELETETEMPFILE(tmpfile));
}
