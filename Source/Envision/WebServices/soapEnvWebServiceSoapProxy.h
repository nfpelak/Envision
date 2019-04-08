/*
This file is part of Envision.

Envision is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Envision is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Envision.  If not, see <http://www.gnu.org/licenses/>

Copywrite 2012 - Oregon State University

*/
/* soapEnvWebServiceSoapProxy.h
   Generated by gSOAP 2.7.16 from EnvWebServices.h
   Copyright(C) 2000-2010, Robert van Engelen, Genivia Inc. All Rights Reserved.
   This part of the software is released under one of the following licenses:
   GPL, the gSOAP public license, or Genivia's license for commercial use.
*/

#ifndef soapEnvWebServiceSoapProxy_H
#define soapEnvWebServiceSoapProxy_H
#include "soapH.h"

class SOAP_CMAC EnvWebServiceSoapProxy : public soap
{ public:
	/// Endpoint URL of service 'EnvWebServiceSoapProxy' (change as needed)
	const char *soap_endpoint;
	/// Constructor
	EnvWebServiceSoapProxy();
	/// Constructor with copy of another engine state
	EnvWebServiceSoapProxy(const struct soap&);
	/// Constructor with engine input+output mode control
	EnvWebServiceSoapProxy(soap_mode iomode);
	/// Constructor with engine input and output mode control
	EnvWebServiceSoapProxy(soap_mode imode, soap_mode omode);
	/// Destructor frees deserialized data
	virtual	~EnvWebServiceSoapProxy();
	/// Initializer used by constructors
	virtual	void EnvWebServiceSoapProxy_init(soap_mode imode, soap_mode omode);
	/// Delete all deserialized data (uses soap_destroy and soap_end)
	virtual	void destroy();
	/// Disables and removes SOAP Header from message
	virtual	void soap_noheader();
	/// Get SOAP Fault structure (NULL when absent)
	virtual	const SOAP_ENV__Fault *soap_fault();
	/// Get SOAP Fault string (NULL when absent)
	virtual	const char *soap_fault_string();
	/// Get SOAP Fault detail as string (NULL when absent)
	virtual	const char *soap_fault_detail();
	/// Force close connection (normally automatic, except for send_X ops)
	virtual	int soap_close_socket();
	/// Print fault
	virtual	void soap_print_fault(FILE*);
#ifndef WITH_LEAN
	/// Print fault to stream
	virtual	void soap_stream_fault(std::ostream&);
	/// Put fault into buffer
	virtual	char *soap_sprint_fault(char *buf, size_t len);
#endif

	/// Web service operation 'GetEnvisionSetupDateTime' (returns error code or SOAP_OK)
	virtual	int GetEnvisionSetupDateTime(_ns1__GetEnvisionSetupDateTime *ns1__GetEnvisionSetupDateTime, _ns1__GetEnvisionSetupDateTimeResponse *ns1__GetEnvisionSetupDateTimeResponse);

	/// Web service operation 'GetEnvisionVersion' (returns error code or SOAP_OK)
	virtual	int GetEnvisionVersion(_ns1__GetEnvisionVersion *ns1__GetEnvisionVersion, _ns1__GetEnvisionVersionResponse *ns1__GetEnvisionVersionResponse);

	/// Web service operation 'GetStudyAreaSetupDateTime' (returns error code or SOAP_OK)
	virtual	int GetStudyAreaSetupDateTime(_ns1__GetStudyAreaSetupDateTime *ns1__GetStudyAreaSetupDateTime, _ns1__GetStudyAreaSetupDateTimeResponse *ns1__GetStudyAreaSetupDateTimeResponse);

	/// Web service operation 'GetStudyAreaVersion' (returns error code or SOAP_OK)
	virtual	int GetStudyAreaVersion(_ns1__GetStudyAreaVersion *ns1__GetStudyAreaVersion, _ns1__GetStudyAreaVersionResponse *ns1__GetStudyAreaVersionResponse);
};
#endif
