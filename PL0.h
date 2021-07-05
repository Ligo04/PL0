#pragma once

#include"PL0_WordParser.h"
#include"PLO_ProgramParser.h"
#include"PL0_VirtualMachine.h"

#include"PL0_ErrorMsg.h"

#ifdef  _WIN64
#error "x64 is unsupported!Switch back to x86"
#endif //  _WIN64


namespace PL0
{
	// ���������Ϣ
	void OutputErrorInfo(const std::string& content, const std::vector<ErrorInfo>& errorInfo);
	// ������ű�
	void OutputSymbols(const std::vector<Symbol> symbols);
	// ���������Ϣ
	void OutputProgramInfo(const ProgramInfo& programInfo);
}