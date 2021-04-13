/**
* This file is part of Volition.

* Volition is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* Volition is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with Volition.  If not, see <https://www.gnu.org/licenses/>.
**/

#ifndef _VL_NODE_JOBS_H_
#define _VL_NODE_JOBS_H_


#include "../libvolition/include/common.h"
#include "../libvolition/include/conation.h"
#include "../libvolition/include/vlthreads.h"
#include <queue>

namespace Jobs
{
	struct Job
	{
		uint64_t JobID; //The number of the job according to this node.
		uint64_t CmdIdent;
		CommandCode CmdCode;
		Conation::ConationQueue Read_Queue, N2N_Queue; //The main thread puts streams intended for a certain job in this queue.
		bool CaptureIncomingStreams : 1;
		bool ReceiveN2N : 1;
		
		//We don't have a write one cuz we just use Main::PushStreamToWriteQueue() to access the primary one.
		
		VLScopedPtr<VLThreads::Thread*> JobThread;

		Job(void) : JobID(), CmdIdent(), CmdCode(), CaptureIncomingStreams(), ReceiveN2N(), JobThread() {}
	};
	
	bool StartJob(const CommandCode NewJob, const Conation::ConationStream *Data);
	void ProcessCompletedJobs(void);
	VLString GetWorkingDirectory(void);
	void ForwardToScriptJobs(Conation::ConationStream *const Stream);
	void ForwardN2N(Conation::ConationStream *const Stream);
	void KillAllJobs(void);
	bool KillJobByID(const uint64_t JobID);
	bool KillJobByCmdCode(const CommandCode CmdCode);
	Conation::ConationStream *BuildRunningJobsReport(const Conation::ConationStream::StreamHeader &Hdr);
}

#endif //_VL_NODE_JOBS_H_
