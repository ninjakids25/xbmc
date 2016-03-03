#pragma once
/*
 *      Copyright (C) 2015 Team XBMC
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

#include <map>

#include "addons/IAddon.h"
#include "addons/AddonManager.h"
#include "addons/Repository.h"
#include "addons/RepositoryUpdater.h"
#include "addons/GUIDialogAddonInfo.h"
#include "addons/GUIDialogAddonSettings.h"
#include "guilib/GUIWindowManager.h"
#include "video/windows/GUIWindowVideoNav.h"


namespace ADDON
{
  class CContextMenuAddon;
}

class IContextMenuItem
{
public:
  virtual bool IsVisible(const CFileItem& item) const = 0;
  virtual bool Execute(const CFileItemPtr& item) const = 0;
  virtual std::string GetLabel(const CFileItem& item) const = 0;
  virtual bool IsGroup() const { return false; }
};


class CStaticContextMenuAction : public IContextMenuItem
{
  const uint32_t m_label;
public:
  CStaticContextMenuAction(uint32_t label) : m_label(label) {}
  std::string GetLabel(const CFileItem& item) const override final
  {
    return g_localizeStrings.Get(m_label);
  }
  bool IsGroup() const override final { return false; }
};


class CContextMenuItem : public IContextMenuItem
{
public:
  std::string GetLabel(const CFileItem& item) const { return m_label; }
  bool IsVisible(const CFileItem& item) const override ;
  bool IsParentOf(const CContextMenuItem& menuItem) const;
  bool IsGroup() const override ;
  bool Execute(const CFileItemPtr& item) const override;
  bool operator==(const CContextMenuItem& other) const;
  std::string ToString() const;

  static CContextMenuItem CreateGroup(
    const std::string& label,
    const std::string& parent,
    const std::string& groupId);

  static CContextMenuItem CreateItem(
    const std::string& label,
    const std::string& parent,
    const std::string& library,
    const INFO::InfoPtr& condition);

  friend class ADDON::CContextMenuAddon;

private:
  std::string m_label;
  std::string m_parent;
  std::string m_groupId;
  std::string m_library;
  INFO::InfoPtr m_condition;
  ADDON::AddonPtr m_addon;
};

namespace contextmenu
{
namespace actions
{

struct CAddonInfo : IContextMenuItem
{
  std::string GetLabel(const CFileItem& item) const override { return g_localizeStrings.Get(24003); }

  bool IsVisible(const CFileItem& item) const override
  {
    return item.HasAddonInfo();
  }

  bool Execute(const CFileItemPtr& item) const override
  {
    return CGUIDialogAddonInfo::ShowForItem(item);
  }
};

struct CAddonSettings : IContextMenuItem
{
  std::string GetLabel(const CFileItem& item) const override { return g_localizeStrings.Get(24020); }

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

struct CCheckForUpdates : IContextMenuItem
{
  std::string GetLabel(const CFileItem& item) const override { return g_localizeStrings.Get(24034); }

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

struct CTVShowInfo : IContextMenuItem
{
  std::string GetLabel(const CFileItem& item) const override
  {
    return g_localizeStrings.Get(20351);
  }

  bool IsVisible(const CFileItem& item) const override
  {
    return item.HasVideoInfoTag() && item.GetVideoInfoTag()->m_type == MediaTypeTvShow;
  }

  bool Execute(const CFileItemPtr& item) const override
  {
    auto window = static_cast<CGUIWindowVideoNav*>(g_windowManager.GetWindow(WINDOW_VIDEO_NAV));
    if (window)
    {
      ADDON::ScraperPtr info;
      window->OnItemInfo(item.get(), info);
      return true;
    }
    return false;
  }
};

}
}
