#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

void ezTypeScriptBinding::RemoveAutoGeneratedCode(ezStringBuilder& content)
{
  ezStringBuilder sFinal;

  {
    if (const char* szAutoGen = content.FindSubString("// AUTO-GENERATED"))
    {
      sFinal.SetSubString_FromTo(content.GetData(), szAutoGen);
    }
    else
    {
      sFinal = content;
    }

    sFinal.Trim(" \t\n\r");
    sFinal.Append("\n\n");
  }

  content = sFinal;
}

void ezTypeScriptBinding::AppendToTextFile(ezStringBuilder& content, ezStringView text)
{
  content.Append("\n\n// AUTO-GENERATED\n");
  content.Append(text);
  content.Append("\n");
}

void ezTypeScriptBinding::GenerateEnumsFile(const char* szFile, const ezSet<const ezRTTI*>& items)
{
  ezArrayMap<ezString, const ezRTTI*> sortedItems;
  ezStringBuilder sType, sName;

  for (const ezRTTI* pRtti : items)
  {
    if (pRtti->GetProperties().IsEmpty())
      continue;

    sName = pRtti->GetTypeName();
    sName.TrimWordStart("ez");

    sortedItems.Insert(sName, pRtti);
  }

  sortedItems.Sort();

  ezStringBuilder sFileContent = "// AUTO-GENERATED FILE\n\n";

  for (ezUInt32 i = 0; i < sortedItems.GetCount(); ++i)
  {
    const ezRTTI* pRtti = sortedItems.GetValue(i);

    sType.Format("export enum {0} { ", sortedItems.GetKey(i));

    for (auto pProp : pRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == ezPropertyCategory::Constant)
      {
        const ezVariant value = static_cast<const ezAbstractConstantProperty*>(pProp)->GetConstant();
        const ezInt64 iValue = value.ConvertTo<ezInt64>();

        sType.AppendFormat(" {0} = {1},", ezStringUtils::FindLastSubString(pProp->GetPropertyName(), "::") + 2, iValue);
      }
    }

    sType.Shrink(0, 1);
    sType.Append(" }\n");

    sFileContent.Append(sType.GetView());
  }

  ezDeferredFileWriter file;
  file.SetOutput(szFile, true);

  file.WriteBytes(sFileContent.GetData(), sFileContent.GetElementCount()).IgnoreResult();

  if (file.Close().Failed())
  {
    ezLog::Error("Failed to write file '{}'", szFile);
    return;
  }
}

void ezTypeScriptBinding::InjectEnumImportExport(ezStringBuilder& content, const char* szEnumFile)
{
  ezStringBuilder sImportExport, sTypeName;

  sImportExport.Format("import __AllEnums = require(\"{}\")\n", szEnumFile);

  ezDynamicArray<const ezRTTI*> sorted;
  sorted.Reserve(s_RequiredEnums.GetCount());
  for (const ezRTTI* pRtti : s_RequiredEnums)
  {
    sorted.PushBack(pRtti);
  }
  sorted.Sort([](const ezRTTI* p1, const ezRTTI* p2) -> bool { return p1->GetTypeName().Compare(p2->GetTypeName()) < 0; });

  for (const ezRTTI* pRtti : sorted)
  {
    GetTsName(pRtti, sTypeName);
    sImportExport.AppendFormat("export import {0} = __AllEnums.{0};\n", sTypeName);
  }

  AppendToTextFile(content, sImportExport);
}

void ezTypeScriptBinding::InjectFlagsImportExport(ezStringBuilder& content, const char* szFlagsFile)
{
  ezStringBuilder sImportExport, sTypeName;

  sImportExport.Format("import __AllFlags = require(\"{}\")\n", szFlagsFile);

  ezDynamicArray<const ezRTTI*> sorted;
  sorted.Reserve(s_RequiredFlags.GetCount());
  for (const ezRTTI* pRtti : s_RequiredFlags)
  {
    sorted.PushBack(pRtti);
  }
  sorted.Sort([](const ezRTTI* p1, const ezRTTI* p2) -> bool { return p1->GetTypeName().Compare(p2->GetTypeName()) < 0; });

  for (const ezRTTI* pRtti : sorted)
  {
    GetTsName(pRtti, sTypeName);
    sImportExport.AppendFormat("export import {0} = __AllFlags.{0};\n", sTypeName);
  }

  AppendToTextFile(content, sImportExport);
}