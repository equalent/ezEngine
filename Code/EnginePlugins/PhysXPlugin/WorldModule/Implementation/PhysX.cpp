#include <PhysXPlugin/PhysXPluginPCH.h>

#include <PhysXPlugin/Utilities/PxUserData.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezPxSteppingMode, 1)
  EZ_ENUM_CONSTANTS(ezPxSteppingMode::Variable, ezPxSteppingMode::Fixed, ezPxSteppingMode::SemiFixed)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezOnPhysXContact, 1)
  EZ_BITFLAGS_CONSTANT(ezOnPhysXContact::SendReportMsg),
  EZ_BITFLAGS_CONSTANT(ezOnPhysXContact::ImpactReactions),
  EZ_BITFLAGS_CONSTANT(ezOnPhysXContact::SlideReactions),
  EZ_BITFLAGS_CONSTANT(ezOnPhysXContact::RollXReactions),
  EZ_BITFLAGS_CONSTANT(ezOnPhysXContact::RollYReactions),
  EZ_BITFLAGS_CONSTANT(ezOnPhysXContact::RollZReactions),
EZ_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

//////////////////////////////////////////////////////////////////////////

void ezPxErrorCallback::reportError(PxErrorCode::Enum code, const char* szMessage, const char* szFile, int iLine)
{
  switch (code)
  {
    case PxErrorCode::eABORT:
      ezLog::Error("PhysX: {0}", szMessage);
      break;
    case PxErrorCode::eDEBUG_INFO:
      ezLog::Dev("PhysX: {0}", szMessage);
      break;
    case PxErrorCode::eDEBUG_WARNING:
      ezLog::Warning("PhysX: {0}", szMessage);
      break;
    case PxErrorCode::eINTERNAL_ERROR:
      ezLog::Error("PhysX Internal: {0}", szMessage);
      break;
    case PxErrorCode::eINVALID_OPERATION:
      ezLog::Error("PhysX Invalid Operation: {0}", szMessage);
      break;
    case PxErrorCode::eINVALID_PARAMETER:
      if (ezStringUtils::IsEqual(szMessage, "PxScene::raycast(): pose is not valid.") ||
          ezStringUtils::IsEqual(szMessage, "Gu::overlap(): pose1 is not valid.") ||
          ezStringUtils::IsEqual(szMessage, "PxScene::sweep(): pose1 is not valid."))
      {
        // ezLog::Warning("Invalid pose");
        return;
      }
      ezLog::Error("PhysX Invalid Parameter: {0}", szMessage);
      break;
    case PxErrorCode::eOUT_OF_MEMORY:
      ezLog::Error("PhysX Out-of-Memory: {0}", szMessage);
      break;
    case PxErrorCode::ePERF_WARNING:
      ezLog::Warning("PhysX Performance: {0}", szMessage);
      break;

    default:
      ezLog::Error("PhysX: Unknown error type '{0}': {1}", code, szMessage);
      break;
  }
}

//////////////////////////////////////////////////////////////////////////

// #define EZ_PX_DETAILED_MEMORY_STATS EZ_ON
#define EZ_PX_DETAILED_MEMORY_STATS EZ_OFF

ezPxAllocatorCallback::ezPxAllocatorCallback()
  : m_Allocator("PhysX", ezFoundation::GetAlignedAllocator())
{
}

void* ezPxAllocatorCallback::allocate(size_t uiSize, const char* szTypeName, const char* szFilename, int iLine)
{
  void* pPtr = m_Allocator.Allocate(uiSize, 16);

#if EZ_ENABLED(EZ_PX_DETAILED_MEMORY_STATS)
  ezStringBuilder s;
  s.Set(typeName, " - ", filename);
  m_Allocations[pPtr] = s;
#endif

  return pPtr;
}

void ezPxAllocatorCallback::deallocate(void* pPtr)
{
  if (pPtr == nullptr)
    return;

#if EZ_ENABLED(EZ_PX_DETAILED_MEMORY_STATS)
  m_Allocations.Remove(ptr);
#endif

  m_Allocator.Deallocate(pPtr);
}

void ezPxAllocatorCallback::VerifyAllocations()
{
#if EZ_ENABLED(EZ_PX_DETAILED_MEMORY_STATS)
  EZ_ASSERT_DEV(m_Allocations.IsEmpty(), "There are {0} unfreed allocations", m_Allocations.GetCount());

  for (auto it = m_Allocations.GetIterator(); it.IsValid(); ++it)
  {
    const char* s = it.Value().GetData();
    ezLog::Info(s);
  }
#endif
}

//////////////////////////////////////////////////////////////////////////

PxQueryHitType::Enum ezPxQueryFilter::preFilter(const PxFilterData& filterData, const PxShape* pShape, const PxRigidActor* pActor, PxHitFlags& ref_queryFlags)
{
  if (pShape->getFlags().isSet(PxShapeFlag::eTRIGGER_SHAPE))
  {
    // ignore all trigger shapes
    return PxQueryHitType::eNONE;
  }

  if (m_bIncludeQueryShapes == false && pActor->getActorFlags().isSet(PxActorFlag::eDISABLE_SIMULATION))
  {
    // ignore all shapes that don't participate in the simulation
    return PxQueryHitType::eNONE;
  }

  const PxFilterData& shapeFilterData = pShape->getQueryFilterData();

  ref_queryFlags = (PxHitFlags)0;

  // shape should be ignored
  if (shapeFilterData.word2 == filterData.word2)
  {
    return PxQueryHitType::eNONE;
  }

  // trigger the contact callback for pairs (A,B) where
  // the filter mask of A contains the ID of B and vice versa.
  if ((filterData.word0 & shapeFilterData.word1) || (shapeFilterData.word0 & filterData.word1))
  {
    ref_queryFlags |= PxHitFlag::eDEFAULT;
    return PxQueryHitType::eBLOCK;
  }

  return PxQueryHitType::eNONE;
}

PxQueryHitType::Enum ezPxQueryFilter::postFilter(const PxFilterData& filterData, const PxQueryHit& hit)
{
  const PxLocationHit& locHit = static_cast<const PxLocationHit&>(hit);

  if (locHit.hadInitialOverlap())
    return PxQueryHitType::eNONE;

  return PxQueryHitType::eBLOCK;
}

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_SINGLETON(ezPhysX);
static ezPhysX g_PhysXSingleton;

ezPhysX::ezPhysX()
  : m_SingletonRegistrar(this)
{
  m_bInitialized = false;

  m_pFoundation = nullptr;
  m_pAllocatorCallback = nullptr;
  m_pPhysX = nullptr;
  m_pDefaultMaterial = nullptr;
  m_pPvdConnection = nullptr;
}

ezPhysX::~ezPhysX() = default;

void ezPhysX::Startup()
{
  if (m_bInitialized)
    return;

  m_bInitialized = true;

  m_pAllocatorCallback = EZ_DEFAULT_NEW(ezPxAllocatorCallback);

  m_pFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, *m_pAllocatorCallback, m_ErrorCallback);
  EZ_ASSERT_DEV(m_pFoundation != nullptr, "Initializing PhysX failed");

#if EZ_ENABLED(EZ_PX_DETAILED_MEMORY_STATS)
  m_pFoundation->setReportAllocationNames(true);
#else
  m_pFoundation->setReportAllocationNames(false);
#endif

  bool bRecordMemoryAllocations = false;
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  bRecordMemoryAllocations = true;
#endif

  m_pPvdConnection = PxCreatePvd(*m_pFoundation);

  m_pPhysX = PxCreatePhysics(PX_PHYSICS_VERSION, *m_pFoundation, PxTolerancesScale(), bRecordMemoryAllocations, m_pPvdConnection);
  EZ_ASSERT_DEV(m_pPhysX != nullptr, "Initializing PhysX API failed");

  PxInitExtensions(*m_pPhysX, m_pPvdConnection);

  m_pDefaultMaterial = m_pPhysX->createMaterial(0.6f, 0.4f, 0.25f);

  ezSurfaceResource::s_Events.AddEventHandler(ezMakeDelegate(&ezPhysX::SurfaceResourceEventHandler, this));
}

void ezPhysX::Shutdown()
{
  if (!m_bInitialized)
    return;

  m_bInitialized = false;

  ezSurfaceResource::s_Events.RemoveEventHandler(ezMakeDelegate(&ezPhysX::SurfaceResourceEventHandler, this));

  if (m_pDefaultMaterial != nullptr)
  {
    m_pDefaultMaterial->release();
    m_pDefaultMaterial = nullptr;
  }

  if (m_pPhysX != nullptr)
  {
    m_pPhysX->release();
    m_pPhysX = nullptr;
  }

  ShutdownVDB();

  PxCloseExtensions();

  if (m_pFoundation != nullptr)
  {
    m_pFoundation->release();
    m_pFoundation = nullptr;
  }

  m_pAllocatorCallback->VerifyAllocations();
  EZ_DEFAULT_DELETE(m_pAllocatorCallback);
}

void ezPhysX::StartupVDB()
{
  // disconnect if we already have a connection
  // this check does not work when the PVD app was closed, instead just always call disconnect
  // if (m_PvdConnection->isConnected(false))
  {
    m_pPvdConnection->disconnect();
  }

  PxPvdTransport* pTransport = m_pPvdConnection->getTransport();
  if (pTransport != nullptr)
  {
    pTransport->release();
  }


  // setup connection parameters
  const char* pvd_host_ip = "127.0.0.1"; // IP of the PC which is running PVD
  int port = 5425;                       // TCP port to connect to, where PVD is listening

  // timeout in milliseconds to wait for PVD to respond, consoles and remote PCs need a higher timeout.
  // for some reason having a timeout of 100ms will block indefinitely when a second process tries to connect and should fail
  unsigned int timeout = 10;

  pTransport = PxDefaultPvdSocketTransportCreate(pvd_host_ip, port, timeout);
  if (m_pPvdConnection->connect(*pTransport, PxPvdInstrumentationFlag::eALL))
  {
    ezLog::Success("Connected to the PhysX Visual Debugger");
  }
}

void ezPhysX::ShutdownVDB()
{
  if (m_pPvdConnection == nullptr)
    return;

  m_pPvdConnection->disconnect();

  PxPvdTransport* pTransport = m_pPvdConnection->getTransport();
  if (pTransport != nullptr)
  {
    pTransport->release();
  }

  m_pPvdConnection->release();
  m_pPvdConnection = nullptr;
}

void ezPhysX::LoadCollisionFilters()
{
  EZ_LOG_BLOCK("ezPhysX::LoadCollisionFilters");

  if (m_CollisionFilterConfig.Load().Failed())
  {
    ezLog::Info("Collision filter config file could not be found ('{}'). Using default values.", ezCollisionFilterConfig::s_sConfigFile);

    // setup some default config

    m_CollisionFilterConfig.SetGroupName(0, "Default");
    m_CollisionFilterConfig.EnableCollision(0, 0);
  }
}

ezAllocatorBase* ezPhysX::GetAllocator()
{
  return &(m_pAllocatorCallback->m_Allocator);
}

PxFilterData ezPhysX::CreateFilterData(ezUInt32 uiCollisionLayer, ezUInt32 uiShapeId /*= ezInvalidIndex*/, ezBitflags<ezOnPhysXContact> flags /*= ezOnPhysXContact::None*/)
{
  PxFilterData filter;
  filter.word0 = EZ_BIT(uiCollisionLayer);
  filter.word1 = ezPhysX::GetSingleton()->GetCollisionFilterConfig().GetFilterMask(uiCollisionLayer);
  filter.word2 = uiShapeId;
  filter.word3 = flags.GetValue();

  return filter;
}

void ezPhysX::SurfaceResourceEventHandler(const ezSurfaceResourceEvent& e)
{
  if (e.m_Type == ezSurfaceResourceEvent::Type::Created)
  {
    const auto& desc = e.m_pSurface->GetDescriptor();

    PxMaterial* pMaterial = m_pPhysX->createMaterial(desc.m_fPhysicsFrictionStatic, desc.m_fPhysicsFrictionDynamic, desc.m_fPhysicsRestitution);
    ezPxUserData* pxUserData = EZ_DEFAULT_NEW(ezPxUserData);
    pxUserData->Init(e.m_pSurface);
    pMaterial->userData = pxUserData;

    e.m_pSurface->m_pPhysicsMaterialPhysX = pMaterial;
  }
  else if (e.m_Type == ezSurfaceResourceEvent::Type::Destroyed)
  {
    if (e.m_pSurface->m_pPhysicsMaterialPhysX != nullptr)
    {
      PxMaterial* pMaterial = static_cast<PxMaterial*>(e.m_pSurface->m_pPhysicsMaterialPhysX);

      ezPxUserData* pUserData = static_cast<ezPxUserData*>(pMaterial->userData);
      EZ_DEFAULT_DELETE(pUserData);

      pMaterial->release();

      e.m_pSurface->m_pPhysicsMaterialPhysX = nullptr;
    }
  }
}



EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_WorldModule_Implementation_PhysX);