#pragma once
#include"PL0_Common.h"


namespace PL0
{
	/// <summary>
	/// �ʷ�������
	/// </summary>
	class WordParser
	{
	public:
		WordParser() :m_pCurrContent(), m_CurrCoord(), m_Symbols(std::vector<Symbol>()), m_ErrorInfos(std::vector<ErrorInfo>()) {};
		~WordParser() = default;

		//�����ݽ��������Ĵʷ�����
		bool Parse(_In_ const std::string& content);
		//�����ݽ��������Ĵʷ�����
		bool Parse(_In_ const char* content);

		//��ȡ������������з���
		const std::vector<Symbol>& GetSymbols() const;

		//��ȡ�ʷ������Ĵ�����Ϣ
		const std::vector<ErrorInfo>& GetErrorInfos() const;
	private:
		//��ȡ��һ�����ţ����ƽ�����������
		Symbol GetNextSymbol(_Out_ ErrorInfo* perrorInfo);

		//��ȡ��һ���ַ������ƽ�����������
		char GetNextChar();

		//������һ���ַ������ƽ�����������
		void IgnoreNextChar();

		//��ȡ��һ���ַ��������ƽ�����������
		char PeekNextChar();

		//����һ���ַ����õ��������ĩβλ�ã������ƽ�����������
		//��content = "\x12\' + ",��EndPos = "\' + "
		//��content = "e\"",��EndPos = "\""
		void TryParseChar(_In_ const char* pcontent, _Out_ const char** pEndCharPos);

	private:
		const char* m_pCurrContent;				// ��ǰ�����Ĵ�����������λ��
		std::vector<Symbol> m_Symbols;			// ���ż�
		CodeCoord m_CurrCoord;					// ��ǰ����
		std::vector<ErrorInfo> m_ErrorInfos;	// ������Ϣ
	};
}

