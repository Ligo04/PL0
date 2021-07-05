#pragma once

#include"PL0_Common.h"

namespace PL0
{
	/// <summary>
	/// �﷨/���������
	/// </summary>
	class ProgramParser
	{
	public:
		ProgramParser() :m_CurrLevel(), m_CurrProcAddressOffset(), m_CurrProcIndex() {}
		~ProgramParser() = default;

		// �﷨��λ

		// <����>          ::= <����˵������><����˵������><����˵������><���>.

		// <����˵������>  ::= {const <ֵ����><��������>{,<��������>};}
		// <��������>	   ::= <id>:=<ֵ>
		
		// <����˵������>  ::= {<ֵ����><��������>{,<��������>};}
		// <��������>	   ::= <id>:=<ֵ>

		// <����˵������>  ::= <�����ײ�><�ֳ���>;{<����˵������>}
		// <�����ײ�>	   ::= procedure <id>'('[<ֵ����><id>{,<ֵ����><id>}]')'
		// <�ֳ���>	       ::= <����˵������><����˵������><���>

		// <���>		   ::= <�����>|<��ֵ���>|<�������>|<whileѭ�����>|<forѭ�����>|<���̵������>|<�����>|<д���>|<�������>|<�������>
		// <��ֵ���>      ::= <����/�Լ����>  | <id> :=|+=|-=|*=|/=|%= <��ʾʽ>
		// <�������>      ::= begin <���>{;<���>} end
		// <�����>		   ::= ��
		// <�������>      ::= if <����> then <���> {else if <����> then <���>}[else <���>]
		// <whileѭ�����> ::= while <����> do <���>
		// <forѭ�����>   ::= for <id>:=<��ʾʽ> step <���ʽ> until <����> do <���>
		// <�����>        ::= read '('<id>,{,<id>}')'
		// <д���>        ::= write '('<���ʽ>,{,<���ʽ>}')'
		// <���̵������>  ::= call <id> '('[<���ʽ>,{,<���ʽ>}]')'
		// <�������>      ::= return 
		// <����/�Լ����> ::= [++|--]<id>|<id>[++|--]

		// <���ʽ>        ::= [+|-]<��>{<�Ӽ������><��>}
		// <��>            ::= <����>{<�˳�ģ�����><����>}
		// <����>          ::= <id>|<value>|'('<���ʽ>')'

		// <����>          ::= <��������>{<�߼���><��������>}
		// <��������>	   ::= <��������>{<�߼���><��������>}
		// <��������>	   ::= <�ļ�����>{!=|= <�ļ�����>}
		// <�ļ�����>	   ::= <���ʽ>{>|>=|<|<= <���ʽ>}
		// <���ʽ>        ::= <�������ʽ>{+|- <�������ʽ>}
		// <�������ʽ>	   ::= <�������ʽ>{*|/|% <�������ʽ>}
		// <�������ʽ>	   ::= [!|-|+] <�ļ����ʽ>
		// <�ļ����ʽ>	   ::= <id>|<value>|'('<����>')'

		


		// �Է����ŵĴ�������﷨����
		bool Parse(_In_ const std::vector<Symbol>& symbols);

		// ��ȡ�﷨����������ֵĴ�����Ϣ
		const std::vector<ErrorInfo>& GetErrorInfos() const;

		// ��ȡ������
		const ProgramInfo& GetProgramInfo() const;

	private:
		// <����>          ::= <����˵������><����˵������><����˵������><���>.
		bool ParseProgram();

		// <����˵������>  ::= {const <ֵ����><��������>{,<��������>};}
		bool ParseConstDesc(_Inout_ std::vector<Identifier>& constants);
		// <��������>      ::= <id>:=<ֵ>
		void ParseConstDef(_In_ SymbolType type, _Inout_ std::vector<Identifier>& constants);

		// <����˵������>  ::= {<ֵ����><��������>{,<��������>};}
		bool ParseVarDesc(_Inout_ std::vector<Identifier>& variables);
		// <��������>      ::= <id>[:=<ֵ>]
		void ParseVarDef(_In_ SymbolType type, _Inout_ std::vector<Identifier>& variables);

		// <����˵������>  ::= <�����ײ�><�ֳ���>;{<����˵������>}
		bool ParseProcedureDesc();
		// <�����ײ�>      ::= procedure <id>'('[<ֵ����><id>{,<ֵ����><id>}]')';
		void ParseProcedureHeader();
		// <�ֳ���>        ::= <����˵������><����˵������><���>
		void ParseSubProcedure();

		// <���>          ::= <�����>|<��ֵ���>|<�������>|<whileѭ�����>|<forѭ�����>|<���̵������>|<�����>|<д���>|<�������>|<�������>|
		bool ParseStatement();
	    // <��ֵ���>      ::=[++|--] <id> | <id> [++|--] | :=|+=|-=|*=|/=|%= <���ʽ>
		void ParseAssignStat();
		// <�������>      ::= begin <���>{;<���>} end
		void ParseComplexStat();
		// <�������>      ::= if <����> then <���> {else if <����> then <���>}[else <���>]
		void ParseConditionStat();
		// <whileѭ�����> ::= while <����> do <���>
		void ParseWhileLoopStat();
		// <forѭ�����>   ::= for <id>:=<���ʽ> step <���ʽ> until <����> do <���>
		void ParseForLoopStat();
		// <�����>        ::= read '('<id>{,<id>}')'
		void ParseReadStat();
		// <д���>        ::= write '('<���ʽ>{,<���ʽ>}')'
		void ParseWriteStat();
		// <���̵������>  ::= call <id>'('[<���ʽ>{,<���ʽ>}]')'
		void ParseCallStat();
		// <�������>      ::= return
		void ParseReturnStat();

		// <����>          ::= <��������>{<�߼���><��������>}
		void ParseConditionL1();
		// <��������>      ::= <��������>{<�߼���><��������>}
		void ParseConditionL2();
		// <��������>      ::= <�ļ�����>{!=|= <�ļ�����>}
		void ParseConditionL3();
		// <�ļ�����>      ::= <���ʽ>{>|>=|<|<= <���ʽ>}
		void ParseConditionL4();
		// <���ʽ>        ::= <�������ʽ>{+|- <�������ʽ>}
		void ParseExpressionL1();
		// <�������ʽ>    ::= <�������ʽ>{*|/|% <�������ʽ>}
		void ParseExpressionL2();
		// <�������ʽ>    ::= [!|-|+]<�ļ����ʽ>
		void ParseExpressionL3();
		// <�ļ����ʽ>    ::= <id>|<value>|'('<����>')'
		void ParseExpressionL4();

	private:
		// ��鵱ǰ�����Ƿ�ΪST��������ǣ�����������벢�׳��쳣
		void SafeCheck(SymbolType ST, ErrorCode errorCode, const char* errorStr = "");
		// ��ȡid�ڷ��ű��е������Ͳ㼶
		uint32_t GetIDIndex(const std::string& str, int* outLevel = nullptr);

	private:
		int m_CurrLevel;										// ��ǰ����㼶
		ProgramInfo m_ProgramInfo;								// ������Ϣ

		uint32_t m_CurrProcIndex;								// ��ǰ���̱�ʶ���ö�Ӧ����
		uint32_t m_CurrProcAddressOffset;						// ��ǰ���̶�Ӧ��ʼ��ַƫ����
		std::vector<Symbol>::const_iterator m_pCurrSymbol;		// ��ǰ���ŵĵ�����
		std::vector<Symbol>::const_iterator m_pEndSymbol;		// ���ż�β��ĵ�����

		std::vector<ErrorInfo> m_ErrorInfos;					// ������Ϣ

		std::vector<Instruction> m_TempSelfSum;					// �����ʱ����������
	};
}


