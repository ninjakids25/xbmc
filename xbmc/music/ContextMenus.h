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

#include "guilib/GUIWindowManager.h"
#include "music/windows/GUIWindowMusicNav.h"
#include "tags/MusicInfoTag.h"

namespace CONTEXTMENU
{

struct CMusicInfo : CStaticContextMenuAction
{
  const MediaType m_mediaType;
  CMusicInfo(uint32_t label, MediaType mediaType)
      : CStaticContextMenuAction(label), m_mediaType(mediaType) {}

  bool IsVisible(const CFileItem& item) const override
  {
    return item.IsAudio() && item.GetMusicInfoTag()->GetType() == m_mediaType;
  }

  bool Execute(const CFileItemPtr& item) const override
  {
    auto window = static_cast<CGUIWindowMusicNav*>(g_windowManager.GetWindow(WINDOW_MUSIC_NAV));
    if (window)
    {
      ADDON::ScraperPtr info;
      window->OnItemInfo(item.get());
      return true;
    }
    return false;
  }
};

struct CAlbumInfo : CMusicInfo
{
  CAlbumInfo() : CMusicInfo(10523, MediaTypeAlbum) {}
};

struct CArtistInfo : CMusicInfo
{
  CArtistInfo() : CMusicInfo(21891, MediaTypeArtist) {}
};

struct CSongInfo : CMusicInfo
{
  CSongInfo() : CMusicInfo(658, MediaTypeSong) {}
};

}
