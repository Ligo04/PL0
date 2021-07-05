#pragma once

#include"PL0_Common.h"

namespace PL0
{
	/// <summary>
	/// 语法/语义分析器
	/// </summary>
	class ProgramParser
	{
	public:
		ProgramParser() :m_CurrLevel(), m_CurrProcAddressOffset(), m_CurrProcIndex() {}
		~ProgramParser() = default;

		// 语法单位

		// <程序>          ::= <常量说明部分><变量说明部分><过程说明部分><语句>.

		// <常量说明部分>  ::= {const <值类型><常量定义>{,<常量定义>};}
		// <常量定义>	   ::= <id>:=<值>
		
		// <变量说明部分>  ::= {<值类型><变量定义>{,<变量定义>};}
		// <变量定义>	   ::= <id>:=<值>

		// <过程说明部分>  ::= <过程首部><分程序>;{<过程说明部分>}
		// <过程首部>	   ::= procedure <id>'('[<值类型><id>{,<值类型><id>}]')'
		// <分程序>	       ::= <常量说明部分><变量说明部分><语句>

		// <语句>		   ::= <空语句>|<赋值语句>|<条件语句>|<while循环语句>|<for循环语句>|<过程调用语句>|<读语句>|<写语句>|<返回语句>|<复合语句>
		// <赋值语句>      ::= <自增/自减语句>  | <id> :=|+=|-=|*=|/=|%= <表示式>
		// <复合语句>      ::= begin <语句>{;<语句>} end
		// <空语句>		   ::= ε
		// <条件语句>      ::= if <条件> then <语句> {else if <条件> then <语句>}[else <语句>]
		// <while循环语句> ::= while <条件> do <语句>
		// <for循环语句>   ::= for <id>:=<表示式> step <表达式> until <条件> do <语句>
		// <读语句>        ::= read '('<id>,{,<id>}')'
		// <写语句>        ::= write '('<表达式>,{,<表达式>}')'
		// <过程调用语句>  ::= call <id> '('[<表达式>,{,<表达式>}]')'
		// <返回语句>      ::= return 
		// <自增/自减语句> ::= [++|--]<id>|<id>[++|--]

		// <表达式>        ::= [+|-]<项>{<加减运算符><项>}
		// <项>            ::= <因子>{<乘除模运算符><因子>}
		// <因子>          ::= <id>|<value>|'('<表达式>')'

		// <条件>          ::= <二级条件>{<逻辑或><二级条件>}
		// <二级条件>	   ::= <三级条件>{<逻辑与><三级条件>}
		// <三级条件>	   ::= <四级条件>{!=|= <四级条件>}
		// <四级条件>	   ::= <表达式>{>|>=|<|<= <表达式>}
		// <表达式>        ::= <二级表达式>{+|- <二级表达式>}
		// <二级表达式>	   ::= <三级表达式>{*|/|% <三级表达式>}
		// <三级表达式>	   ::= [!|-|+] <四级表达式>
		// <四级表达式>	   ::= <id>|<value>|'('<条件>')'

		


		// 对分析号的词组进行语法分析
		bool Parse(_In_ const std::vector<Symbol>& symbols);

		// 获取语法语义分析发现的错误信息
		const std::vector<ErrorInfo>& GetErrorInfos() const;

		// 获取翻译结果
		const ProgramInfo& GetProgramInfo() const;

	private:
		// <程序>          ::= <常量说明部分><变量说明部分><过程说明部分><语句>.
		bool ParseProgram();

		// <常量说明部分>  ::= {const <值类型><常量定义>{,<常量定义>};}
		bool ParseConstDesc(_Inout_ std::vector<Identifier>& constants);
		// <常量定义>      ::= <id>:=<值>
		void ParseConstDef(_In_ SymbolType type, _Inout_ std::vector<Identifier>& constants);

		// <变量说明部分>  ::= {<值类型><变量定义>{,<变量定义>};}
		bool ParseVarDesc(_Inout_ std::vector<Identifier>& variables);
		// <变量定义>      ::= <id>[:=<值>]
		void ParseVarDef(_In_ SymbolType type, _Inout_ std::vector<Identifier>& variables);

		// <过程说明部分>  ::= <过程首部><分程序>;{<过程说明部分>}
		bool ParseProcedureDesc();
		// <过程首部>      ::= procedure <id>'('[<值类型><id>{,<值类型><id>}]')';
		void ParseProcedureHeader();
		// <分程序>        ::= <常量说明部分><变量说明部分><语句>
		void ParseSubProcedure();

		// <语句>          ::= <空语句>|<赋值语句>|<条件语句>|<while循环语句>|<for循环语句>|<过程调用语句>|<读语句>|<写语句>|<返回语句>|<复合语句>|
		bool ParseStatement();
	    // <赋值语句>      ::=[++|--] <id> | <id> [++|--] | :=|+=|-=|*=|/=|%= <表达式>
		void ParseAssignStat();
		// <复合语句>      ::= begin <语句>{;<语句>} end
		void ParseComplexStat();
		// <条件语句>      ::= if <条件> then <语句> {else if <条件> then <语句>}[else <语句>]
		void ParseConditionStat();
		// <while循环语句> ::= while <条件> do <语句>
		void ParseWhileLoopStat();
		// <for循环语句>   ::= for <id>:=<表达式> step <表达式> until <条件> do <语句>
		void ParseForLoopStat();
		// <读语句>        ::= read '('<id>{,<id>}')'
		void ParseReadStat();
		// <写语句>        ::= write '('<表达式>{,<表达式>}')'
		void ParseWriteStat();
		// <过程调用语句>  ::= call <id>'('[<表达式>{,<表达式>}]')'
		void ParseCallStat();
		// <返回语句>      ::= return
		void ParseReturnStat();

		// <条件>          ::= <二级条件>{<逻辑或><二级条件>}
		void ParseConditionL1();
		// <二级条件>      ::= <三级条件>{<逻辑与><三级条件>}
		void ParseConditionL2();
		// <三级条件>      ::= <四级条件>{!=|= <四级条件>}
		void ParseConditionL3();
		// <四级条件>      ::= <表达式>{>|>=|<|<= <表达式>}
		void ParseConditionL4();
		// <表达式>        ::= <二级表达式>{+|- <二级表达式>}
		void ParseExpressionL1();
		// <二级表达式>    ::= <三级表达式>{*|/|% <三级表达式>}
		void ParseExpressionL2();
		// <三级表达式>    ::= [!|-|+]<四级表达式>
		void ParseExpressionL3();
		// <四级表达式>    ::= <id>|<value>|'('<条件>')'
		void ParseExpressionL4();

	private:
		// 检查当前符号是否为ST，如果不是，则填入错误码并抛出异常
		void SafeCheck(SymbolType ST, ErrorCode errorCode, const char* errorStr = "");
		// 获取id在符号表中的索引和层级
		uint32_t GetIDIndex(const std::string& str, int* outLevel = nullptr);

	private:
		int m_CurrLevel;										// 当前代码层级
		ProgramInfo m_ProgramInfo;								// 程序信息

		uint32_t m_CurrProcIndex;								// 当前过程标识符得对应索引
		uint32_t m_CurrProcAddressOffset;						// 当前过程对应起始地址偏移量
		std::vector<Symbol>::const_iterator m_pCurrSymbol;		// 当前符号的迭代器
		std::vector<Symbol>::const_iterator m_pEndSymbol;		// 符号集尾后的迭代器

		std::vector<ErrorInfo> m_ErrorInfos;					// 错误信息

		std::vector<Instruction> m_TempSelfSum;					// 存放临时的自运算结果
	};
}


