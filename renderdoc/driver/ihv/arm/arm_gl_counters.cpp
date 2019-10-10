#include "arm_gl_counters.h"
#include <dlfcn.h>    //dlopen
#include "android/android.h"
#include "core/plugins.h"
#include "os/os_specific.h"

ArmGlCounters::ArmGlCounters()
{
}

ArmGlCounters::~ArmGlCounters()
{
  if(m_Module)
    dlclose(m_Module);
}

void ArmGlCounters::ListCounters(LizardInstance *ctx, LizardCounterId id, const char *shortName,
                                 void *userData)
{
  printf("%d. %s\n", id, shortName);
  ArmGlCounters *armGlCounters = (ArmGlCounters *)userData;
  const char *title = armGlCounters->m_Api->GetCounterStringAttribute(ctx, id, LZD_COUNTER_TITLE);
  const char *name = armGlCounters->m_Api->GetCounterStringAttribute(ctx, id, LZD_COUNTER_NAME);
  CounterDescription desc;
  desc.name = std::string(title) + " " + std::string(name);
  desc.counter = GPUCounter((int)GPUCounter::FirstArm + id);
  desc.category = armGlCounters->m_Api->GetCounterStringAttribute(ctx, id, LZD_COUNTER_CATEGORY);
  desc.description =
      armGlCounters->m_Api->GetCounterStringAttribute(ctx, id, LZD_COUNTER_DESCRIPTION);
  desc.description += " (" + std::string(shortName) + ")";
  desc.resultType = CompType::UInt;
  armGlCounters->m_Counters[desc.counter] = desc;
  armGlCounters->m_CounterIds.push_back(desc.counter);
}

bool ArmGlCounters::Init()
{
#if ENABLED(RDOC_ANDROID)
  std::string path = "liblizard.so";
  m_Module = dlopen(path.c_str(), RTLD_NOW);
  if(m_Module == NULL)
  {
    path = "/data/data/org.renderdoc.renderdoccmd.arm64/liblizard.so";
  }
  m_Module = dlopen(path.c_str(), RTLD_NOW);
#else
  std::string path = "liblizard.so";    // TODO
  m_Module = dlopen(path.c_str(), RTLD_NOW);
#endif

  if(m_Module == NULL)
  {
    RDCLOG("Failed to load lizard: %s", dlerror());
    return false;
  }

  LZD_LoadApi_PFN loadApi = (LZD_LoadApi_PFN)Process::GetFunctionAddress(m_Module, "LoadApi");

  if(loadApi(&m_Api) != LZD_OK)
  {
    RDCLOG("Failed to load lizard api");
    return false;
  }

  m_Ctx = m_Api->Init("127.0.0.1", 8080);
  if(!m_Ctx)
  {
    RDCLOG("Failed to load gatord");
    return false;
  }

  uint32_t count = m_Api->EnumerateCounters(m_Ctx, ListCounters, this);
  if(count == 0)
    return false;

  return true;
}

std::vector<GPUCounter> ArmGlCounters::GetPublicCounterIds()
{
  return m_CounterIds;
}

CounterDescription ArmGlCounters::GetCounterDescription(GPUCounter index)
{
  return m_Counters.at(index);
}

void ArmGlCounters::EnableCounters(const std::vector<GPUCounter> &counters)
{
  m_EnabledCounters.clear();
  for(size_t i = 0; i < counters.size(); i++)
  {
    GPUCounter counter = counters.at(i);
    uint32_t id = (uint32_t)counter - (uint32_t)GPUCounter::FirstArm;
    m_Api->EnableCounter(m_Ctx, id);
    m_EnabledCounters.push_back(id);
  }
}

void ArmGlCounters::BeginSample(uint32_t eventId)
{
  m_EventId = eventId;
  m_Api->StartCapture(m_Ctx);
}

void ArmGlCounters::EndSample()
{
  m_Api->StopCapture(m_Ctx);
  for(size_t i = 0; i < m_EnabledCounters.size(); i++)
  {
    int64_t data = m_Api->ReadCounter(m_Ctx, m_EnabledCounters.at(i));
    m_CounterData[m_EventId][m_EnabledCounters.at(i)] = data;
  }
}

std::vector<CounterResult> ArmGlCounters::GetCounterData(std::vector<uint32_t> &eventIDs,
                                                         const std::vector<GPUCounter> &counters)
{
  std::vector<CounterResult> result;
  for(size_t i = 0; i < eventIDs.size(); i++)
  {
    uint32_t eventId = eventIDs.at(i);
    for(size_t j = 0; j < counters.size(); j++)
    {
      GPUCounter counter = counters.at(j);
      uint32_t counterId = (uint32_t)counter - (uint32_t)GPUCounter::FirstArm;
      uint64_t data = m_CounterData.at(eventId).at(counterId);
      result.push_back(CounterResult(eventId, counter, data));
    }
  }
  return result;
}
