#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class ezSkeletonAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkeletonAssetDocumentManager, ezAssetDocumentManager);

public:
  ezSkeletonAssetDocumentManager();
  ~ezSkeletonAssetDocumentManager();

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  ezAssetDocumentTypeDescriptor m_DocTypeDesc;
};