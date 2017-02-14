/* Copyright (C) 2006-2017 PUC-Rio/Laboratorio TeleMidia

This file is part of Ginga (Ginga-NCL).

Ginga is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Ginga is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.

You should have received a copy of the GNU General Public License
along with Ginga.  If not, see <http://www.gnu.org/licenses/>.  */

#include "ginga.h"
#include "BaseDeviceDomain.h"
#include "ActiveDeviceService.h"

#include "util/functions.h"
using namespace ::ginga::util;

#include "system/SystemCompat.h"
using namespace ::ginga::system;

GINGA_PRAGMA_DIAG_IGNORE (-Wconversion)

GINGA_MULTIDEV_BEGIN

BaseDeviceDomain::BaseDeviceDomain (bool useMulticast, int srvPort)
    : DeviceDomain (useMulticast, srvPort)
{
  timerCount = 0;

  Thread::mutexInit (&pMutex, false);
  lastMediaContentTask.size = 0;

  deviceClass = CT_BASE;
  deviceService = new BaseDeviceService ();
}

BaseDeviceDomain::~BaseDeviceDomain () { Thread::mutexDestroy (&pMutex); }

bool
BaseDeviceDomain::taskRequest (int destDevClass, char *data, int taskSize)
{
  switch (destDevClass)
    {
    case CT_ACTIVE:
      return activeTaskRequest (data, taskSize);

    default:
      return false;
    }
}

bool
BaseDeviceDomain::activeTaskRequest (arg_unused (char *data), arg_unused (int taskSize))
{
  return true;
}

void
BaseDeviceDomain::receiveConnectionRequest (char *task)
{
  int reqDevClass, width, height, srvPort;
  clog << "BaseDeviceDomain::receiveConnectionRequest " << endl;

  reqDevClass = (int)(unsigned char)task[0];
  width = ((((unsigned char)task[1]) & 0xFF)
           | ((((unsigned char)task[2]) << 8) & 0xFF00));

  height = ((((unsigned char)task[3]) & 0xFF)
            | ((((unsigned char)task[4]) << 8) & 0xFF00));

  srvPort = ((((unsigned char)task[5]) & 0xFF)
             | ((((unsigned char)task[6]) << 8) & 0xFF00));

  if (addDevice (reqDevClass, width, height, srvPort))
    {
      schedDevClass = reqDevClass;
      schedulePost = FT_ANSWERTOREQUEST;
    }
  else
    {
      clog << "BaseDeviceDomain::receiveConnectionRequest can't add ";
      clog << "device" << endl;
    }
}

void
BaseDeviceDomain::postAnswerTask (int reqDeviceClass, int answer)
{
  int taskSize, pos, broadcastPort;
  int answerPayloadSize = 11;
  char *task;
  char *streamSourceIP;

  clog << "BaseDeviceDomain::postAnswerTask answer '";
  clog << answer << "' for device '";
  clog << sourceIp << "', which means '" << getStrIP (sourceIp);
  clog << "', of class '" << reqDeviceClass << endl;

  // prepare frame
  task = mountFrame (myIP, reqDeviceClass, FT_ANSWERTOREQUEST,
                     answerPayloadSize);

  // fill prepared frame with answer payload
  pos = HEADER_SIZE;

  streamSourceIP = getStreamFromUInt (sourceIp);
  memcpy (task + pos, streamSourceIP, 4);
  delete[] streamSourceIP;

  pos = pos + 4;
  task[pos] = answer;

  pos++;
  memcpy (task + pos, task, 4);

  broadcastPort = broadcastService->getServicePort ();
  pos = pos + 4;
  task[pos] = broadcastPort & 0xFF;
  task[pos + 1] = (broadcastPort & 0xFF00) >> 8;

  taskSize = HEADER_SIZE + answerPayloadSize;
  clog << "BaseDeviceDomain::answer with taskSize=" << taskSize << endl;
  broadcastTaskRequest (task, taskSize);
  clog << "BaseDeviceDomain::called broadcastTaskRequest()" << endl;
}
/*
        void BaseDeviceDomain::postNclMetadata(
                        int devClass, vector<StreamData*>* streams) {
                char* task;
                StreamData* streamData;
                vector<StreamData*>::iterator i;

                if (devClass != DeviceDomain::CT_BASE &&
                                devClass != DeviceDomain::CT_ACTIVE) {
                        return;
                }

                i = streams->begin();
                while (i != streams->end()) {
                        streamData = *i;

                        //prepare frame
                        task = mountFrame(
                                        myIP, devClass, FT_MEDIACONTENT,
   streamData->size);

                        memcpy(task + HEADER_SIZE, streamData->stream,
   streamData->size);
                        taskRequest(devClass, task, streamData->size +
   HEADER_SIZE);

                        delete streamData;
                        streams->erase(i);
                        i = streams->begin();
                }
        }*/

bool
BaseDeviceDomain::postMediaContentTask (int destDevClass, const string &url)
{
  FILE *fd;
  int fileSize;

  clog << "BaseDeviceDomain::postMediaContentTask file '";
  clog << url << "' to devices of class '" << destDevClass << "'";
  clog << endl;

  if (destDevClass == 0)
    {
      return false;
    }

  if (!deviceService->hasDevices ())
    {
      clog << "BaseDeviceDomain::postMediaContentTask no devs found!";
      clog << endl;
      return false;
    }

  fd = fopen (url.c_str (), "rb");
  if (fd != NULL)
    {
      fseek (fd, 0L, SEEK_END);
      fileSize = ftell (fd);
      if (fileSize > 0)
        {
          fclose (fd);

          if (fileSize > MAX_FRAME_SIZE)
            {
              // TODO: frame segmentation support
              clog << "BaseDeviceDomain::postMediaContentTask ";
              clog << "Warning! Can't post a frame that the ";
              clog << "network doesn't support (" << fileSize;
              clog << ")" << endl;
              return false;
            }

          fd = fopen (url.c_str (), "rb");
          if (fd == NULL)
            {
              clog << "BaseDeviceDomain::postMediaContentTask ";
              clog << "Warning! Can't re-open file '" << url;
              clog << "'" << endl;
              return false;
            }
        }
      else
        {
          clog << "BaseDeviceDomain::postMediaContentTask ";
          clog << "Warning! Can't seek file '" << url << "'" << endl;
        }

      fclose (fd);
    }
  else
    {
      clog << "BaseDeviceDomain::postMediaContentTask ";
      clog << "Warning! Can't open file '" << url << "'" << endl;
    }

  return true;
}

bool
BaseDeviceDomain::receiveEventTask (char *task)
{
  clog << "BaseDeviceDomain::receiveEventTask destClass '";
  clog << destClass << "'" << endl;
  return deviceService->receiveEvent (sourceIp, frameType, task,
                                      this->frameSize);
}

void
BaseDeviceDomain::setDeviceInfo (int width, int height,
                                 const string &base_device_ncl_path)
{
  DeviceDomain::setDeviceInfo (width, height, base_device_ncl_path);
  connected = true;
}

bool
BaseDeviceDomain::runControlTask ()
{
  char *task = NULL;

  // clog << "BaseDeviceDomain::runControlTask :: " << endl;

  if (taskIndicationFlag)
    {
      task = taskReceive ();
      if (task == NULL)
        {
          taskIndicationFlag = false;
          return false;
        }
      /*
                              if (myIP == sourceIp) {
                                      clog << "DeviceDomain::runControlTask
         got
         my own task ";
                                      clog << "(size = '" << frameSize <<
         "')"
         << endl;

                                      delete[] task;
                                      task = NULL;
                                      taskIndicationFlag = false;
                                      return false;
                              }
      */
      if (destClass != deviceClass)
        {
          clog << "DeviceDomain::runControlTask Task isn't for me!";
          clog << endl;

          delete[] task;
          task = NULL;
          taskIndicationFlag = false;
          return false;
        }

      if (frameSize + HEADER_SIZE != (unsigned int) bytesRecv)
        {
          delete[] task;
          task = NULL;
          taskIndicationFlag = false;
          clog << "DeviceDomain::runControlTask Warning! Invalid task ";
          clog << "size '" << frameSize + HEADER_SIZE << "' (received '";
          clog << bytesRecv << "'" << endl;
          return false;
        }

      clog << "BaseDeviceDomain::runControlTask frame type '";
      clog << frameType << "'" << endl;

      switch (frameType)
        {
        case FT_CONNECTIONREQUEST:
          // TODO: fix passive connection request - use 7 bytes as well (add
          // port field)
          if ((frameSize != 5) && (frameSize != 7))
            {
              clog << "25BaseDeviceDomain::runControlTask Warning!";
              clog << "received a connection request frame with";
              clog << " wrong size: '" << frameSize << "'" << endl;
            }
          else
            {
              clog << "calling receiveConnectionRequest" << endl;
              receiveConnectionRequest (task);
            }
          break;

        case FT_KEEPALIVE:
          clog << "BaseDeviceDomain::runControlTask KEEPALIVE";
          clog << endl;
          break;

        // what?
        default:
          clog << "DeviceDomain::runControlTask frame type ";
          clog << "WHAT?" << endl;
          delete[] task;
          task = NULL;
          taskIndicationFlag = false;
          return false;
        }

      delete[] task;
      task = NULL;
    }
  else
    {
      clog << "DeviceDomain::runControlTask Warning! Trying to control";
      clog << "a non indicated task." << endl;
    }

  taskIndicationFlag = false;
  return true;
}

bool
BaseDeviceDomain::runDataTask ()
{
  char *task;

  task = taskReceive ();
  if (task == NULL)
    {
      return false;
    }
  /*
                  if (myIP == sourceIp) {
                          clog << "BaseDeviceDomain::runDataTask receiving
     my
     own task";
                          clog << endl;

                          delete[] task;
                          return false;
                  }
  */
  if (destClass != deviceClass)
    {
      clog << "BaseDeviceDomain::runDataTask";
      clog << " should never reach here (receiving wrong destination";
      clog << " class '" << destClass << "')";
      clog << endl;

      delete[] task;
      return false;
    }

  if (frameSize + HEADER_SIZE != (unsigned int) bytesRecv)
    {
      delete[] task;
      clog << "BaseDeviceDomain::runDataTask Warning! wrong ";
      clog << "frameSize '" << bytesRecv << "'" << endl;
      return false;
    }

  switch (frameType)
    {
    case FT_SELECTIONEVENT:
    case FT_ATTRIBUTIONEVENT:
    case FT_PRESENTATIONEVENT:
      receiveEventTask (task);
      break;

    // what?
    default:
      clog << "BaseDeviceDomain::runDataTask frame type ";
      clog << "WHAT?" << endl;
      delete[] task;
      return false;
    }

  delete[] task;
  return true;
}

void
BaseDeviceDomain::checkDomainTasks ()
{
  DeviceDomain::checkDomainTasks ();
  if (newAnswerPosted)
    {
      newAnswerPosted = false;
      deviceService->newDeviceConnected (sourceIp);
    }
}

GINGA_MULTIDEV_END