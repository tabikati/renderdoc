/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#pragma once

#include <QDebug>
#include <QStringList>
#include <QVariantList>
#include "Code/QRDUtils.h"
#include "renderdoc_replay.h"

// macros to help define encode/decode functions

#define VARIANT_ENCODE(p) lit(#p), p
#define VARIANT_DECODE(p)                \
  if(paramName == lit(#p))               \
  {                                      \
    p = paramValue.value<decltype(p)>(); \
    continue;                            \
  }

enum class RGPCommand
{
  SetEvent,
};

struct RGPInteropEvent
{
  uint32_t rgplinearid = 0;
  uint32_t cmdbufid = 0;
  QString eventname;

  QVariantList toParams(uint32_t version) const
  {
    return {VARIANT_ENCODE(rgplinearid), VARIANT_ENCODE(cmdbufid), VARIANT_ENCODE(eventname)};
  }

  void fromParams(uint32_t version, QVariantList list)
  {
    for(int i = 0; i + 1 < list.size(); i += 2)
    {
      QString paramName = list[i].toString();
      QVariant paramValue = list[i + 1];
      VARIANT_DECODE(rgplinearid);
      VARIANT_DECODE(cmdbufid);
      VARIANT_DECODE(eventname);

      qWarning() << "Unrecognised param" << paramName;
    }
  }
};

class RGPInterop
{
public:
  RGPInterop(uint32_t version, ICaptureContext &ctx);

  void SelectEvent(uint32_t eventId);
  bool HackProcessInput(QString input) { return DecodeCommand(input); }
  bool Valid() { return !m_Event2RGP.isEmpty(); }
private:
  void CreateMapping(const rdcarray<DrawcallDescription> &drawcalls);

  void EventSelected(RGPInteropEvent event);

  uint32_t m_Version = 0;

  ICaptureContext &m_Ctx;
  QStringList m_EventNames;
  rdcarray<RGPInteropEvent> m_Event2RGP;
  rdcarray<uint32_t> m_RGP2Event;
  QString EncodeCommand(RGPCommand command, QVariantList params);
  bool DecodeCommand(QString command);
};