// Copyright (c) 2017 by Thomas A Early N7TAE

#pragma once

#include "SmartServerThread.h"

class CSmartServerAppD {

public:
	CSmartServerAppD(const std::string &configFile);
	~CSmartServerAppD();

	bool init();

	void run();

private:
	CSmartServerThread *m_thread;
	bool createThread();
};
