#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Math/Vec3.h>
#include <Utilities/UtilitiesDLL.h>

struct ezDynamicTree
{
  struct ezObjectData
  {
    ezInt32 m_iObjectType;
    ezInt32 m_iObjectInstance;
  };

  struct ezMultiMapKey
  {
    ezUInt32 m_uiKey;
    ezUInt32 m_uiCounter;

    ezMultiMapKey()
    {
      m_uiKey = 0;
      m_uiCounter = 0;
    }

    inline bool operator<(const ezMultiMapKey& rhs) const
    {
      if (m_uiKey == rhs.m_uiKey)
        return m_uiCounter < rhs.m_uiCounter;

      return m_uiKey < rhs.m_uiKey;
    }

    inline bool operator==(const ezMultiMapKey& rhs) const { return (m_uiCounter == rhs.m_uiCounter && m_uiKey == rhs.m_uiKey); }
  };
};

using ezDynamicTreeObject = ezMap<ezDynamicTree::ezMultiMapKey, ezDynamicTree::ezObjectData>::Iterator;
using ezDynamicTreeObjectConst = ezMap<ezDynamicTree::ezMultiMapKey, ezDynamicTree::ezObjectData>::ConstIterator;

/// \brief Callback type for object queries. Return "false" to abort a search (e.g. when the desired element has been found).
using EZ_VISIBLE_OBJ_CALLBACK = bool (*)(void*, ezDynamicTreeObjectConst);

class ezDynamicOctree;
class ezDynamicQuadtree;
