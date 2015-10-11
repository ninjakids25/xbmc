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

#include <cstdlib>

#include "FileItemList.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Archive.h"
#include "Util.h"
#include "playlists/PlayListFactory.h"
#include "utils/Crc32.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/CurlFile.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "filesystem/VideoDatabaseDirectory/QueryParams.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "CueDocument.h"
#include "video/VideoDatabase.h"
#include "music/MusicDatabase.h"
#include "epg/Epg.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRRadioRDSInfoTag.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "video/VideoInfoTag.h"
#include "threads/SingleLock.h"
#include "music/tags/MusicInfoTag.h"
#include "pictures/PictureInfoTag.h"
#include "music/Artist.h"
#include "music/Album.h"
#include "URL.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/RegExp.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "music/karaoke/karaokelyricsfactory.h"
#include "utils/Mime.h"

#include <assert.h>
#include <algorithm>

using namespace XFILE;
using namespace PLAYLIST;
using namespace MUSIC_INFO;
using namespace PVR;
using namespace EPG;


CFileItemList::CFileItemList()
: CFileItem("", true),
  m_fastLookup(false),
  m_sortIgnoreFolders(false),
  m_cacheToDisc(CACHE_IF_SLOW),
  m_replaceListing(false)
{
}

CFileItemList::CFileItemList(const std::string& strPath)
: CFileItem(strPath, true),
  m_fastLookup(false),
  m_sortIgnoreFolders(false),
  m_cacheToDisc(CACHE_IF_SLOW),
  m_replaceListing(false)
{
}

CFileItemList::~CFileItemList()
{
  Clear();
}

CFileItemPtr CFileItemList::operator[] (int iItem)
{
  return Get(iItem);
}

const CFileItemPtr CFileItemList::operator[] (int iItem) const
{
  return Get(iItem);
}

CFileItemPtr CFileItemList::operator[] (const std::string& strPath)
{
  return Get(strPath);
}

const CFileItemPtr CFileItemList::operator[] (const std::string& strPath) const
{
  return Get(strPath);
}

void CFileItemList::SetFastLookup(bool fastLookup)
{
  CSingleLock lock(m_lock);

  if (fastLookup && !m_fastLookup)
  { // generate the map
    m_map.clear();
    for (unsigned int i=0; i < m_items.size(); i++)
    {
      CFileItemPtr pItem = m_items[i];
      m_map.insert(MAPFILEITEMSPAIR(pItem->GetPath(), pItem));
    }
  }
  if (!fastLookup && m_fastLookup)
    m_map.clear();
  m_fastLookup = fastLookup;
}

bool CFileItemList::Contains(const std::string& fileName) const
{
  CSingleLock lock(m_lock);

  if (m_fastLookup)
    return m_map.find(fileName) != m_map.end();

  // slow method...
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    const CFileItemPtr pItem = m_items[i];
    if (pItem->IsPath(fileName))
      return true;
  }
  return false;
}

void CFileItemList::Clear()
{
  CSingleLock lock(m_lock);

  ClearItems();
  m_sortDescription.sortBy = SortByNone;
  m_sortDescription.sortOrder = SortOrderNone;
  m_sortDescription.sortAttributes = SortAttributeNone;
  m_sortIgnoreFolders = false;
  m_cacheToDisc = CACHE_IF_SLOW;
  m_sortDetails.clear();
  m_replaceListing = false;
  m_content.clear();
}

void CFileItemList::ClearItems()
{
  CSingleLock lock(m_lock);
  // make sure we free the memory of the items (these are GUIControls which may have allocated resources)
  FreeMemory();
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItemPtr item = m_items[i];
    item->FreeMemory();
  }
  m_items.clear();
  m_map.clear();
}

void CFileItemList::Add(const CFileItemPtr &pItem)
{
  CSingleLock lock(m_lock);

  m_items.push_back(pItem);
  if (m_fastLookup)
  {
    m_map.insert(MAPFILEITEMSPAIR(pItem->GetPath(), pItem));
  }
}

void CFileItemList::AddFront(const CFileItemPtr &pItem, int itemPosition)
{
  CSingleLock lock(m_lock);

  if (itemPosition >= 0)
  {
    m_items.insert(m_items.begin()+itemPosition, pItem);
  }
  else
  {
    m_items.insert(m_items.begin()+(m_items.size()+itemPosition), pItem);
  }
  if (m_fastLookup)
  {
    m_map.insert(MAPFILEITEMSPAIR(pItem->GetPath(), pItem));
  }
}

void CFileItemList::Remove(CFileItem* pItem)
{
  CSingleLock lock(m_lock);

  for (IVECFILEITEMS it = m_items.begin(); it != m_items.end(); ++it)
  {
    if (pItem == it->get())
    {
      m_items.erase(it);
      if (m_fastLookup)
      {
        m_map.erase(pItem->GetPath());
      }
      break;
    }
  }
}

void CFileItemList::Remove(int iItem)
{
  CSingleLock lock(m_lock);

  if (iItem >= 0 && iItem < (int)Size())
  {
    CFileItemPtr pItem = *(m_items.begin() + iItem);
    if (m_fastLookup)
    {
      m_map.erase(pItem->GetPath());
    }
    m_items.erase(m_items.begin() + iItem);
  }
}

void CFileItemList::Append(const CFileItemList& itemlist)
{
  CSingleLock lock(m_lock);

  for (int i = 0; i < itemlist.Size(); ++i)
    Add(itemlist[i]);
}

void CFileItemList::Assign(const CFileItemList& itemlist, bool append)
{
  CSingleLock lock(m_lock);
  if (!append)
    Clear();
  Append(itemlist);
  SetPath(itemlist.GetPath());
  SetLabel(itemlist.GetLabel());
  m_sortDetails = itemlist.m_sortDetails;
  m_sortDescription = itemlist.m_sortDescription;
  m_replaceListing = itemlist.m_replaceListing;
  m_content = itemlist.m_content;
  m_mapProperties = itemlist.m_mapProperties;
  m_cacheToDisc = itemlist.m_cacheToDisc;
}

bool CFileItemList::Copy(const CFileItemList& items, bool copyItems /* = true */)
{
  // assign all CFileItem parts
  *(CFileItem*)this = *(CFileItem*)&items;

  // assign the rest of the CFileItemList properties
  m_replaceListing  = items.m_replaceListing;
  m_content         = items.m_content;
  m_mapProperties   = items.m_mapProperties;
  m_cacheToDisc     = items.m_cacheToDisc;
  m_sortDetails     = items.m_sortDetails;
  m_sortDescription = items.m_sortDescription;
  m_sortIgnoreFolders = items.m_sortIgnoreFolders;

  if (copyItems)
  {
    // make a copy of each item
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr newItem(new CFileItem(*items[i]));
      Add(newItem);
    }
  }

  return true;
}

CFileItemPtr CFileItemList::Get(int iItem)
{
  CSingleLock lock(m_lock);

  if (iItem > -1 && iItem < (int)m_items.size())
    return m_items[iItem];

  return CFileItemPtr();
}

const CFileItemPtr CFileItemList::Get(int iItem) const
{
  CSingleLock lock(m_lock);

  if (iItem > -1 && iItem < (int)m_items.size())
    return m_items[iItem];

  return CFileItemPtr();
}

CFileItemPtr CFileItemList::Get(const std::string& strPath)
{
  CSingleLock lock(m_lock);

  if (m_fastLookup)
  {
    IMAPFILEITEMS it=m_map.find(strPath);
    if (it != m_map.end())
      return it->second;

    return CFileItemPtr();
  }
  // slow method...
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->IsPath(strPath))
      return pItem;
  }

  return CFileItemPtr();
}

const CFileItemPtr CFileItemList::Get(const std::string& strPath) const
{
  CSingleLock lock(m_lock);

  if (m_fastLookup)
  {
    std::map<std::string, CFileItemPtr>::const_iterator it=m_map.find(strPath);
    if (it != m_map.end())
      return it->second;

    return CFileItemPtr();
  }
  // slow method...
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->IsPath(strPath))
      return pItem;
  }

  return CFileItemPtr();
}

int CFileItemList::Size() const
{
  CSingleLock lock(m_lock);
  return (int)m_items.size();
}

bool CFileItemList::IsEmpty() const
{
  CSingleLock lock(m_lock);
  return m_items.empty();
}

void CFileItemList::Reserve(int iCount)
{
  CSingleLock lock(m_lock);
  m_items.reserve(iCount);
}

void CFileItemList::Sort(FILEITEMLISTCOMPARISONFUNC func)
{
  CSingleLock lock(m_lock);
  std::stable_sort(m_items.begin(), m_items.end(), func);
}

void CFileItemList::FillSortFields(FILEITEMFILLFUNC func)
{
  CSingleLock lock(m_lock);
  std::for_each(m_items.begin(), m_items.end(), func);
}

void CFileItemList::Sort(SortBy sortBy, SortOrder sortOrder, SortAttribute sortAttributes /* = SortAttributeNone */)
{
  if (sortBy == SortByNone ||
     (m_sortDescription.sortBy == sortBy && m_sortDescription.sortOrder == sortOrder &&
      m_sortDescription.sortAttributes == sortAttributes))
    return;

  SortDescription sorting;
  sorting.sortBy = sortBy;
  sorting.sortOrder = sortOrder;
  sorting.sortAttributes = sortAttributes;

  Sort(sorting);
  m_sortDescription = sorting;
}

void CFileItemList::Sort(SortDescription sortDescription)
{
  if (sortDescription.sortBy == SortByFile ||
      sortDescription.sortBy == SortBySortTitle ||
      sortDescription.sortBy == SortByDateAdded ||
      sortDescription.sortBy == SortByRating ||
      sortDescription.sortBy == SortByYear ||
      sortDescription.sortBy == SortByPlaylistOrder ||
      sortDescription.sortBy == SortByLastPlayed ||
      sortDescription.sortBy == SortByPlaycount)
    sortDescription.sortAttributes = (SortAttribute)((int)sortDescription.sortAttributes | SortAttributeIgnoreFolders);

  if (sortDescription.sortBy == SortByNone ||
     (m_sortDescription.sortBy == sortDescription.sortBy && m_sortDescription.sortOrder == sortDescription.sortOrder &&
      m_sortDescription.sortAttributes == sortDescription.sortAttributes))
    return;

  if (m_sortIgnoreFolders)
    sortDescription.sortAttributes = (SortAttribute)((int)sortDescription.sortAttributes | SortAttributeIgnoreFolders);

  const Fields fields = SortUtils::GetFieldsForSorting(sortDescription.sortBy);
  SortItems sortItems((size_t)Size());
  for (int index = 0; index < Size(); index++)
  {
    sortItems[index] = std::shared_ptr<SortItem>(new SortItem);
    m_items[index]->ToSortable(*sortItems[index], fields);
    (*sortItems[index])[FieldId] = index;
  }

  // do the sorting
  SortUtils::Sort(sortDescription, sortItems);

  // apply the new order to the existing CFileItems
  VECFILEITEMS sortedFileItems;
  sortedFileItems.reserve(Size());
  for (SortItems::const_iterator it = sortItems.begin(); it != sortItems.end(); it++)
  {
    CFileItemPtr item = m_items[(int)(*it)->at(FieldId).asInteger()];
    // Set the sort label in the CFileItem
    item->SetSortLabel((*it)->at(FieldSort).asWideString());

    sortedFileItems.push_back(item);
  }

  // replace the current list with the re-ordered one
  m_items.assign(sortedFileItems.begin(), sortedFileItems.end());
}

void CFileItemList::Randomize()
{
  CSingleLock lock(m_lock);
  std::random_shuffle(m_items.begin(), m_items.end());
}

void CFileItemList::Archive(CArchive& ar)
{
  CSingleLock lock(m_lock);
  if (ar.IsStoring())
  {
    CFileItem::Archive(ar);

    int i = 0;
    if (m_items.size() > 0 && m_items[0]->IsParentFolder())
      i = 1;

    ar << (int)(m_items.size() - i);

    ar << m_fastLookup;

    ar << (int)m_sortDescription.sortBy;
    ar << (int)m_sortDescription.sortOrder;
    ar << (int)m_sortDescription.sortAttributes;
    ar << m_sortIgnoreFolders;
    ar << (int)m_cacheToDisc;

    ar << (int)m_sortDetails.size();
    for (unsigned int j = 0; j < m_sortDetails.size(); ++j)
    {
      const GUIViewSortDetails &details = m_sortDetails[j];
      ar << (int)details.m_sortDescription.sortBy;
      ar << (int)details.m_sortDescription.sortOrder;
      ar << (int)details.m_sortDescription.sortAttributes;
      ar << details.m_buttonLabel;
      ar << details.m_labelMasks.m_strLabelFile;
      ar << details.m_labelMasks.m_strLabelFolder;
      ar << details.m_labelMasks.m_strLabel2File;
      ar << details.m_labelMasks.m_strLabel2Folder;
    }

    ar << m_content;

    for (; i < (int)m_items.size(); ++i)
    {
      CFileItemPtr pItem = m_items[i];
      ar << *pItem;
    }
  }
  else
  {
    CFileItemPtr pParent;
    if (!IsEmpty())
    {
      CFileItemPtr pItem=m_items[0];
      if (pItem->IsParentFolder())
        pParent.reset(new CFileItem(*pItem));
    }

    SetFastLookup(false);
    Clear();


    CFileItem::Archive(ar);

    int iSize = 0;
    ar >> iSize;
    if (iSize <= 0)
      return ;

    if (pParent)
    {
      m_items.reserve(iSize + 1);
      m_items.push_back(pParent);
    }
    else
      m_items.reserve(iSize);

    bool fastLookup=false;
    ar >> fastLookup;

    int tempint;
    ar >> (int&)tempint;
    m_sortDescription.sortBy = (SortBy)tempint;
    ar >> (int&)tempint;
    m_sortDescription.sortOrder = (SortOrder)tempint;
    ar >> (int&)tempint;
    m_sortDescription.sortAttributes = (SortAttribute)tempint;
    ar >> m_sortIgnoreFolders;
    ar >> (int&)tempint;
    m_cacheToDisc = CACHE_TYPE(tempint);

    unsigned int detailSize = 0;
    ar >> detailSize;
    for (unsigned int j = 0; j < detailSize; ++j)
    {
      GUIViewSortDetails details;
      ar >> (int&)tempint;
      details.m_sortDescription.sortBy = (SortBy)tempint;
      ar >> (int&)tempint;
      details.m_sortDescription.sortOrder = (SortOrder)tempint;
      ar >> (int&)tempint;
      details.m_sortDescription.sortAttributes = (SortAttribute)tempint;
      ar >> details.m_buttonLabel;
      ar >> details.m_labelMasks.m_strLabelFile;
      ar >> details.m_labelMasks.m_strLabelFolder;
      ar >> details.m_labelMasks.m_strLabel2File;
      ar >> details.m_labelMasks.m_strLabel2Folder;
      m_sortDetails.push_back(details);
    }

    ar >> m_content;

    for (int i = 0; i < iSize; ++i)
    {
      CFileItemPtr pItem(new CFileItem);
      ar >> *pItem;
      Add(pItem);
    }

    SetFastLookup(fastLookup);
  }
}

void CFileItemList::FillInDefaultIcons()
{
  CSingleLock lock(m_lock);
  for (int i = 0; i < (int)m_items.size(); ++i)
  {
    CFileItemPtr pItem = m_items[i];
    pItem->FillInDefaultIcon();
  }
}

int CFileItemList::GetFolderCount() const
{
  CSingleLock lock(m_lock);
  int nFolderCount = 0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->m_bIsFolder)
      nFolderCount++;
  }

  return nFolderCount;
}

int CFileItemList::GetObjectCount() const
{
  CSingleLock lock(m_lock);

  int numObjects = (int)m_items.size();
  if (numObjects && m_items[0]->IsParentFolder())
    numObjects--;

  return numObjects;
}

int CFileItemList::GetFileCount() const
{
  CSingleLock lock(m_lock);
  int nFileCount = 0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (!pItem->m_bIsFolder)
      nFileCount++;
  }

  return nFileCount;
}

int CFileItemList::GetSelectedCount() const
{
  CSingleLock lock(m_lock);
  int count = 0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->IsSelected())
      count++;
  }

  return count;
}

void CFileItemList::FilterCueItems()
{
  CSingleLock lock(m_lock);
  // Handle .CUE sheet files...
  std::vector<std::string> itemstodelete;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (!pItem->m_bIsFolder)
    { // see if it's a .CUE sheet
      if (pItem->IsCUESheet())
      {
        CCueDocumentPtr cuesheet(new CCueDocument);
        if (cuesheet->ParseFile(pItem->GetPath()))
        {
          std::vector<std::string> MediaFileVec;
          cuesheet->GetMediaFiles(MediaFileVec);

          // queue the cue sheet and the underlying media file for deletion
          for(std::vector<std::string>::iterator itMedia = MediaFileVec.begin(); itMedia != MediaFileVec.end(); itMedia++)
          {
            std::string strMediaFile = *itMedia;
            std::string fileFromCue = strMediaFile; // save the file from the cue we're matching against,
                                                   // as we're going to search for others here...
            bool bFoundMediaFile = CFile::Exists(strMediaFile);
            if (!bFoundMediaFile)
            {
              // try file in same dir, not matching case...
              if (Contains(strMediaFile))
              {
                bFoundMediaFile = true;
              }
              else
              {
                // try removing the .cue extension...
                strMediaFile = pItem->GetPath();
                URIUtils::RemoveExtension(strMediaFile);
                CFileItem item(strMediaFile, false);
                if (item.IsAudio() && Contains(strMediaFile))
                {
                  bFoundMediaFile = true;
                }
                else
                { // try replacing the extension with one of our allowed ones.
                  std::vector<std::string> extensions = StringUtils::Split(g_advancedSettings.GetMusicExtensions(), "|");
                  for (std::vector<std::string>::const_iterator i = extensions.begin(); i != extensions.end(); ++i)
                  {
                    strMediaFile = URIUtils::ReplaceExtension(pItem->GetPath(), *i);
                    CFileItem item(strMediaFile, false);
                    if (!item.IsCUESheet() && !item.IsPlayList() && Contains(strMediaFile))
                    {
                      bFoundMediaFile = true;
                      break;
                    }
                  }
                }
              }
            }
            if (bFoundMediaFile)
            {
              cuesheet->UpdateMediaFile(fileFromCue, strMediaFile);
              // apply CUE for later processing
              for (int j = 0; j < (int)m_items.size(); j++)
              {
                CFileItemPtr pItem = m_items[j];
                if (stricmp(pItem->GetPath().c_str(), strMediaFile.c_str()) == 0)
                  pItem->SetCueDocument(cuesheet);
              }
            }
          }
        }
        itemstodelete.push_back(pItem->GetPath());
      }
    }
  }
  // now delete the .CUE files.
  for (int i = 0; i < (int)itemstodelete.size(); i++)
  {
    for (int j = 0; j < (int)m_items.size(); j++)
    {
      CFileItemPtr pItem = m_items[j];
      if (stricmp(pItem->GetPath().c_str(), itemstodelete[i].c_str()) == 0)
      { // delete this item
        m_items.erase(m_items.begin() + j);
        break;
      }
    }
  }
}

// Remove the extensions from the filenames
void CFileItemList::RemoveExtensions()
{
  CSingleLock lock(m_lock);
  for (int i = 0; i < Size(); ++i)
    m_items[i]->RemoveExtension();
}

void CFileItemList::Stack(bool stackFiles /* = true */)
{
  CSingleLock lock(m_lock);

  // not allowed here
  if (IsVirtualDirectoryRoot() ||
      IsLiveTV() ||
      IsSourcesPath() ||
      IsLibraryFolder())
    return;

  SetProperty("isstacked", true);

  // items needs to be sorted for stuff below to work properly
  Sort(SortByLabel, SortOrderAscending);

  StackFolders();

  if (stackFiles)
    StackFiles();
}

void CFileItemList::StackFolders()
{
  // Precompile our REs
  VECCREGEXP folderRegExps;
  CRegExp folderRegExp(true, CRegExp::autoUtf8);
  const std::vector<std::string>& strFolderRegExps = g_advancedSettings.m_folderStackRegExps;

  std::vector<std::string>::const_iterator strExpression = strFolderRegExps.begin();
  while (strExpression != strFolderRegExps.end())
  {
    if (!folderRegExp.RegComp(*strExpression))
      CLog::Log(LOGERROR, "%s: Invalid folder stack RegExp:'%s'", __FUNCTION__, strExpression->c_str());
    else
      folderRegExps.push_back(folderRegExp);

    strExpression++;
  }

  if (!folderRegExp.IsCompiled())
  {
    CLog::Log(LOGDEBUG, "%s: No stack expressions available. Skipping folder stacking", __FUNCTION__);
    return;
  }

  // stack folders
  for (int i = 0; i < Size(); i++)
  {
    CFileItemPtr item = Get(i);
    // combined the folder checks
    if (item->m_bIsFolder)
    {
      // only check known fast sources?
      // NOTES:
      // 1. rars and zips may be on slow sources? is this supposed to be allowed?
      if( !item->IsRemote()
        || item->IsSmb()
        || item->IsNfs() 
        || URIUtils::IsInRAR(item->GetPath())
        || URIUtils::IsInZIP(item->GetPath())
        || URIUtils::IsOnLAN(item->GetPath())
        )
      {
        // stack cd# folders if contains only a single video file

        bool bMatch(false);

        VECCREGEXP::iterator expr = folderRegExps.begin();
        while (!bMatch && expr != folderRegExps.end())
        {
          //CLog::Log(LOGDEBUG,"%s: Running expression %s on %s", __FUNCTION__, expr->GetPattern().c_str(), item->GetLabel().c_str());
          bMatch = (expr->RegFind(item->GetLabel().c_str()) != -1);
          if (bMatch)
          {
            CFileItemList items;
            CDirectory::GetDirectory(item->GetPath(),items,g_advancedSettings.m_videoExtensions);
            // optimized to only traverse listing once by checking for filecount
            // and recording last file item for later use
            int nFiles = 0;
            int index = -1;
            for (int j = 0; j < items.Size(); j++)
            {
              if (!items[j]->m_bIsFolder)
              {
                nFiles++;
                index = j;
              }

              if (nFiles > 1)
                break;
            }

            if (nFiles == 1)
              *item = *items[index];
          }
          expr++;
        }

        // check for dvd folders
        if (!bMatch)
        {
          std::string dvdPath = item->GetOpticalMediaPath();

          if (!dvdPath.empty())
          {
            // NOTE: should this be done for the CD# folders too?
            item->m_bIsFolder = false;
            item->SetPath(dvdPath);
            item->SetLabel2("");
            item->SetLabelPreformated(true);
            m_sortDescription.sortBy = SortByNone; /* sorting is now broken */
          }
        }
      }
    }
  }
}

void CFileItemList::StackFiles()
{
  // Precompile our REs
  VECCREGEXP stackRegExps;
  CRegExp tmpRegExp(true, CRegExp::autoUtf8);
  const std::vector<std::string>& strStackRegExps = g_advancedSettings.m_videoStackRegExps;
  std::vector<std::string>::const_iterator strRegExp = strStackRegExps.begin();
  while (strRegExp != strStackRegExps.end())
  {
    if (tmpRegExp.RegComp(*strRegExp))
    {
      if (tmpRegExp.GetCaptureTotal() == 4)
        stackRegExps.push_back(tmpRegExp);
      else
        CLog::Log(LOGERROR, "Invalid video stack RE (%s). Must have 4 captures.", strRegExp->c_str());
    }
    strRegExp++;
  }

  // now stack the files, some of which may be from the previous stack iteration
  int i = 0;
  while (i < Size())
  {
    CFileItemPtr item1 = Get(i);

    // skip folders, nfo files, playlists
    if (item1->m_bIsFolder
      || item1->IsParentFolder()
      || item1->IsNFO()
      || item1->IsPlayList()
      )
    {
      // increment index
      i++;
      continue;
    }

    int64_t               size        = 0;
    size_t                offset      = 0;
    std::string           stackName;
    std::string           file1;
    std::string           filePath;
    std::vector<int>      stack;
    VECCREGEXP::iterator  expr        = stackRegExps.begin();

    URIUtils::Split(item1->GetPath(), filePath, file1);
    if (URIUtils::HasEncodedFilename(CURL(filePath)))
      file1 = CURL::Decode(file1);

    int j;
    while (expr != stackRegExps.end())
    {
      if (expr->RegFind(file1, offset) != -1)
      {
        std::string Title1      = expr->GetMatch(1),
                    Volume1     = expr->GetMatch(2),
                    Ignore1     = expr->GetMatch(3),
                    Extension1  = expr->GetMatch(4);
        if (offset)
          Title1 = file1.substr(0, expr->GetSubStart(2));
        j = i + 1;
        while (j < Size())
        {
          CFileItemPtr item2 = Get(j);

          // skip folders, nfo files, playlists
          if (item2->m_bIsFolder
            || item2->IsParentFolder()
            || item2->IsNFO()
            || item2->IsPlayList()
            )
          {
            // increment index
            j++;
            continue;
          }

          std::string file2, filePath2;
          URIUtils::Split(item2->GetPath(), filePath2, file2);
          if (URIUtils::HasEncodedFilename(CURL(filePath2)) )
            file2 = CURL::Decode(file2);

          if (expr->RegFind(file2, offset) != -1)
          {
            std::string  Title2      = expr->GetMatch(1),
                        Volume2     = expr->GetMatch(2),
                        Ignore2     = expr->GetMatch(3),
                        Extension2  = expr->GetMatch(4);
            if (offset)
              Title2 = file2.substr(0, expr->GetSubStart(2));
            if (StringUtils::EqualsNoCase(Title1, Title2))
            {
              if (!StringUtils::EqualsNoCase(Volume1, Volume2))
              {
                if (StringUtils::EqualsNoCase(Ignore1, Ignore2) &&
                    StringUtils::EqualsNoCase(Extension1, Extension2))
                {
                  if (stack.size() == 0)
                  {
                    stackName = Title1 + Ignore1 + Extension1;
                    stack.push_back(i);
                    size += item1->m_dwSize;
                  }
                  stack.push_back(j);
                  size += item2->m_dwSize;
                }
                else // Sequel
                {
                  offset = 0;
                  expr++;
                  break;
                }
              }
              else if (!StringUtils::EqualsNoCase(Ignore1, Ignore2)) // False positive, try again with offset
              {
                offset = expr->GetSubStart(3);
                break;
              }
              else // Extension mismatch
              {
                offset = 0;
                expr++;
                break;
              }
            }
            else // Title mismatch
            {
              offset = 0;
              expr++;
              break;
            }
          }
          else // No match 2, next expression
          {
            offset = 0;
            expr++;
            break;
          }
          j++;
        }
        if (j == Size())
          expr = stackRegExps.end();
      }
      else // No match 1
      {
        offset = 0;
        expr++;
      }
      if (stack.size() > 1)
      {
        // have a stack, remove the items and add the stacked item
        // dont actually stack a multipart rar set, just remove all items but the first
        std::string stackPath;
        if (Get(stack[0])->IsRAR())
          stackPath = Get(stack[0])->GetPath();
        else
        {
          CStackDirectory dir;
          stackPath = dir.ConstructStackPath(*this, stack);
        }
        item1->SetPath(stackPath);
        // clean up list
        for (unsigned k = 1; k < stack.size(); k++)
          Remove(i+1);
        // item->m_bIsFolder = true;  // don't treat stacked files as folders
        // the label may be in a different char set from the filename (eg over smb
        // the label is converted from utf8, but the filename is not)
        if (!CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_SHOWEXTENSIONS))
          URIUtils::RemoveExtension(stackName);

        item1->SetLabel(stackName);
        item1->m_dwSize = size;
        break;
      }
    }
    i++;
  }
}

bool CFileItemList::Load(int windowID)
{
  CFile file;
  if (file.Open(GetDiscFileCache(windowID)))
  {
    CArchive ar(&file, CArchive::load);
    ar >> *this;
    CLog::Log(LOGDEBUG,"Loading items: %i, directory: %s sort method: %i, ascending: %s", Size(), CURL::GetRedacted(GetPath()).c_str(), m_sortDescription.sortBy,
      m_sortDescription.sortOrder == SortOrderAscending ? "true" : "false");
    ar.Close();
    file.Close();
    return true;
  }

  return false;
}

bool CFileItemList::Save(int windowID)
{
  int iSize = Size();
  if (iSize <= 0)
    return false;

  CLog::Log(LOGDEBUG,"Saving fileitems [%s]", CURL::GetRedacted(GetPath()).c_str());

  CFile file;
  if (file.OpenForWrite(GetDiscFileCache(windowID), true)) // overwrite always
  {
    CArchive ar(&file, CArchive::store);
    ar << *this;
    CLog::Log(LOGDEBUG,"  -- items: %i, sort method: %i, ascending: %s", iSize, m_sortDescription.sortBy, m_sortDescription.sortOrder == SortOrderAscending ? "true" : "false");
    ar.Close();
    file.Close();
    return true;
  }

  return false;
}

void CFileItemList::RemoveDiscCache(int windowID) const
{
  std::string cacheFile(GetDiscFileCache(windowID));
  if (CFile::Exists(cacheFile))
  {
    CLog::Log(LOGDEBUG,"Clearing cached fileitems [%s]",GetPath().c_str());
    CFile::Delete(cacheFile);
  }
}

std::string CFileItemList::GetDiscFileCache(int windowID) const
{
  std::string strPath(GetPath());
  URIUtils::RemoveSlashAtEnd(strPath);

  Crc32 crc;
  crc.ComputeFromLowerCase(strPath);

  std::string cacheFile;
  if (IsCDDA() || IsOnDVD())
    cacheFile = StringUtils::Format("special://temp/r-%08x.fi", (unsigned __int32)crc);
  else if (IsMusicDb())
    cacheFile = StringUtils::Format("special://temp/mdb-%08x.fi", (unsigned __int32)crc);
  else if (IsVideoDb())
    cacheFile = StringUtils::Format("special://temp/vdb-%08x.fi", (unsigned __int32)crc);
  else if (IsSmartPlayList())
    cacheFile = StringUtils::Format("special://temp/sp-%08x.fi", (unsigned __int32)crc);
  else if (windowID)
    cacheFile = StringUtils::Format("special://temp/%i-%08x.fi", windowID, (unsigned __int32)crc);
  else
    cacheFile = StringUtils::Format("special://temp/%08x.fi", (unsigned __int32)crc);
  return cacheFile;
}

bool CFileItemList::AlwaysCache() const
{
  // some database folders are always cached
  if (IsMusicDb())
    return CMusicDatabaseDirectory::CanCache(GetPath());
  if (IsVideoDb())
    return CVideoDatabaseDirectory::CanCache(GetPath());
  if (IsEPG())
    return true; // always cache
  return false;
}

void CFileItemList::Swap(unsigned int item1, unsigned int item2)
{
  if (item1 != item2 && item1 < m_items.size() && item2 < m_items.size())
    std::swap(m_items[item1], m_items[item2]);
}

bool CFileItemList::UpdateItem(const CFileItem *item)
{
  if (!item)
    return false;

  CSingleLock lock(m_lock);
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->IsSamePath(item))
    {
      pItem->UpdateInfo(*item);
      return true;
    }
  }
  return false;
}

void CFileItemList::AddSortMethod(SortBy sortBy, int buttonLabel, const LABEL_MASKS &labelMasks, SortAttribute sortAttributes /* = SortAttributeNone */)
{
  AddSortMethod(sortBy, sortAttributes, buttonLabel, labelMasks);
}

void CFileItemList::AddSortMethod(SortBy sortBy, SortAttribute sortAttributes, int buttonLabel, const LABEL_MASKS &labelMasks)
{
  SortDescription sorting;
  sorting.sortBy = sortBy;
  sorting.sortAttributes = sortAttributes;

  AddSortMethod(sorting, buttonLabel, labelMasks);
}

void CFileItemList::AddSortMethod(SortDescription sortDescription, int buttonLabel, const LABEL_MASKS &labelMasks)
{
  GUIViewSortDetails sort;
  sort.m_sortDescription = sortDescription;
  sort.m_buttonLabel = buttonLabel;
  sort.m_labelMasks = labelMasks;

  m_sortDetails.push_back(sort);
}

void CFileItemList::SetReplaceListing(bool replace)
{
  m_replaceListing = replace;
}

void CFileItemList::ClearSortState()
{
  m_sortDescription.sortBy = SortByNone;
  m_sortDescription.sortOrder = SortOrderNone;
  m_sortDescription.sortAttributes = SortAttributeNone;
}
