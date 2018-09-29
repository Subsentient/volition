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


#include "../libvolition/include/common.h"
#include "../libvolition/include/utils.h"

#include <stdio.h>
#include <stdlib.h>

#include <curl/curl.h>

#include "files.h"
#include "web.h"

//Types

//Prototypes
static size_t Writer(void *InStream, size_t SizePerUnit, size_t NumMembers, FILE *Descriptor);

//Function definitions.
static size_t Writer(void *InStream, size_t SizePerUnit, size_t NumMembers, FILE *Descriptor)
{	
	const size_t MembersWritten = fwrite(InStream, SizePerUnit, NumMembers, Descriptor);
	
	return SizePerUnit * MembersWritten;
}

bool Web::GetHTTP(const char *URL_, VLString *FilePathOut, const int NumAttempts, const char *UserAgent, const char *Referrer) //This is using chars and pointers instead of VLStrings and references so it works in the C API.
{
	int AttemptsRemaining = NumAttempts;
	CURLcode Code;
	CURL *Curl = nullptr;

	VLString URL = URL_;
	
	/*Get the correct URL.*/
	if (!URL.StartsWith("http://") &&
		!URL.StartsWith("https://"))
	{
		URL = VLString("http://") + URL;
	}
	
#ifdef DEBUG
	puts(VLString("Web::GetHTTP(): URL is ") + URL);
#endif

	VLString FilePath = Utils::GetTempDirectory() + Utils::StripPathFromFilename(URL);
	
#ifdef DEBUG
	puts(VLString("Web::GetHTTP(): Temporary file path is ") + FilePath);
#endif

	do
	{
		Files::Delete(FilePath);
		
		FILE *Descriptor = fopen(FilePath, "wb");
		
		if (!Descriptor) return false;
		
		Curl = curl_easy_init(); /*Fire up curl.*/
		
		/*Add the URL*/
		curl_easy_setopt(Curl, CURLOPT_URL, +URL);
		
		curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYPEER, 0L);
		
		/*Follow paths that redirect.*/
		curl_easy_setopt(Curl, CURLOPT_FOLLOWLOCATION, 1L);
		
		/*No progress bar on stdout please.*/
		curl_easy_setopt(Curl, CURLOPT_NOPROGRESS, 1L);
		
		if (UserAgent) curl_easy_setopt(Curl, CURLOPT_USERAGENT, +UserAgent);
		if (Referrer) curl_easy_setopt(Curl, CURLOPT_REFERER, +Referrer);
		
		curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, Writer);
		curl_easy_setopt(Curl, CURLOPT_WRITEDATA, Descriptor);
		curl_easy_setopt(Curl, CURLOPT_VERBOSE, 0L); /*No verbose please.*/
		
		/*Request maximum 5 second timeout for each try of the operation.*/
		curl_easy_setopt(Curl, CURLOPT_CONNECTTIMEOUT, 5L); /*This will not save us in some cases.*/

		//Set the maximum possible max file size.
		curl_easy_setopt(Curl, CURLOPT_MAXFILESIZE_LARGE, (1024ul * 1024ul * 1024ul * 1024ul));
#ifdef DEBUG
		curl_easy_setopt(Curl, CURLOPT_VERBOSE, 1l);
#endif
		Code = curl_easy_perform(Curl);
		
		curl_easy_cleanup(Curl);
		
		fclose(Descriptor);
		
	} while (--AttemptsRemaining, (Code != CURLE_OK && AttemptsRemaining > 0));
	
	if (Code == CURLE_OK)
	{
		*FilePathOut = FilePath;
	}

#ifdef DEBUG
	else puts("Web::GetHTTP(): All attempts failed!");
#endif
	
	return Code == CURLE_OK;
		
}


