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

#include "RGPInterop.h"
#include <QApplication>
#include <QClipboard>

template <>
std::string DoStringise(const RGPCommand &el)
{
  BEGIN_ENUM_STRINGISE(RGPCommand);
  {
    STRINGISE_ENUM_CLASS_NAMED(SetEvent, "set_event");
  }
  END_ENUM_STRINGISE();
}

RGPInterop::RGPInterop(uint32_t version, ICaptureContext &ctx) : m_Ctx(ctx)
{
  m_Version = version;

  if(ctx.APIProps().pipelineType == GraphicsAPI::Vulkan)
  {
    if(version == 1)
    {
      m_EventNames << lit("vkCmdDispatch") << lit("vkCmdDraw") << lit("vkCmdDrawIndexed");
    }
  }
  else if(ctx.APIProps().pipelineType == GraphicsAPI::D3D12)
  {
    // these names must match those in DoStringise(const D3D12Chunk &el) for the chunks

    if(version == 1)
    {
      m_EventNames << lit("ID3D12GraphicsCommandList::Dispatch")
                   << lit("ID3D12GraphicsCommandList::DrawInstanced")
                   << lit("ID3D12GraphicsCommandList::DrawIndexedInstanced");
    }
  }

  // if we don't have any event names, this API doesn't have a mapping or this was an unrecognised
  // version.
  if(m_EventNames.isEmpty())
    return;

  m_Event2RGP.resize(ctx.GetLastDrawcall()->eventId + 1);

  // linearId 0 is invalid, so map to eventId 0.
  // the first real event will be linearId 1
  m_RGP2Event.push_back(0);

  CreateMapping(ctx.CurDrawcalls());
}

void RGPInterop::SelectEvent(uint32_t eventId)
{
  RGPInteropEvent ev = m_Event2RGP[eventId];

  if(ev.rgplinearid == 0)
    return;

  QString encoded = EncodeCommand(RGPCommand::SetEvent, ev.toParams(m_Version));

  // hack - should be sent over IPC
  QApplication::clipboard()->setText(encoded.trimmed());
}

void RGPInterop::EventSelected(RGPInteropEvent event)
{
  uint32_t eventId = m_RGP2Event[event.rgplinearid];

  if(eventId == 0)
  {
    qWarning() << "RGP Event " << event.rgplinearid << event.cmdbufid << event.eventname
               << " did not correspond to a known eventId";
    return;
  }

  const DrawcallDescription *draw = m_Ctx.GetDrawcall(eventId);

  if(draw && QString(draw->name) != event.eventname)
    qWarning() << "Drawcall name mismatch. Expected " << event.eventname << " but got "
               << QString(draw->name);

  m_Ctx.SetEventID({}, eventId, eventId);
}

void RGPInterop::CreateMapping(const rdcarray<DrawcallDescription> &drawcalls)
{
  const SDFile &file = m_Ctx.GetStructuredFile();

  for(const DrawcallDescription &draw : drawcalls)
  {
    for(const APIEvent &ev : draw.events)
    {
      if(ev.chunkIndex == 0 || ev.chunkIndex >= file.chunks.size())
        continue;

      const SDChunk *chunk = file.chunks[ev.chunkIndex];

      if(m_EventNames.contains(chunk->name, Qt::CaseSensitive))
      {
        m_Event2RGP[ev.eventId].rgplinearid = (uint32_t)m_RGP2Event.size();
        if(ev.eventId == draw.eventId)
          m_Event2RGP[ev.eventId].eventname = draw.name;
        else
          m_Event2RGP[ev.eventId].eventname = chunk->name;

        m_RGP2Event.push_back(ev.eventId);
      }
    }

    // if we have children, step into them first before going to our next sibling
    if(!draw.children.empty())
      CreateMapping(draw.children);
  }
}

QString RGPInterop::EncodeCommand(RGPCommand command, QVariantList params)
{
  QString ret;

  QString cmd = ToQStr(command);

  ret += lit("command=%1\n").arg(cmd);

  // iterate params in pair, name and value
  for(int i = 0; i + 1 < params.count(); i += 2)
    ret += QFormatStr("%1.%2=%3\n").arg(cmd).arg(params[i].toString()).arg(params[i + 1].toString());

  ret += lit("endcommand=%1\n").arg(cmd);

  return ret;
}

bool RGPInterop::DecodeCommand(QString command)
{
  QStringList lines = command.split(QLatin1Char('\n'));

  if(lines[0].indexOf(lit("command=")) != 0 || lines.last().indexOf(lit("endcommand=")) != 0)
  {
    qWarning() << "Malformed RGP command:\n" << command;
    return false;
  }

  QString commandName = lines[0].split(QLatin1Char('='))[1];

  if(lines.last().split(QLatin1Char('='))[1] != commandName)
  {
    qWarning() << "Mismatch between command and endcommand:\n" << command;
    return false;
  }

  lines.pop_front();
  lines.pop_back();

  QVariantList params;

  QString prefix = commandName + lit(".");

  for(QString &param : lines)
  {
    int eq = param.indexOf(QLatin1Char('='));

    if(eq < 0)
    {
      qWarning() << "Malformed param: " << param;
      continue;
    }

    QString key = param.left(eq);
    QString value = param.mid(eq + 1);

    if(!key.startsWith(prefix))
    {
      qWarning() << "Malformed param key for" << commandName << ": " << key;
      continue;
    }

    key = key.mid(prefix.count());

    params << key << value;
  }

  if(commandName == ToQStr(RGPCommand::SetEvent))
  {
    RGPInteropEvent ev;
    ev.fromParams(m_Version, params);

    EventSelected(ev);

    return true;
  }
  else
  {
    qWarning() << "Unrecognised command: " << commandName;
  }

  return false;
}