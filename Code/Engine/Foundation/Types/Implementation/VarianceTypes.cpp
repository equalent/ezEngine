#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVarianceTypeBase, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Variance", m_fVariance)
  }
    EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVarianceTypeFloat, ezVarianceTypeBase, 1, ezRTTIDefaultAllocator<ezVarianceTypeFloat>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Value", m_Value)
  }
    EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVarianceTypeTime, ezVarianceTypeBase, 1, ezRTTIDefaultAllocator<ezVarianceTypeTime>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Value", m_Value)
  }
    EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVarianceTypeAngle, ezVarianceTypeBase, 1, ezRTTIDefaultAllocator<ezVarianceTypeAngle>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Value", m_Value)
  }
    EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezVarianceTypeFloat);
EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezVarianceTypeTime);
EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezVarianceTypeAngle);

EZ_STATICLINK_FILE(Foundation, Foundation_Types_Implementation_VarianceTypes);
