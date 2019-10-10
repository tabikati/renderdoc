#pragma once

#include <map>
#include <vector>
#include "api/replay/renderdoc_replay.h"
#include "api/replay/replay_enums.h"
#include "api/replay/data_types.h"
#include "lizard_api.h"

typedef void *(*LizardCreateFunc)();

class ArmGlCounters
{
public:
  ArmGlCounters();
  ~ArmGlCounters();
  bool Init();
  std::vector<GPUCounter> GetPublicCounterIds();
  CounterDescription GetCounterDescription(GPUCounter index);
  void EnableCounters(const std::vector<GPUCounter> &counters);
  void BeginSample(uint32_t eventId);
  void EndSample();
  std::vector<CounterResult> GetCounterData(std::vector<uint32_t> &eventIDs,
                                            const std::vector<GPUCounter> &counters);

private:
  void *m_Module;
  uint32_t m_EventId;
  std::vector<uint32_t> m_EnabledCounters;
  struct LizardApi *m_Api;
  LizardInstance *m_Ctx;
  std::map<GPUCounter, CounterDescription> m_Counters;
  std::vector<GPUCounter> m_CounterIds;
  std::map<uint32_t, std::map<uint32_t, int64_t>> m_CounterData;
  static void ListCounters(LizardInstance *ctx, LizardCounterId id, const char *shortName,
                           void *userData);
};
