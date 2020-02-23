/*
 *   Copyright (C) 2020 by Thomas A. Early N7TAE
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include <cstring>

#include "TLSServer.h"

CTLSServer::~CTLSServer()
{
	CloseClient();
	if (m_sock >= 0)
		    close(m_sock);
    if (m_ctx)
		SSL_CTX_free(m_ctx);
}

bool CTLSServer::CreateContext(const SSL_METHOD *method)
{
	m_ctx = SSL_CTX_new(method);
    if (!m_ctx) {
		fprintf(stderr, "Unable to create SSL context");
		ERR_print_errors_fp(stderr);
		return true;
    }
	return false;
}

// Server Class Definitions
#ifndef CFG_DIR
#define CFG_DIR "/usr/local/etc"
#endif

bool CTLSServer::OpenSocket(const std::string &password, const std::string &address, unsigned short port)
{
	m_password.assign(password);
	m_address.assign(address);
	m_port = port;

	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();

	const SSL_METHOD *method = TLS_server_method();
	if (NULL == method) {
		perror("Can't set SSL method");
		return true;
	}

	if (CreateContext(method))
		return true;

	if (0 == SSL_CTX_set_min_proto_version(m_ctx, TLS1_2_VERSION)) {
		perror("Can't sent minimum version");
		return true;
	}

	std::string path(CFG_DIR);
	std::string file(path+"/sgs.crt");
	SSL_CTX_set_ecdh_auto(ctx, 1);
	if (0 >= SSL_CTX_use_certificate_file(m_ctx, file.c_str(), SSL_FILETYPE_PEM)) {
		ERR_print_errors_fp(stderr);
		return true;
	}

	file.assign(path+"/sgs.key");
	if (0 >= SSL_CTX_use_PrivateKey_file(m_ctx, file.c_str(), SSL_FILETYPE_PEM)) {
		ERR_print_errors_fp(stderr);
		return true;
	}

	if (CreateSocket())
		return true;

	return false;
}

bool CTLSServer::CreateSocket()
{
	struct sockaddr_storage addr;
	memset(&addr, 0, sizeof(struct sockaddr_storage));

	int family;
	if (m_address.npos != m_address.find(':')) {
		struct sockaddr_in6 *a = (struct sockaddr_in6 *)&addr;
		a->sin6_family = family = AF_INET6;
		a->sin6_port = htons(m_port);
		inet_pton(AF_INET6, m_address.c_str(), &(a->sin6_addr));
	} else if (m_address.npos != m_address.find('.')) {
		struct sockaddr_in *a = (struct sockaddr_in *)&addr;
		a->sin_family = family = AF_INET;
		a->sin_port = htons(m_port);
		inet_pton(AF_INET, m_address.c_str(), &(a->sin_addr));
	} else {
		fprintf(stderr, "Improper addess [%s], remote socket creation failed!\n", m_address.c_str());
		return true;
	}

	m_sock = socket(family, SOCK_STREAM, 0);
	if (m_sock < 0) {
		perror("Unable to create socket");
		return true;
	}

	if (0 > bind(m_sock, (struct sockaddr*)&addr, sizeof(addr))) {
		perror("Unable to bind");
		close(m_sock);
		return true;
	}

	if (0 > listen(m_sock, 1)) {
		perror("Unable to listen");
		close(m_sock);
		return true;
	}

	return false;
}

bool CTLSServer::GetCommand(std::string &command)
{
    struct sockaddr_storage addr;
	uint len = sizeof(addr);
	memset(&addr, 0, len);

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(m_sock, &readfds);

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	// don't care about writefds and exceptfds:
	// and we will return immediately
	int ret = select(m_sock+1, &readfds, NULL, NULL, &tv);

	if (ret && FD_ISSET(m_sock, &readfds)) {
		// there is someting to read
		m_client = accept(m_sock, (struct sockaddr*)&addr, &len);
		if (m_client < 0) {
			perror("Remote is unable to accept");
			return true;
		}

		if (AF_INET6 == addr.ss_family) {
			struct sockaddr_in6 *a = (struct sockaddr_in6 *)&addr;
			char s[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, &(a->sin6_addr), s, INET6_ADDRSTRLEN);
			printf("Remote IPV6 client from %s\n", s);
		} else {
			struct sockaddr_in *a = (struct sockaddr_in *)&addr;
			char s[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(a->sin_addr), s, INET_ADDRSTRLEN);
			printf("Remote IPV4 client from %s\n", s);
		}

		m_ssl = SSL_new(m_ctx);
		if (NULL == m_ssl) {
			CloseClient();
			perror("Remote can't create a new SSL");
			return true;
		} else {
			if (0 == SSL_set_fd(m_ssl, m_client)) {
				CloseClient();
				perror("Remote can't set fd");
				return true;
			} else {
				if (SSL_accept(m_ssl) <= 0) {
					CloseClient();
					ERR_print_errors_fp(stderr);
					return true;
				} else {
					char buf[256] = { 0 };
					SSL_read(m_ssl, buf, 256);
					if (m_password.compare(buf)) {
						printf("Password [%s] from remote client failed.\n", buf);
						SSL_write(m_ssl, "fail", 4);
						CloseClient();
						return true;
					} else {
						SSL_write(m_ssl, "pass", 4);
						char com[1024] = { 0 };
						SSL_read(m_ssl, com, 1024);
						command.assign(com);
					}
				}
			}
		}
		return false;
	} else {
		// nothing to read
		command.clear();
		return true;
	}

}

int CTLSServer::Write(const char *line)
{
	return SSL_write(m_ssl, line, strlen(line));
}

void CTLSServer::CloseClient()
{
	if (m_ssl)
		SSL_free(m_ssl);
	m_ssl = NULL;
	if (m_client >= 0)
		close(m_client);
	m_client = 0;
}
