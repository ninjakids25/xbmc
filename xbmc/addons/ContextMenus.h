#pragma once
/*
 *      Copyright (C) 2016 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "IAddon.h"
#include "AddonManager.h"
#include "Repository.h"
#include "RepositoryUpdater.h"
#include "GUIDialogAddonInfo.h"
#include "GUIDialogAddonSettings.h"
#include "ContextMenuItem.h"
#include "FileItem.h"

namespace CONTEXTMENU
{

struct CAddonInfo : CStaticContextMenuAction
{
  CAddonInfo() : CStaticContextMenuAction(24003) {}
  bool IsVisible(const CFileItem& item) const override { return item.HasAddonInfo(); }
  bool Execute(const CFileItemPtr& item) const override
  {
    return CGUIDialogAddonInfo::ShowForItem(item);
  }
};

struct CAddonSettings : CStaticContextMenuAction
{
  CAddonSettings() : CStaticContextMenuAction(10140) {}

  bool IsVisible(const CFileItem& item) const override
  {
    using namespace ADDON;
    AddonPtr addon;
    return item.HasAddonInfo()
           && CAddonMgr::GetInstance().GetAddon(item.GetAddonInfo()->ID(), addon, ADDON_UNKNOWN, false)
           && addon->HasSettings();
  }

  bool Execute(const CFileItemPtr& item) const override
  {
    using namespace ADDON;
    AddonPtr addon;
    return CAddonMgr::GetInstance().GetAddon(item->GetAddonInfo()->ID(), addon, ADDON_UNKNOWN, false)
           && CGUIDialogAddonSettings::ShowAndGetInput(addon);
  }
};

struct CCheckForUpdates : CStaticContextMenuAction
{
  CCheckForUpdates() : CStaticContextMenuAction(24034) {}

  bool IsVisible(const CFileItem& item) const override
  {
    return item.HasAddonInfo() && item.GetAddonInfo()->Type() == ADDON::ADDON_REPOSITORY;
  }

  bool Execute(const CFileItemPtr& item) const override
  {
    using namespace ADDON;
    AddonPtr addon;
    if (item->HasAddonInfo() && CAddonMgr::GetInstance().GetAddon(item->GetAddonInfo()->ID(), addon, ADDON_REPOSITORY))
    {
      CRepositoryUpdater::GetInstance().CheckForUpdates(std::static_pointer_cast<CRepository>(addon), true);
      return true;
    }
    return false;
  }
};
}
