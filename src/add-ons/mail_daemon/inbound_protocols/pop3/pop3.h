/*
 * Copyright 2007-2011, Haiku Inc. All Rights Reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef POP3_H
#define POP3_H


#include <map>
#include <vector>

#ifdef USE_SSL
#	include <openssl/ssl.h>
#endif

#include <DataIO.h>
#include <List.h>
#include <String.h>
#include <View.h>

#include "MailAddon.h"
#include "MailProtocol.h"
#include "MailSettings.h"
#include <StringList.h>


class POP3Protocol : public InboundProtocol {
public:
								POP3Protocol(BMailAccountSettings* settings);
								~POP3Protocol();

			status_t			Connect();
			status_t			Disconnect();

			status_t			SyncMessages();
			status_t			FetchBody(const entry_ref& ref);
			status_t			DeleteMessage(const entry_ref& ref);

			// pop3 methods
			status_t			Open(const char *server, int port,
									int protocol);
			status_t			Login(const char *uid, const char *password,
									int method);
			status_t			Retrieve(int32 message, BPositionIO *write_to);
			status_t			GetHeader(int32 message, BPositionIO *write_to);
			void				Delete(int32 num);

			size_t				MessageSize(int32 index);
			status_t			Stat();
			int32				Messages(void);
			size_t				MailDropSize(void);
			void				CheckForDeletedMessages();
protected:
			status_t			RetrieveInternal(const char *command,
									int32 message, BPositionIO *writeTo,
									bool showProgress);

			int32				ReceiveLine(BString &line);
			status_t			SendCommand(const char* cmd);
			void				MD5Digest(unsigned char *in, char *out);

private:
			status_t			_UniqueIDs();
			void				_ReadManifest();
			void				_WriteManifest();

			int					fSocket;
			BString				fLog;
			int32				fNumMessages;
			size_t				fMailDropSize;
			BList				fSizes;
			BMessage			fSettings;

			BStringList			fManifest;
			BStringList			fUniqueIDs;

			BString				fDestinationDir;
			int32				fFetchBodyLimit;
#ifdef USE_SSL
			SSL_CTX*			fSSLContext;
			SSL*				fSSL;
			BIO*				fSSLBio;

			bool				fUseSSL;
#endif
};


extern "C" status_t pop3_smtp_auth(BMessage& settings);


#endif	/* POP3_H */
