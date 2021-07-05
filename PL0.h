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
	// 输出错误信息
	void OutputErrorInfo(const std::string& content, const std::vector<ErrorInfo>& errorInfo);
	// 输出符号表
	void OutputSymbols(const std::vector<Symbol> symbols);
	// 输出程序信息
	void OutputProgramInfo(const ProgramInfo& programInfo);
}